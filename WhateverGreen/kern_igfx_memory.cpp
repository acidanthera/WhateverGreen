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

///
/// This file contains the following memory-related fixes
///
/// 1. DVMT calculation fix on ICL+.
/// 2. Display data buffer early optimizer on ICL+.
///

// MARK: - DVMT Pre-allocated Memory Calculation Fix

void IGFX::DVMTCalcFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
}

void IGFX::DVMTCalcFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the fix if designated boot argument or device property is found
	enabled = checkKernelArgument("-igfxdvmt");
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-dvmt-calc-fix") != nullptr;
	if (!enabled)
		return;
	
	// Guard: Disable the patch if it is not available on the current Intel platforms
	if (!available) {
		SYSLOG("igfx", "DVMT: This fix is not available on the current platform and has been disabled.");
		return;
	}
	
	// Guard: Wait for the device to be published in the plane, otherwise `read` will crash on macOS Big Sur
	if (!WIOKit::awaitPublishing(info->videoBuiltin)) {
		SYSLOG("igfx", "DVMT: Failed to wait for the graphics device to be published. The fix has been disabled.");
		enabled = false;
		return;
	}
	
	// Read the DVMT preallocated memory set in BIOS from the GMCH Graphics Control field at 0x50 (PCI0,2,0)
	auto gms = WIOKit::readPCIConfigValue(info->videoBuiltin, WIOKit::kIOPCIConfigGraphicsControl, 0, 16) >> 8;
	
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
			*reinterpret_cast<uint32_t*>(movl + 2) = dvmt;
			
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

// MARK: - Display Data Buffer Early Optimizer

void IGFX::DisplayDataBufferEarlyOptimizer::wrapGetFeatureControl(IOService *controller) {
	//
	// Abstract
	//
	// Display Data Buffer (DBUF) is critical for display pipes and planes to function properly.
	// The graphics driver allocates the buffer by writing a <start, end> pair to the plane buffer configuration register.
	// Apple expects that the firmware has allocated an adequate amount of buffer for the Pipe A that drives the builtin display,
	// so the driver can optimize the allocation later to provide better display residency in memory low power modes.
	// However, the buffer allocated by the BIOS on Ice Lake-based laptops seems to be not enough for the plane running in the mode configured by the driver,
	// resulting in a garbled display that lasts for about 7 to 15 seconds when the system finishes booting and presents the login window.
	// This issue will disappear when the function that optimizes the buffer allocation is fired by a timer enabled at the end of mode setting.
	// The default delay of executing the optimizer function is 15 seconds which is hard-coded in the framebuffer controller's startup routine.
	// Fortunately, we can change the delay by injecting the property "DBUFOptimizeTime" to the feature control dictionary.
	// By specifying a delay of 0 second, we can invoke the optimizer function as soon as the graphics driver completes the modeset for the builtin display,
	// thus fixing the garbled builtin screen issue on Ice Lake platforms without having any negative impacts on external monitors.
	//
	// Future Work
	//
	// Ideally, we should be able to increase the buffer allocation at an early boot stage,
	// just like how we fix the Core Display Clock issue on Ice Lake platforms.
	// However, I am still trying to figure out where the best place is to inject the code properly.
	//
	// Acknowledgements
	//
	// I would like to acknowledge @m0d16l14n1's passion and insistence on this annoying issue since Sep, 2020,
	// and @kingo123 for implementing the proof-of-concept code showing that @m0d16l14n1's direction is correct.
	// Your findings motivate me to resume this research on the display data buffer issue and find the root cause.
	//
	// - FireWolf
	// - 2021.10
	//
	auto module = &callbackIGFX->modDisplayDataBufferEarlyOptimizer;
	
	do {
		// Guard: Fetch the current feature control dictionary
		auto features = OSDynamicCast(OSDictionary, controller->getProperty(kFeatureControl));
		if (features == nullptr) {
			SYSLOG("igfx", "DBEO: Failed to fetch the feature control dictionary.");
			break;
		}
		
		auto clonedFeatures = features->copyCollection();
		if (clonedFeatures == nullptr) {
			SYSLOG("igfx", "DBEO: Failed to clone the feature control dictionary.");
			break;
		}
		
		auto newFeatures = OSDynamicCast(OSDictionary, clonedFeatures);
		PANIC_COND(newFeatures == nullptr, "igfx", "DBEO: The cloned collection is not a dictionary.");
		
		// Allocate the new optimizer delay
		auto delay = OSNumber::withNumber(module->optimizerTime, 32);
		if (delay == nullptr) {
			SYSLOG("igfx", "DBEO: Failed to allocate the new optimizer delay.");
			newFeatures->release();
			break;
		}

		// Set the new optimizer delay
		newFeatures->setObject(kOptimizerTime, delay);
		controller->setProperty(kFeatureControl, newFeatures);
		delay->release();
		newFeatures->release();
		DBGLOG("igfx", "DBEO: The new optimizer value has been set to %u.", module->optimizerTime);
	} while (false);
	
	module->orgGetFeatureControl(controller);
}

void IGFX::DisplayDataBufferEarlyOptimizer::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
}

void IGFX::DisplayDataBufferEarlyOptimizer::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the fix if the corresponding boot argument is found
	enabled = checkKernelArgument("-igfxdbeo");
	// Of if "enable-dbuf-early-optimizer" is set in IGPU property
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-dbuf-early-optimizer") != nullptr;
	if (!enabled)
		return;
	
	// Fetch the user configuration
	if (WIOKit::getOSDataValue(info->videoBuiltin, "dbuf-optimizer-delay", optimizerTime))
		DBGLOG("igfx", "DBEO: User requested optimizer delay = %u.", optimizerTime);
}

void IGFX::DisplayDataBufferEarlyOptimizer::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest request("__ZN31AppleIntelFramebufferController17getFeatureControlEv", wrapGetFeatureControl, orgGetFeatureControl);
	SYSLOG_COND(!patcher.routeMultiple(index, &request, 1, address, size), "igfx", "DBEO: Failed to route the function.");
}
