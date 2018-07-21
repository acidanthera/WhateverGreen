// rd_route.c
// Copyright (c) 2014-2015 Dmitry Rodionov
//
// This software may be modified and distributed under the terms
// of the MIT license.  See the LICENSE file for details.

#include <stdlib.h>         // realloc()
#include <libgen.h>         // basename()
#include <assert.h>         // assert()
#include <stdio.h>          // fprintf()
#include <dlfcn.h>          // dladdr()

#include "TargetConditionals.h"
#if defined(__i386__) || defined(__x86_64__)
	#if !(TARGET_IPHONE_SIMULATOR)
		#include <mach/mach_vm.h> // mach_vm_*
	#else
		#include <mach/vm_map.h>  // vm_*
		#define mach_vm_address_t vm_address_t
		#define mach_vm_size_t vm_size_t
		#define mach_vm_allocate vm_allocate
		#define mach_vm_deallocate vm_deallocate
		#define mach_vm_write vm_write
		#define mach_vm_remap vm_remap
		#define mach_vm_protect vm_protect
		#define NSLookupSymbolInImage(...) ((void)0)
		#define NSAddressOfSymbol(...) ((void)0)
	#endif
#else
	#error rd_route doesn't work on iOS
#endif

#include <mach-o/dyld.h>    // _dyld_*
#include <mach-o/nlist.h>   // nlist/nlist_64
#include <mach/mach_init.h> // mach_task_self()
#include "rd_route.h"

#define RDErrorLog(format, ...) fprintf(stderr, "%s:%d:\n\terror: "format"\n", \
	__FILE__, __LINE__, ##__VA_ARGS__)

#if defined(__x86_64__)
	typedef struct mach_header_64     mach_header_t;
	typedef struct segment_command_64 segment_command_t;
	#define LC_SEGMENT_ARCH_INDEPENDENT   LC_SEGMENT_64
	typedef struct nlist_64 nlist_t;
#else
	typedef struct mach_header        mach_header_t;
	typedef struct segment_command    segment_command_t;
	#define LC_SEGMENT_ARCH_INDEPENDENT   LC_SEGMENT
	typedef struct nlist nlist_t;
#endif

typedef struct rd_injection {
	mach_vm_address_t injected_mach_header;
	mach_vm_address_t target_address;
} rd_injection_t;

static mach_vm_size_t _image_size(void *image, mach_vm_size_t image_slide, mach_vm_address_t *data_segment_offset);
static kern_return_t  _remap_image(void *image,  mach_vm_size_t image_slide, mach_vm_address_t *new_location);
static kern_return_t  _insert_jmp(void* where, void* to);
static kern_return_t  _patch_memory(void *address, mach_msg_type_number_t count, uint8_t *new_bytes);
static void*          _function_ptr_from_name(const char *function_name, const char *suggested_image_name);
static void*          _function_ptr_within_image(const char *function_name, void *macho_image_header, uintptr_t vm_image_slide);


int rd_route(void *function, void *replacement, void **original_ptr)
{
	if (!function || !replacement) {
		return KERN_INVALID_ARGUMENT;
	}
	if (function == replacement) {
		return KERN_INVALID_ADDRESS;
	}

	int ret = KERN_SUCCESS;
	if (original_ptr) {
		ret = rd_duplicate_function(function, original_ptr);
	}

	if (ret == KERN_SUCCESS) {
		ret = _insert_jmp(function, replacement);
	}

	return (ret);
}


int rd_route_byname(const char *function_name, const char *suggested_image_name, void *replacement, void **original)
{
	/**
	 * These cases are actually handled by rd_route() function itself, but we don't want to dig over
	 * all loaded images just to do nothing at the end.
	 */
	if (!function_name|| !replacement) {
		return KERN_INVALID_ARGUMENT;
	}
	void *function = _function_ptr_from_name(function_name, suggested_image_name);

	return rd_route(function, replacement, original);
}


