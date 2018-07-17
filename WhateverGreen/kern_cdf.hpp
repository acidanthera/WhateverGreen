//
//  kern_cdf.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_cdf_hpp
#define kern_cdf_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_user.hpp>

class CDF {
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
	static CDF *callbackCDF;

	/**
	 *  Current proc info containing the right path to WindowServer
	 */
	UserPatcher::ProcInfo *currentProcInfo {nullptr};

	/**
	 *  Current binary modification containing the right framework mod
	 */
	UserPatcher::BinaryModInfo *currentModInfo {nullptr};

	/**
	 *  Disable the patches based on -cdfoff boot-arg
	 */
	bool disableHDMI20 = false;
};

#endif /* kern_cdf_hpp */
