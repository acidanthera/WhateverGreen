//
//  kern_igfx_memory.cpp
//  WhateverGreen
//
//  Created by FireWolf on 8/30/20.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"
#include <Headers/kern_util.hpp>
#include <Headers/kern_disasm.hpp>

void IGFX::DVMTCalcFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
}

void IGFX::DVMTCalcFix::deinit() {}

void IGFX::DVMTCalcFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Guard: Disable the patch if it is not available on the current Intel platforms
	if (!available) {
		SYSLOG("igfx", "DVMT: This fix is not available on the current platform and has been disabled.");
		return;
	}
		
	// Enable the fix if designated boot argument or device property is found
	enabled = checkKernelArgument("-igfxdvmt");
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-dvmt-calc-fix") != nullptr;
	if (!enabled)
		return;
	
	// Guard: Wait for the device to be published in the plane, otherwise `read` will crash on macOS Big Sur
	if (!WIOKit::awaitPublishing(info->videoBuiltin)) {
		SYSLOG("igfx", "DVMT: Failed to wait for the graphics device to be published. The fix has been disabled.");
		enabled = false;
		return;
	}
	
	// Read the DVMT preallocated memory set in BIOS from the GMCH Graphics Control field at 0x50 (PCI0,2,0)
	// TODO: Lilu needs to be updated to define the enumeration case `kIOPCIConfigGraphicsControl`
	auto gms = WIOKit::readPCIConfigValue(info->videoBuiltin, /*WIOKit::kIOPCIConfigGraphicsControl*/ 0x50, 0, 16) >> 8;
	
	// Disable the fix if the GMS value can be calculated by Apple's formula correctly
	// Reference: 10th Generation Intel Processor Families: Datasheet, Volume 2, Section 4.1.28
	// https://www.intel.com/content/www/us/en/products/docs/processors/core/10th-gen-core-families-datasheet-vol-2.html
	if (gms < 0x10) {
		dvmt = gms * 32;
		enabled = false;
		DBGLOG("igfx", "DVMT: GMS value is supported by the driver. The fix has been disabled.");
	} else if (gms == 0x20 || gms == 0x30 || gms == 0x40) {
		dvmt = gms * 32;
	} else if (gms >= 0xF0 && gms <= 0xFE) {
		dvmt = ((gms & 0x0F) + 1) * 4;
	} else {
		SYSLOG("igfx", "DVMT: GMS value is reserved. Check your BIOS settings. DVMT will be set to 0.");
		return;
	}
	
	DBGLOG("igfx", "DVMT: GMS value is 0x%02x; DVMT pre-allocated memory is %d MB.", gms, dvmt);
#ifdef DEBUG
	info->videoBuiltin->setProperty("fw-dvmt-gms-field-value", gms, 8);
	info->videoBuiltin->setProperty("fw-dvmt-preallocated-memory", dvmt, 32);
#endif
	dvmt *= (1024 * 1024); /* MB to Bytes */
}