int rd_duplicate_function(void *function, void **duplicate)
{
	if (!function || !duplicate) {
		return (KERN_INVALID_ARGUMENT);
	}

	void *image = NULL;
	mach_vm_size_t image_slide = 0;

	/* Obtain the macho header image which contains the function */
	Dl_info image_info = {0};
	if (dladdr(function, &image_info)) {
		image = image_info.dli_fbase;
	}
	if (!image) {
		RDErrorLog("Could not found a loaded mach-o image containing the given function.");
		return KERN_FAILURE;
	}

	for (uint32_t i = 0; i < _dyld_image_count(); i++) {
		if (image == _dyld_get_image_header(i)) {
			image_slide = _dyld_get_image_vmaddr_slide(i);
			break;
		}
	}

	static rd_injection_t *injection_history = NULL;
	static uint16_t history_size = 0;
	/* Look up the injection history if we already have this image remapped. */
	for (uint16_t i = 0; i < history_size; i++) {
		if (injection_history[i].injected_mach_header == (mach_vm_address_t)image) {
			if (duplicate) {
				*duplicate = (void *)injection_history[i].target_address + (function - image);
			}
			return KERN_SUCCESS;
		}
	}

	/**
	 * Take a note that we have already remapped this mach-o image, so won't do this
	 * again when routing another function from the image.
	 */
	size_t new_size = history_size + 1;
	injection_history = realloc(injection_history, sizeof(*injection_history) * new_size);
	injection_history[history_size].injected_mach_header = (mach_vm_address_t)image;
	injection_history[history_size].target_address = ({
		mach_vm_address_t target = 0;
		kern_return_t err = _remap_image(image, image_slide, &target);
		if (KERN_SUCCESS != err) {
			RDErrorLog("Failed to remap segements of the image. See error messages above.");
			return (err);
		}
		/* Backup an original function implementation if needed */
		if (duplicate) {
			*duplicate = (void *)(target + (function - image));
		}

		target;
	});

	history_size = new_size;

	return KERN_SUCCESS;
}


static kern_return_t _remap_image(void *image, mach_vm_size_t image_slide, mach_vm_address_t *new_location)
{
	assert(image);
	assert(new_location);

	mach_vm_address_t data_segment_offset = 0;
	mach_vm_size_t image_size = _image_size(image, image_slide, &data_segment_offset);

	kern_return_t err = KERN_FAILURE;
	/*
	 * On x86_64 for some images __DATA segment is mapped far from other segments.
	 * To handle it we need to allocate a memory zone that has enough capacity
	 * for both __DATA's island and the rest of the image.
	 *
	 * Since __DATA is mapped even *before* the actual image, we are going to have
	 * a "safety zone" (the space between __TEXT and __DATA segments) by skipping
	 * the first data_segment_offset bytes. Thus, __DATA will be remapped into a
	 * valid (i.e. owned by user) memory space.
	 * Here's a map of whole memory zone:
	 *                                               image_size bytes
	 *                                            ---------------------
	 *        __DATA island                      /                     \
	 *            /   \                         /                       \
	 *  >--------*-----*-----------------------*------(...)------*-------*--->
	 *           |                             |                 |
	 *           |                             |                 |
	 *        __DATA                        __TEXT          __LINKEDIT
	 *           |                             |
	 *           |                             |
	 *      *new_location        *new_location + data_segment_offset
	 *
	 * NOTE: vmaddr(__TEXT) - vmaddr(__DATA) = const, so we can't remap __DATA
	 * into any place we want.
	 */
	*new_location = 0;
	err = mach_vm_allocate(mach_task_self(), new_location,
	                      (image_size + data_segment_offset), VM_FLAGS_ANYWHERE);
	*new_location += data_segment_offset;

	if (KERN_SUCCESS != err) {
		RDErrorLog("Failed to allocate a memory region for the function copy - mach_vm_allocate() returned 0x%x\n", err);
		return (err);
	}

	const mach_header_t *header = (mach_header_t *)image;
	struct load_command *cmd = (struct load_command *)(header + 1);

	/**
	 * Remap each segment of the mach-o image into a new location.
	 * New location is:
	 * -> target_image + segment.offset_in_image;
	 */
	for (uint32_t i = 0; (i < header->ncmds); i++, cmd = (void *)cmd + cmd->cmdsize) {
		if (cmd->cmd != LC_SEGMENT_ARCH_INDEPENDENT) continue;

		segment_command_t *segment = (segment_command_t *)cmd;
		mach_vm_address_t vmaddr = segment->vmaddr;
		mach_vm_size_t    vmsize = segment->vmsize;

		if (vmsize == 0) continue;

		mach_vm_address_t seg_source = vmaddr + image_slide;
		mach_vm_address_t seg_target = *new_location + (seg_source - (mach_vm_address_t)header);

		vm_prot_t cur_protection, max_protection;

		err = mach_vm_remap(
			/* Target information */
			mach_task_self(), &seg_target, vmsize, 0x0,
			/* Flags */
			(VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE),
			/* Source information */
			mach_task_self(), seg_source,
			/* Should we copy this region? (it will be directly mapped otherwise) */
			false,
			/* The initial protection for the new region */
			&cur_protection, &max_protection,
			/* The inheritance attribute */
			VM_INHERIT_SHARE);
	}

	if (KERN_SUCCESS != err) {
		RDErrorLog("Failed to remap the function implementation to the new address - mach_vm_remap() returned 0x%x\n", err);
	}

	return (err);
}


