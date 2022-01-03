//
//  kern_unfair.hpp
//  WhateverGreen
//
//  Copyright © 2021 vit9696. All rights reserved.
//

#ifndef kern_unfair_hpp
#define kern_unfair_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_user.hpp>

class UNFAIR {
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

private:
	/**
	 *  Private self instance for callbacks
	 */
	static UNFAIR *callbackUNFAIR;

	/**
	 *  Disable unfair, based on mode
	 */
	bool disableUnfair {false};

	/**
	 *  GVA bitmask as specified in unfairgva boot argument / property.
	 */
	enum : uint32_t {
		UnfairAllowHardwareDrmStreamDecoderOnOldCpuid = 1,
		UnfairRelaxHdcpRequirements = 2,
		UnfairCustomAppleGvaBoardId = 4,
		UnfairDyldSharedCache = UnfairRelaxHdcpRequirements | UnfairCustomAppleGvaBoardId,
	};

	/**
	 *  Patch rule bitmask (0 means none).
	 */
	uint32_t unfairGva {0};

	/**
	 *  Codesign page validation wrapper used for userspace patching
	 */
	static void csValidatePage(vnode *vp, memory_object_t pager, memory_object_offset_t page_offset, const void *data, int *validated_p, int *tainted_p, int *nx_p);

	/**
	 * Original codesign page validation pointer.
	 */
	mach_vm_address_t orgCsValidatePage {0};
};

#endif /* kern_unfair_hpp */
