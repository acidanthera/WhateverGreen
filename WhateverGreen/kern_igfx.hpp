//
//  kern_igfx.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_igfx_hpp
#define kern_igfx_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Library/LegacyIOService.h>

class IGFX {
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
	static IGFX *callbackIGFX;

	/**
	 *  Current graphics kext used for modification
	 */
	KernelPatcher::KextInfo *currentGraphics {nullptr};

	/**
	 *  Current framebuffer kext used for modification
	 */
	KernelPatcher::KextInfo *currentFramebuffer {nullptr};

	/**
	 *  Original PAVP session callback function used for PAVP command handling
	 */
	mach_vm_address_t orgPavpSessionCallback {};

	/**
	 *  Original AppleIntelFramebufferController::ComputeLaneCount function used for DP lane count calculation
	 */
	mach_vm_address_t orgComputeLaneCount {};

	/**
	 *  Original IOService::copyExistingServices function from the kernel
	 */
	mach_vm_address_t orgCopyExistingServices {};

	/**
	 *  Original IntelAccelerator::start function
	 */
	mach_vm_address_t orgAcceleratorStart {};

	/**
	 *  Detected CPU generation of the host system
	 */
	CPUInfo::CpuGeneration cpuGeneration {};

	/**
	 *  Set to true if a black screen ComputeLaneCount patch is required
	 */
	bool blackScreenPatch {false};

	/**
	 *  Set to true if PAVP code should be disabled
	 */
	bool pavpDisablePatch {false};

	/**
	 *  Set to true to disable Metal support
	 */
	bool forceOpenGL {false};

	/**
	 *  Set to true to disable acceleration
	 */
	bool forceVesaMode {false};

	/**
	 *  Set to true if Sandy Bridge Gen6Accelerator should be renamed
	 */
	bool moderniseAccelerator {false};

	/**
	 *  Set to true to avoid incompatible GPU firmware loading
	 */
	bool avoidFirmwareLoading {false};

	/**
	 *  PAVP session callback wrapper used to prevent freezes on incompatible PAVP certificates
	 */
	static IOReturn wrapPavpSessionCallback(void *intelAccelerator, int32_t sessionCommand, uint32_t sessionAppId, uint32_t *a4, bool flag);

	/**
	 *  DP ComputeLaneCount wrapper to report success on non-DP screens to avoid black screen
	 */
	static bool wrapComputeLaneCount(void *that, void *timing, uint32_t bpp, int32_t availableLanes, int32_t *laneCount);

	/**
	 *  copyExistingServices wrapper used to rename Gen6Accelerator from userspace calls
	 */
	static OSObject *wrapCopyExistingServices(OSDictionary *matching, IOOptionBits inState, IOOptionBits options);

	/**
	 *  IntelAccelerator::start wrapper to support vesa mode, force OpenGL, prevent fw loading, etc.
	 */
	static bool wrapAcceleratorStart(IOService *that, IOService *provider);
};

#endif /* kern_igfx_hpp */