static mach_vm_size_t _image_size(void *image, mach_vm_size_t image_slide, mach_vm_address_t *data_segment_offset)
{
	assert(image);

	const mach_header_t *header = (mach_header_t *)image;
	struct load_command *cmd = (struct load_command *)(header + 1);

	mach_vm_address_t image_addr = (mach_vm_address_t)image - image_slide;
	mach_vm_address_t image_end = image_addr;
	mach_vm_address_t data_vmaddr = 0, text_vmaddr = 0;

	for (uint32_t i = 0; (i < header->ncmds); i++, cmd = (void *)cmd + cmd->cmdsize) {
		if (cmd->cmd != LC_SEGMENT_ARCH_INDEPENDENT) continue;

		segment_command_t *segment = (segment_command_t *)cmd;
		if (0 == strcmp("__DATA", segment->segname)) {
			data_vmaddr = segment->vmaddr;
		}  else if (0 == strcmp("__TEXT", segment->segname)) {
			text_vmaddr = segment->vmaddr;
		}

		if ((segment->vmaddr + segment->vmsize) > image_end) {
			image_end = segment->vmaddr + segment->vmsize;
		}
	}

	if (data_vmaddr < text_vmaddr) {
		*data_segment_offset = text_vmaddr - data_vmaddr;
	}

	return (image_end - image_addr);
}


static kern_return_t _insert_jmp(void* where, void* to)
{
	assert(where);
	assert(to);
	/**
	 * We are going to use an absolute JMP instruction for x86_64
	 * and a relative one for i386.
	 */
#if defined (__x86_64__)
	mach_msg_type_number_t size_of_jump = (sizeof(uintptr_t) * 2);
#else
	mach_msg_type_number_t size_of_jump = (sizeof(int) + 1);
#endif

	kern_return_t err = KERN_SUCCESS;
	uint8_t opcodes[size_of_jump];
#if defined(__x86_64__)
	opcodes[0] = 0xFF;
	opcodes[1] = 0x25;
	*((int*)&opcodes[2]) = 0;
	*((uintptr_t*)&opcodes[6]) = (uintptr_t)to;
	err = _patch_memory((void *)where, size_of_jump, opcodes);
#else
	int offset = (int)(to - where - size_of_jump);
	opcodes[0] = 0xE9;
	*((int*)&opcodes[1]) = offset;
	err = _patch_memory((void *)where, size_of_jump, opcodes);
#endif

	return (err);
}