void IGFX::DVMTCalcFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	// Find the address of `AppleIntelFramebufferController::FBMemMgr_Init()`
	auto startAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController13FBMemMgr_InitEv", address, size);
	if (!startAddress) {
		SYSLOG("igfx", "DVMT: Failed to find FBMemMgr_Init().");
		patcher.clearError();
		return;
	}
	
	//
	// Locate Apple's formula: (GGCRegValue << 0x11) & 0xFE000000
	//
	// macOS Catalina 10.15.6 (19G2021)
	// loc_3f61f: movl $0x50, %esi
	// loc_3f624: call __ZN11IOPCIDevice20extendedConfigRead16Ey
	// loc_3f629: shll $0x11, %eax
	// loc_3f62c: andl $0xFE000000, %eax
	// loc_3f631: movl %eax, 0xd7c(%r15)
	// The final result is saved into the framebuffer controller instance and is used later in FBMemMgr_Init().
	//
	// Abstract:
	//
	// The amount of DVMT pre-allocated memory is a pain on laptops.
	// Apple’s graphics driver reads the value set in the BIOS or the firmware
	// and uses a “magic” formula to calculate the amount of memory in bytes.
	// Unfortunately, the formula only works for a pre-allocated memory size that is a multiple of 32MB.
	// i.e. 0M, 32M, 64M, ..., 512M. (The corresponding GMS values are 0x00, 0x01, ..., 0x10.)
	// Non-Apple laptops normally set DVMT to 32MB on CFL- platforms, and related kernel panics can be solved by patching the framebuffer properly.
	//
	// However, problem arises as new laptops now have DVMT set to 60MB on ICL+ platforms by default,
	// and the framebuffer controller ends up with initializing the stolen memory manager with an incorrect amount of pre-allocated memory.
	// Later, the accelerator driver fails to allocate objects on the stolen memory area,
	// and hence a kernel panic is triggered inside the `IGAccelTask::withOptions()` function.
	//
	// Side Note:
	// Apple has removed the kernel panic from `AppleIntelFramebufferController::FBMemMgr_Init()` if the stolen memory is insufficient. (ICL)
	// As a result, one will encounter a kernel panic saying "Unsupported ICL SKU" triggered in `IntelAccelerator::getGPUInfo()`.
	// We could manually fix this issue, but later a page fault occurs inside `IGAccelTask::withOptions()`.
	// This is because the accelerator driver fails to allocate objects on the stolen memory.
	// The pointer to the object is set to NULL, and the cleanup procedure `IOAccelTask::release()` tries to dereference the NULL object.
	//
	// Even though one might be able to modify DVMT settings via `EFI shell` or `RU.EFI`,
	// these methods are not applicable to some laptops, such as Surface Pro 7, that use custom firmware.
	// As such, this patch locates instructions related to calculating the amount of memory available and identifies which register stores the final result.
	// It calculates the correct number of bytes beforehand and finds the proper instruction to copy the value to the result register.
	// Consequently, the framebuffer controller will initialize the memory manager with proper values and aforementioned kernel panics can be avoided.
	//
	// - FireWolf
	// - 2020.08
	//
	hde64s handle;
	uint64_t shllAddr = 0, andlAddr = 0; /* Instruction Address  */
	uint32_t shllSize = 0, andlSize = 0; /* Instruction Length   */
	uint32_t shllDstr = 0, andlDstr = 0; /* Destination Register */
	
	// e.g. movl $0x03C00000, %eax (60MB DVMT)
	// Apply the middle 5 bytes if the target register is %eax;
	// Apply the last 6 bytes if the target register is below %r8d;
	// Apply all 7 bytes if the target register is or above %r8d.
	uint8_t movl[] = {0x41, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x90};
	uint8_t nops[] = {0x90, 0x90, 0x90, 0x90};
	
	// Patch Heuristics:
	// We need to locate two instructions `shll` and `andl` that manipulate the value read from the GGC field,
	// but we cannot assume that they always stay together and operate on the register %eax.
	// As such, we need to first figure out which register is of interest, and
	// we need 5 or 6 bytes to move a 32-bit integer to the register manipulated by the above instructions.
	// Since `andl` is 5 - 7 bytes long, we could just replace it with a "movl" and then erases `shll` by filling `nop`s.
	for (auto index = 0; index < 64; index += 1) {
		auto size = Disassembler::hdeDisasm(startAddress, &handle);
		
		// Guard: Should be able to disassemble the function
		if (handle.flags & F_ERROR) {
			SYSLOG("igfx", "DVMT: Failed to disassemble FBMemMgr_Init().");
			break;
		}
		
		// Instruction: shll $0x11, %???
		// 3 bytes long if DSTReg < %r8d, otherwise 4 bytes long
		if (handle.opcode == 0xC1 && handle.imm.imm8 == 0x11) {
			shllAddr = startAddress;
			shllSize = handle.len;
			shllDstr = (handle.rex_b << 3) | handle.modrm_rm;
			SYSLOG("igfx", "DVMT: Found the shll instruction. Length = %d; DSTReg = %d.", shllSize, shllDstr);
		}
		
		// Instruction: andl $0xFE000000, %???
		// 5 bytes long if DSTReg is %eax; 6 bytes long if DSTReg < %r8d; otherwise 7 bytes long.
		if ((handle.opcode == 0x25 || handle.opcode == 0x81) && handle.imm.imm32 == 0xFE000000) {
			andlAddr = startAddress;
			andlSize = handle.len;
			andlDstr = (handle.rex_b << 3) | handle.modrm_rm;
			SYSLOG("igfx", "DVMT: Found the andl instruction. Length = %d; DSTReg = %d.", andlSize, andlDstr);
		}
		
		// Guard: Calculate and apply the binary patch if we have found both instructions
		if (shllAddr && andlAddr) {
			// Update the `movl` instruction with the actual amount of DVMT preallocated memory
			*(uint32_t*) (movl + 2) = dvmt;
			
			// Update the `movl` instruction with the actual destination register
			// Find the actual starting point of the patch and the number of bytes to patch
			uint8_t* patchStart;
			uint32_t patchSize;
			if (andlDstr >= 8) {
				// %r8d, %r9d, ..., %r15d
				movl[1] += (andlDstr - 8);
				patchStart = movl;
				patchSize = 7;
			} else {
				// %eax, %ecx, ..., %edi
				movl[1] += andlDstr;
				patchStart = (movl + 1);
				patchSize = andlDstr == 0 ? /* %eax */ 5 : /* others */ 6;
			}
			
			// Guard: Prepare to apply the binary patch
			if (MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS) {
				SYSLOG("igfx", "DVMT: Failed to set kernel writing. Aborted patching.");
				return;
			}
			
			// Replace `shll` with `nop`s
			// The number of nops is determined by the actual instruction length
			lilu_os_memcpy(reinterpret_cast<void*>(shllAddr), nops, shllSize);
			
			// Replace `andl` with `movl`
			// The patch contents and size are determined by the destination register of `andl`
			lilu_os_memcpy(reinterpret_cast<void*>(andlAddr), patchStart, patchSize);
			
			// Finished applying the binary patch
			MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
			DBGLOG("igfx", "DVMT: Calculation patch has been applied successfully.");
			return;
		}
		
		startAddress += size;
	}
	
	SYSLOG("igfx", "DVMT: Failed to find instructions of interest. Aborted patching.");
}

void IGFX::DVMTCalcFix::processAcceleratorKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {}
