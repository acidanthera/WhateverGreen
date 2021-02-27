//
//  kern_ngfx.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_ngfx_hpp
#define kern_ngfx_hpp

#include <IOKit/IOService.h>

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IOService.h>
#include <IOKit/ndrvsupport/IONDRVFramebuffer.h>

// Assembly exports for restoreLegacyOptimisations
extern "C" bool wrapVaddrPreSubmitTrampoline(void *that);
extern "C" bool orgVaddrPresubmitTrampoline(void *that);
extern "C" bool (*orgVaddrPreSubmit)(void *addr);

class NGFX {
public:
	void init();
	void deinit();

	/**
	 *  Property patching routine
	 *
	 *  @param patcher  KernelPatcher instance
	 *  @param info     device info
	 */
	void processKernel(KernelPatcher &patcher, DeviceInfo *info);

	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 *
	 *  @return true if patched anything
	 */
	bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

private:
	/**
	 *  Private self instance for callbacks
	 */
	static NGFX *callbackNGFX;

	/**
	 *  NVIDIA Apple Developer Team ID used for permission override
	 */
	static constexpr const char *NvidiaTeamId { "6KR3T733EC" };

	/**
	 *  Force Web Driver compatibility, -1 lets us override via GPU property
	 */
	int forceDriverCompatibility {-1};

	/**
	 *  Virtual address submission performance fix
	 */
	int fifoSubmit {-1};

	/**
	 *  Disable team unrestriction patches fixing visual glitches on 10.12 with Web drivers
	 */
	bool disableTeamUnrestrict {false};

	/**
	 *  nvGpFifoChannel::PreSubmit function type
	 */
	using t_fifoPreSubmit = bool (*)(void *, uint32_t, void *, uint32_t, void *, uint32_t *, uint64_t, uint32_t);

	/**
	 *  Pointer to nvGpFifoChannel::Prepare function
	 */
	bool (*orgFifoPrepare)(void *fifo) {nullptr};

	/**
	 *  Pointer to nvGpFifoChannel::Complete function
	 */
	void (*orgFifoComplete)(void *fifo) {nullptr};

	/**
	 *  Pointer to csfg_get_teamid function
	 */
	const char *(*orgCsfgGetTeamId)(void *fg) {nullptr};

	/**
	 *  Original csfg_get_platform_binary function
	 */
	mach_vm_address_t orgCsfgGetPlatformBinary {};

	/**
	 * Original SetAccelProperties functions for official and web drivers
	 */
	mach_vm_address_t orgSetAccelProperties {};
	mach_vm_address_t orgSetAccelPropertiesWeb {};

	/**
	 *  Original NVDAStartupWeb::probe function
	 */
	mach_vm_address_t orgStartupWebProbe {};

	/**
	 *  Original IONDRVFramebuffer::_doControl function
	 */
	mach_vm_address_t orgNdrvDoControl {};

	/**
	 *  Restore legacy optimisations from 10.13.0, which fix lags for Kepler GPUs.
	 *  For Web drivers it is very experimental, since they have a lot of additional different (broken) code.
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void restoreLegacyOptimisations(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  Add IOVARenderer properties to fix hardware video decoding
	 *
	 *  @param that  accelerator instance
	 */
	void applyAcceleratorProperties(IOService *that);

	/**
	 *  csfg_get_platform_binary wrapper used for fixing visual glitches with web drivers due to missing entitlements.
	 *
	 *  @param fg  codesign information
	 *
	 *  @result 1 if the binary has platform access rights
	 */
	static int wrapCsfgGetPlatformBinary(void *fg);

	/**
	 *  csfg_get_platform_binary wrapper used for fixing visual glitches with web drivers due to missing entitlements.
	 *
	 *  @param fg  codesign information
	 *
	 *  @result 1 if the binary is platform
	 */
	static bool wrapVaddrPreSubmit(void *that);

	/**
	 *  SetAccelProperties wrapper used to add IOVARenderer properties
	 */
	static void wrapSetAccelProperties(IOService *that);
	static void wrapSetAccelPropertiesWeb(IOService *that);

	/**
	 *  NVDAStartup::probe wrapper used to force-enable web-drivers
	 */
	static IOService *wrapStartupWebProbe(IOService *that, IOService *provider, SInt32 *score);

	/**
	 *  IONDRVFramebuffer::_doControl wrapper used to avoid debug spam
	 */

	static IOReturn wrapNdrvDoControl(IONDRVFramebuffer *fb, UInt32 code, void *params);
};

#endif /* kern_ngfx_hpp */