static kern_return_t _patch_memory(void *address, mach_msg_type_number_t count, uint8_t *new_bytes)
{
	assert(address);
	assert(count > 0);
	assert(new_bytes);

	kern_return_t kr = 0;
	kr = mach_vm_protect(mach_task_self(), (mach_vm_address_t)address, (mach_vm_size_t)count, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE | VM_PROT_COPY);
	if (kr != KERN_SUCCESS) {
		RDErrorLog("mach_vm_protect() failed with error: 0x%x", kr);
		return (kr);
	}

	kr = mach_vm_write(mach_task_self(), (mach_vm_address_t)address, (vm_offset_t)new_bytes, count);
	if (kr != KERN_SUCCESS) {
		RDErrorLog("mach_vm_write() failed with error: 0x%x", kr);
		return (kr);
	}

	kr = mach_vm_protect(mach_task_self(), (mach_vm_address_t)address, (mach_vm_size_t)count, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
	if (kr != KERN_SUCCESS) {
		RDErrorLog("mach_vm_protect() failed with error: 0x%x", kr);
	}

	return (kr);
}

static void* _function_ptr_from_name(const char *function_name, const char *suggested_image_name)
{
	assert(function_name);

	for (uint32_t i = 0; i < _dyld_image_count(); i++) {
		void *header = (void *)_dyld_get_image_header(i);
		uintptr_t vmaddr_slide = _dyld_get_image_vmaddr_slide(i);

		if (!suggested_image_name) {
			void *ptr = _function_ptr_within_image(function_name, header, vmaddr_slide);
			if (ptr) return ptr;
		} else {
			int name_matches = 0;
			const char *image_name = _dyld_get_image_name(i);
			name_matches |= !strcmp(suggested_image_name, image_name);
			name_matches |= !strcmp(suggested_image_name, basename((char *)image_name));
			if (name_matches) {
				return _function_ptr_within_image(function_name, header, vmaddr_slide);
			}
		}
	}
	RDErrorLog("Failed to find symbol `%s` in the current address space.", function_name);

	return NULL;
}


static void* _function_ptr_within_image(const char *function_name, void *macho_image_header, uintptr_t vmaddr_slide)
{
	assert(function_name);
	assert(macho_image_header);
	/**
	 * Try the system NSLookup API to find out the function's pointer withing the specifed header.
	 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
	void *pointer_via_NSLookup = ({
		NSSymbol symbol = NSLookupSymbolInImage(macho_image_header, function_name,
			NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
		NSAddressOfSymbol(symbol);
	});
#pragma clang diagnostic pop
	if (pointer_via_NSLookup) return pointer_via_NSLookup;

	/**
	 * So NSLookup() has failed and we have to parse a mach-o image to find the function's symbol
	 * in a symbols list.
	 */
	const mach_header_t *header     = macho_image_header;
	struct symtab_command *symtab   = NULL;
	segment_command_t *seg_linkedit = NULL;
	segment_command_t *seg_text     = NULL;
	struct load_command *command    = (struct load_command *)(header+1);

	for (uint32_t i = 0; i < header->ncmds; i++) {
		switch(command->cmd) {
			case LC_SEGMENT_ARCH_INDEPENDENT: {
				segment_command_t *segment = (segment_command_t *)command;
				if (0 == strcmp(SEG_TEXT, segment->segname)) {
					seg_text = segment;
				} else
				if (0 == strcmp(SEG_LINKEDIT, segment->segname)) {
					seg_linkedit = segment;
				}
				break;
			}
			case LC_SYMTAB: {
				symtab = (struct symtab_command *)command;
				break;
			}
			default: {}
		}
		command = (void *)command + command->cmdsize;
	}

	if (!symtab || !seg_linkedit || !seg_text) {
		RDErrorLog("The given Mach-O image header is missing some important sections.");
		return NULL;
	}

	uintptr_t file_slide = ((uintptr_t)seg_linkedit->vmaddr - (uintptr_t)seg_text->vmaddr) - seg_linkedit->fileoff;
	uintptr_t strings = (uintptr_t)header + (symtab->stroff + file_slide);
	nlist_t *sym = (nlist_t *)((uintptr_t)header + (symtab->symoff + file_slide));

	for (uint32_t i = 0; i < symtab->nsyms; i++, sym++) {
		if (!sym->n_value) continue;
		const char *symbol_name = (const char *)strings + sym->n_un.n_strx;
		/* Ignore the leading underscore ("_") character for a real symbol name */
		if (0 == strcmp(symbol_name+1, function_name)) {
			return (void *)(sym->n_value + vmaddr_slide);
		}
	}

	return NULL;
}
