//
//  kern_igfx_backlight.cpp
//  WhateverGreen
//
//  Created by FireWolf on 7/29/21.
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "kern_igfx_backlight.hpp"
#include "kern_igfx_kexts.hpp"
#include "kern_igfx.hpp"
#include <Headers/kern_time.hpp>
#include <Headers/kern_disasm.hpp>

///
/// This file contains the following backlight-related fixes and enhancements
///
/// 1. Backlight registers fix that solves the 3-minute black screen on CFL+.
/// 2. Backlight registers alternative fix that revertes the inlined invocation of `hwSetBacklight()` in `LightUpEDP()` and `hwSetPanelPower()`,
///    providing an alternative solution to the 3-minute black screen on CFL.
/// 3. Backlight smoother that makes brightness transitions smoother on IVB+.
///

//
// MARK: - Backlight Registers Fix
//

void IGFX::BacklightRegistersFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingFramebuffer = true;
	
	// We need R/W access to MMIO registers
	requiresMMIORegistersReadAccess = true;
	requiresMMIORegistersWriteAccess = true;
}

void IGFX::BacklightRegistersFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	enabled = checkKernelArgument("-igfxblr");
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-backlight-registers-fix") != nullptr;
	if (!enabled)
		return;
	
	if (WIOKit::getOSDataValue(info->videoBuiltin, "max-backlight-freq", targetBacklightFrequency))
		DBGLOG("igfx", "BLR: Will use the custom backlight frequency %u.", targetBacklightFrequency);
}

void IGFX::BacklightRegistersFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	// Intel backlight is modeled via pulse-width modulation (PWM). See page 144 of:
	// https://01.org/sites/default/files/documentation/intel-gfx-prm-osrc-kbl-vol12-display.pdf
	// Singal-wise it looks as a cycle of signal levels on the timeline:
	// 22111100221111002211110022111100 (4 cycles)
	// 0 - no signal, 1 - no value (no pulse), 2 - pulse (light on)
	// - Physical Cycle (0+1+2) defines maximum backlight frequency, limited by HW precision.
	// - Base Cycle (1+2) defines [1/PWM Base Frequency], limited by physical cycle, see BXT_BLC_PWM_FREQ1.
	// - Duty Cycle (2) defines [1/PWM Increment] - backlight level,
	//   [PWM Frequency Divider] - backlight max, see BXT_BLC_PWM_DUTY1.
	// - Duty Cycle position (first vs last) is [PWM Polarity]
	//
	// Duty cycle = PWM Base Frequeny * (1 / PWM Increment) / PWM Frequency Divider
	//
	// On macOS there are extra limitations:
	// - All values and operations are u32 (32-bit unsigned)
	// - [1/PWM Increment] has 0 to 0xFFFF range
	// - [PWM Frequency Divider] is fixed to be 0xFFFF
	// - [PWM Base Frequency] is capped by 0xFFFF (to avoid u32 wraparound), and is hardcoded
	//   either in Framebuffer data (pre-CFL) or in the code (CFL: 7777 or 22222).
	//
	// On CFL the following patches have to be applied:
	// - Hardcoded [PWM Base Frequency] should be patched or set after the hardcoded value is written by patching
	//   hardcoded frequencies. 65535 is used by default.
	// - If [PWM Base Frequency] is > 65535, to avoid a wraparound code calculating BXT_BLC_PWM_DUTY1
	//   should be replaced to use 64-bit arithmetics.
	// [PWM Base Frequency] can be specified via igfxbklt=1 boot-arg or backlight-base-frequency property.

	// This patch will overwrite WriteRegister32 function to rescale all the register writes of backlight controller.
	// Slightly different methods are used for CFL hardware running on KBL and CFL drivers.
	// Guard: Register injections based on the current framebuffer in use
	auto framebuffer = Value::of(callbackIGFX->getRealFramebuffer(index));
	if (framebuffer.isOneOf(&kextIntelKBLFb)) {
		DBGLOG("igfx", "BLR: [KBL ] Will setup the fix for KBL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dKBLPWMFreq1);
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dKBLPWMCtrl1);
	} else if (framebuffer.isOneOf(&kextIntelCFLFb, &kextIntelICLLPFb)) {
		DBGLOG("igfx", "BLR: [CFL+] Will setup the fix for CFL/ICL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dCFLPWMFreq1);
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dCFLPWMDuty1);
	} else {
		SYSLOG("igfx", "BLR: [ERR!] Found an unsupported platform. Will not perform any injections.");
	}
}

void IGFX::BacklightRegistersFix::wrapKBLWriteRegisterPWMFreq1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_FREQ1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_FREQ1.");
	auto self = &callbackIGFX->modBacklightRegistersFix;
	
	// Preserve the hardware PWM frequency set by the system firmware on boot
	// We'll need this to restore later after system sleep.
	if (self->targetBacklightFrequency == 0) {
		// Guard: The system should be initialized with a non-zero PWM frequency
		if (auto bootValue = callbackIGFX->readRegister32(controller, BXT_BLC_PWM_FREQ1); bootValue != 0) {
			DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = 0x%x.", bootValue);
			DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_CTL1 = 0x%x.",
				   callbackIGFX->readRegister32(controller, BXT_BLC_PWM_CTL1));
			self->targetBacklightFrequency = bootValue;
		} else {
			SYSLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = ZERO. Will use the fallback frequency.");
			self->targetBacklightFrequency = kFallbackTargetBacklightFrequency;
		}
	}

	// For the KBL driver, 0xc8254 (BLC_PWM_PCH_CTL2) controls the backlight intensity.
	// High 16 of this write are the denominator (frequency), low 16 are the numerator (duty cycle).
	// Translate this into a write to c8258 (BXT_BLC_PWM_DUTY1) for the CFL hardware, scaled by the system-provided value in c8254 (BXT_BLC_PWM_FREQ1).
	uint16_t frequency = (value & 0xffff0000U) >> 16U;
	uint16_t dutyCycle = value & 0xffffU;

	uint32_t rescaledValue = frequency == 0 ? 0 : static_cast<uint32_t>((dutyCycle * static_cast<uint64_t>(self->targetBacklightFrequency)) / static_cast<uint64_t>(frequency));
	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: Write PWM_DUTY1 0x%x/0x%x, rescaled to 0x%x/0x%x.",
		   dutyCycle, frequency, rescaledValue, self->targetBacklightFrequency);

	// Reset the hardware PWM frequency. Write the original system value if the driver-requested value is nonzero. If the driver requests
	// zero, we allow that, since it's trying to turn off the backlight PWM for sleep.
	callbackIGFX->writeRegister32(controller, BXT_BLC_PWM_FREQ1, frequency ? self->targetBacklightFrequency : 0);

	// Finish by writing the duty cycle.
	if (callbackIGFX->modBacklightSmoother.enabled) {
		// Need to pass the scaled value to the smoother
		DBGLOG("igfx", "BLS: [KBL ] Will pass the rescaled value 0x%08x to the smoother version.", rescaledValue);
		IGFX::BacklightSmoother::smoothCFLWriteRegisterPWMDuty1(controller, BXT_BLC_PWM_DUTY1, rescaledValue);
	} else {
		// Otherwise invoke the original function
		DBGLOG("igfx", "BLR: [KBL ] Will pass the rescaled value 0x%08x to the original version.", rescaledValue);
		callbackIGFX->writeRegister32(controller, BXT_BLC_PWM_DUTY1, rescaledValue);
	}
}

void IGFX::BacklightRegistersFix::wrapKBLWriteRegisterPWMCtrl1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_CTL1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_CTL1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_CTL1.");
	auto self = &callbackIGFX->modBacklightRegistersFix;
	
	if (self->targetPwmControl == 0) {
		// Save the original hardware PWM control value
		self->targetPwmControl = callbackIGFX->readRegister32(controller, BXT_BLC_PWM_CTL1);
	}

	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_CTL1>: Write BXT_BLC_PWM_CTL1 0x%x, previous was 0x%x.",
		   value, callbackIGFX->readRegister32(controller, BXT_BLC_PWM_CTL1));

	if (value) {
		// Set the PWM frequency before turning it on to avoid the 3 minute blackout bug
		callbackIGFX->writeRegister32(controller, BXT_BLC_PWM_FREQ1, self->targetBacklightFrequency);

		// Use the original hardware PWM control value.
		value = self->targetPwmControl;
	}
	
	// Finish by writing the new value
	callbackIGFX->writeRegister32(controller, reg, value);
}

void IGFX::BacklightRegistersFix::wrapCFLWriteRegisterPWMFreq1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_FREQ1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_FREQ1.");
	auto self = &callbackIGFX->modBacklightRegistersFix;
	
	if (value && value != self->driverBacklightFrequency) {
		DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: Driver requested BXT_BLC_PWM_FREQ1 = 0x%x.", value);
		self->driverBacklightFrequency = value;
	}

	// Preserve the hardware PWM frequency set by the system firmware on boot
	// We'll need this to restore later after system sleep.
	if (self->targetBacklightFrequency == 0) {
		// Guard: The system should be initialized with a non-zero PWM frequency
		if (auto bootValue = callbackIGFX->readRegister32(controller, BXT_BLC_PWM_FREQ1); bootValue != 0) {
			DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = 0x%x.", bootValue);
			self->targetBacklightFrequency = bootValue;
		} else {
			SYSLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = ZERO. Will use the fallback frequency.");
			self->targetBacklightFrequency = kFallbackTargetBacklightFrequency;
		}
	}

	if (value) {
		// Nonzero writes to this register need to use the original system value.
		// Yet the driver can safely write zero to this register as part of system sleep.
		value = self->targetBacklightFrequency;
	}
	
	// Finish by writing the new value
	callbackIGFX->writeRegister32(controller, reg, value);
}

void IGFX::BacklightRegistersFix::wrapCFLWriteRegisterPWMDuty1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_DUTY1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_DUTY1.");
	auto self = &callbackIGFX->modBacklightRegistersFix;
	
	if (value && self->driverBacklightFrequency == 0) {
		// CFL+ backlight additional fix.
		DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Backlight additional fix was entered.");
		uint32_t registerValue = callbackIGFX->readRegister32(controller, SFUSE_STRAP);
		uint32_t selectedFreq = (registerValue & SFUSE_STRAP_RAW_FREQUENCY) ? ICL_FREQ_RAW : ICL_FREQ_NORMAL;
		wrapCFLWriteRegisterPWMFreq1(controller, BXT_BLC_PWM_FREQ1, selectedFreq);
	}

	if (self->driverBacklightFrequency && self->targetBacklightFrequency) {
		// Translate the PWM duty cycle between the driver scale value and the HW scale value
		uint32_t rescaledValue = static_cast<uint32_t>((value * static_cast<uint64_t>(self->targetBacklightFrequency)) / static_cast<uint64_t>(self->driverBacklightFrequency));
		DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Write PWM_DUTY1 0x%x/0x%x, rescaled to 0x%x/0x%x.",
			   value, self->driverBacklightFrequency, rescaledValue, self->targetBacklightFrequency);
		value = rescaledValue;
	} else {
		// This should never happen, but in case it does we should log it at the very least.
		SYSLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Write PWM_DUTY1 has zero frequency driver (%d) target (%d).",
			   self->driverBacklightFrequency, self->targetBacklightFrequency);
	}
	
	if (callbackIGFX->modBacklightSmoother.enabled) {
		// Need to pass the scaled value to the smoother
		DBGLOG("igfx", "BLS: [CFL+] Will pass the rescaled value 0x%08x to the smoother version.", value);
		IGFX::BacklightSmoother::smoothCFLWriteRegisterPWMDuty1(controller, reg, value);
	} else {
		// Otherwise invoke the original function
		DBGLOG("igfx", "BLR: [CFL+] Will pass the rescaled value 0x%08x to the original version.", value);
		callbackIGFX->writeRegister32(controller, reg, value);
	}
}

//
// MARK: - Backlight Registers Alternative Fix
//

void IGFX::BacklightRegistersAltFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingFramebuffer = true;
	
	// We need R/W access to MMIO registers
	requiresMMIORegistersReadAccess = true;
	requiresMMIORegistersWriteAccess = true;
}

void IGFX::BacklightRegistersAltFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	enabled = checkKernelArgument("-igfxblt");
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-backlight-registers-alternative-fix") != nullptr;
}

void IGFX::BacklightRegistersAltFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	//
	// Apple has "accidentally" simplified the implementation of the functions, `ReadRegister32` and `WriteRegister32`, in Coffee Lake's framebuffer driver shipped by macOS 13.4,
	// so the compiler chose to inline invocations of those functions as many as possible. As a result, `hwSetBacklight()` no longer invokes
	//  `WriteRegister32` to update backlight registers; instead it modifies the register value via mapped memory directly, making itself an inline helper.
	// `LightUpEDP()` and `hwSetPanelPower()` that invoke `hwSetBacklight()` now have the definition of `hwSetBacklight()` embedded in themselves.
	// As such, the `WriteRegister32` hooks registered by the Backlight Registers Fix (BLR) and the Backlight Smoother (BLS) submodules no longer work.
	// This patch submodule (BLT) reverts the optimizations done by the compiler in aforementioned three functions and overwrites Apple's implementation of `hwSetBacklight()`,
	// thus providing an alternative to BLR and making BLS work properly on macOS 13.4 or later.
	//
	// - FireWolf
	// - 2023.06
	//
	static constexpr const char* kHwSetBacklightSymbol = "__ZN31AppleIntelFramebufferController14hwSetBacklightEj";
	
	// Guard: Verify the kernel major version
	if (getKernelVersion() < KernelVersion::Ventura) {
		SYSLOG("igfx", "BLT: [COMM] Aborted: This patch submodule is only available as of macOS 13.4.");
		return;
	} else if (getKernelVersion() == KernelVersion::Ventura && getKernelMinorVersion() < 5) {
		SYSLOG("igfx", "BLT: [COMM] Aborted: This patch submodule is only available as of macOS 13.4.");
		return;
	} else {
		DBGLOG("igfx", "BLT: [COMM] Running on Darwin %d.%d. Assuming that BLT is compatible with the current macOS release.",
			   getKernelVersion(), getKernelMinorVersion());
	}
	
	// Guard: Verify the current framebuffer driver to select the concrete submodule
	auto framebuffer = callbackIGFX->getRealFramebuffer(index);
	IGFX::BacklightRegistersAltFix *self = nullptr;
	if (framebuffer == &kextIntelKBLFb) {
		DBGLOG("igfx", "BLT: [COMM] Will set up the fix for the KBL platform.");
		self = &callbackIGFX->modBacklightRegistersAltFixKBL;
	} else if (framebuffer == &kextIntelCFLFb) {
		DBGLOG("igfx", "BLT: [COMM] Will set up the fix for the CFL platform.");
		self = &callbackIGFX->modBacklightRegistersAltFixCFL;
	} else {
		SYSLOG("igfx", "BLT: [COMM] Aborted: Only KBL and CFL platforms are supported.");
		return;
	}

	// Guard: Resolve the address of `hwSetBacklight()`
	auto orgHwSetBacklight = patcher.solveSymbol(index, kHwSetBacklightSymbol, address, size);
	if (orgHwSetBacklight == 0) {
		SYSLOG("igfx", "BLT: [COMM] Error: Unable to find the address of hwSetBacklight().");
		patcher.clearError();
		return;
	}
	
	// Guard: Analyze `hwSetBacklight()` to find the offset of each required member field in the framebuffer controller
	auto probeContext = self->probeMemberOffsets(orgHwSetBacklight, kMaxNumInstructions);
	if (!probeContext.isValid()) {
		SYSLOG("igfx", "BLT: [COMM] Error: Failed to find the offset of one of the required member field.");
		return;
	}
	
	// Analyze and patch each function that contains an inlined invocation of `hwSetBacklight()`
	for (auto descriptor = self->getFunctionDescriptors(); descriptor->name != nullptr; descriptor += 1) {
		// Guard: Resolve the symbol of the current function
		descriptor->address = patcher.solveSymbol(index, descriptor->symbol, address, size);
		if (descriptor->address == 0) {
			SYSLOG("igfx", "BLT: [COMM] Error: Failed to find the address of %s().", descriptor->name);
			patcher.clearError();
			return;
		}
		
		// Guard: Identify the location of the inlined invocation and the register that stores the controller instance
		auto invocationContext = descriptor->probe(probeContext);
		if (!invocationContext.isValid()) {
			SYSLOG("igfx", "BLT: [COMM] Error: Unable to find the position of the inlined invocation of hwSetBacklight() in %s().", descriptor->name);
			return;
		}
		
		// Guard: Patch the function to invoke `hwSetBacklight()` explicitly
		if (!descriptor->revert(probeContext, invocationContext, patcher, orgHwSetBacklight)) {
			SYSLOG("igfx", "BLT: [COMM] Error: Failed to patch the function %s().", descriptor->name);
			return;
		} else {
			DBGLOG("igfx", "BLT: [COMM] Reverted the inlined invocation of hwSetBacklight() in %s() sucessfully.", descriptor->name);
		}
	}

	// Guard: Replace the implementation of `hwSetBacklight()`
	KernelPatcher::RouteRequest request(kHwSetBacklightSymbol, self->getHwSetBacklightWrapper());
	SYSLOG_COND(!patcher.routeMultiple(index, &request, 1, address, size), "igfx", "BLT: [COMM] Error: Failed to route hwSetBacklight().");
	
	// Save the probe context which is needed by the hwSetBacklight wrapper
	self->probeContext = probeContext;
}

bool IGFX::BacklightRegistersAltFix::revertInlinedInvocation(const FunctionDescriptor &descriptor,
															   const ProbeContext &probeContext,
															   const InvocationContext &invocationContext,
															   KernelPatcher &patcher,
															   mach_vm_address_t orgHwSetBacklight) {
	//
	// Sample Patch 1: Revert inlined hwSetBacklight() invocation
	//
	// Function: AppleIntelFramebufferController::LightUpEDP()
	//   Offset: 488
	//  Version: macOS 13.4
	// Platform: CFL
	//
	//     Find: movq  0x2e60(%r15), %rdi // Beginning of the inlined function call
	//           testq %rdi, %rdi         // %r15 stores the implicit controller instance
	//           je    loc_146f78b6
	//           movl  0x2e78(%r15), %esi
	//
	//  Replace: movl 0x2e78(%r15), %esi  // Fetch the target backlight level which is the 2nd argument
	//           movq %r15, %rdi          // The implicit controller instance is the 1st argument
	//           call 0x146ee4ae          // Call AppleIntelFramebufferController::hwSetBacklight(level)
	//           jmp 0x146f7920           // Jump to the end of inlined function call
	//
	// ----------------------------------------------------------------------------------------------------
	//
	// Sample Patch 2: Revert inlined hwSetBacklight() invocation
	//
	// Function: AppleIntelFramebufferController::hwSetPanelPower()
	//   Offset: 1505
	//  Version: macOS 13.4
	// Platform: CFL
	//
	//     Find: leal 0xfff37da7(%rax), %edx // Beginning of the inlined function call
	//           cmpl $0xfff37daa, %edx      // %r12 stores the implicit controller instance
	//           ja   loc_146ea9e5           // %rcx stores the base address of the MMIO region
	//           movl 0x2e84(%r12), %eax
	//
	//  Replace: pushq %rcx                  // Preserve the base address of the MMIO region
	//           movl 0x2e78(%r12), %esi     // Fetch the target backlight level which is the 2nd argument
	//           movq %r12, %rdi             // The implicit controller instance is the 1st argument
	//           call 0x146ee4ae             // Call AppleIntelFramebufferController::hwSetBacklight(level)
	//           popq %rcx                   // Restore the base address of the MMIO region
	//           jmp 0x146eaa1c              // Jump to the end of inlined function call
	//
	// Step 1: [Template] Preserve all caller-saved registers to avoid analyzing register liveness
	static constexpr uint8_t kPreserve[] = {
		0x50,           // pushq %rax
		0x57,           // pushq %rdi
		0x56,           // pushq %rsi
		0x52,           // pushq %rdx
		0x51,           // pushq %rcx
		0x41, 0x50,     // pushq %r8
		0x41, 0x51,     // pushq %r9
		0x41, 0x52,     // pushq %r10
		0x41, 0x53      // pushq %r11
	};
	
	// Step 2: [Template] Fetch the new brightness level which will be the 2nd argument
	static constexpr uint8_t kSetArg1[] = {
		//
		// Template: movl 0x???(%r8), %esi
		//
		// Notes:
		//  - Start from 0x8B if the source register is < %r8
		//  - Start from 0x41 if the source register is >= %r8 and is NOT %r12
		//  - Increment 0xB0 by `index` where `index` = register % 8
		//  - Using %rsp or %rbp as the source register should never happen
		//
		0x41, 0x8B, 0xB0, 0x00, 0x00, 0x00, 0x00
	};
	static constexpr uint8_t kSetArg1_r12[] = {
		//
		// Template: movl 0x???(%r12), %esi
		//
		// Notes:
		//  - 0x24 is inserted after 0xB0 if the source register is %r12
		//
		0x41, 0x8B, 0xB4, 0x24, 0x00, 0x00, 0x00, 0x00
	};
	static constexpr uint8_t kSetArg1_Zero[] = {
		//
		// Template: xorl %esi, %esi
		//
		0x31, 0xF6
	};
	
	// Step 3: [Template] Set the controller instance which will be the 1st argument
	static constexpr uint8_t kSetArg0[] = {
		//
		// Template: movq %rax, %rdi
		//
		// Notes:
		//  - Use 0x4C instead of 0x48 if the source register is >= %r8
		//  - Increment 0xC7 by `index` * 8, where `index` = register % 8
		//
		0x48, 0x89, 0xC7
	};
	
	// Step 4: [Template] Invoke `AppleIntelFramebufferController::hwSetBacklight(level)`
	static constexpr uint8_t kCall[] = {
		//
		// Template: call <offset?>
		//
		// Notes:
		//  - The offset is relative to the address of the next instruction
		//
		0xE8, 0x00, 0x00, 0x00, 0x00
	};
	
	// Step 5: [Template] Restore all caller-saved registers
	static constexpr uint8_t kRestore[] = {
		0x41, 0x5B,     // popq %r11
		0x41, 0x5A,     // popq %r10
		0x41, 0x59,     // popq %r9
		0x41, 0x58,     // popq %r8
		0x59,           // popq %rcx
		0x5A,           // popq %rdx
		0x5E,           // popq %rsi
		0x5F,           // popq %rdi
		0x58            // popq %rax
	};
	
	// Step 6: [Template] Jump to the end of the inlined invocation of `hwSetBacklight()`
	static constexpr uint8_t kJump[] = {
		//
		// Template: jmp 0x?? (8-bit offset)
		//
		// Notes:
		//  - The offset is relative to the address of the next instruction
		//  - The offset should not exceed 0xFF since `hwSetBacklight` is small enough to be inlined
		//
		0xEB, 0x00
	};
	
	// The maximum number of bytes in the final patch
	static constexpr size_t kMaxPatchSize = sizeof(kPreserve) + sizeof(kSetArg1_r12) + sizeof(kSetArg0) + sizeof(kCall) + sizeof(kRestore) + sizeof(kJump);
	
	// Guard: Ensure that there is enough space to revert the inlined invocation
	if (auto freeSpace = invocationContext.freeSpace(); freeSpace < kMaxPatchSize) {
		SYSLOG("igfx", "BLT: [COMM] Error: %s() does not have enough space to revert the inlined invocation. Required = %zu; Free = %zu.",
			   descriptor.name, kMaxPatchSize, freeSpace);
		return false;
	}
	
	// Build the patch step by step
	uint8_t patch[kMaxPatchSize] = {};
	uint8_t* current = patch;
	DBGLOG("igfx", "BLT: [COMM] Building the assembly patch for %s() at 0x%016llx to revert the inlined invocation of hwSetBacklight().", descriptor.name, descriptor.address);
	DBGLOG("igfx", "BLT: [COMM] Invocation context: Start Offset = %zu, End Offset = %zu, Free Space = %zu Bytes, Framebuffer controller stored in register %s.",
		   invocationContext.start, invocationContext.end, invocationContext.freeSpace(), registerName(invocationContext.registerController));
	
	// Step 1: Preserve all caller-saved registers to avoid analyzing register liveness
	lilu_os_memcpy(current, kPreserve, sizeof(kPreserve));
	current += sizeof(kPreserve);
	
	// Step 2: Fetch the new brightness level which will be the 2nd argument
	if (descriptor.useCurrentBrightnessLevel) {
		// Step 2.1: Select the instruction based upon the register that stores the controller instance
		if (invocationContext.registerController < 8) {
			// %rax, %rcx, %rdx, %rbx, /* %rsp, %rbp */, %rsi, %rdi
			lilu_os_memcpy(current, kSetArg1 + 1, sizeof(kSetArg1) - 1);
			current[1] += invocationContext.registerController % 8;
			current += sizeof(kSetArg1) - 1;
		} else if (invocationContext.registerController != 12) {
			// %r8, %r9, %r10, %r11, %r13, %r14, %r15
			lilu_os_memcpy(current, kSetArg1, sizeof(kSetArg1));
			current[2] += invocationContext.registerController % 8;
			current += sizeof(kSetArg1);
		} else {
			// %r12
			lilu_os_memcpy(current, kSetArg1_r12, sizeof(kSetArg1_r12));
			current += sizeof(kSetArg1_r12);
		}
		// Step 2.2: Set the offset of the member field that stores the new brightness level
		*reinterpret_cast<uint32_t*>(current - sizeof(uint32_t)) = static_cast<uint32_t>(probeContext.offsetBrightnessLevel);
		DBGLOG("igfx", "BLT: [COMM] Patched %s() to invoke hwSetBacklight() with the current brightness level.", descriptor.name);
	} else {
		lilu_os_memcpy(current, kSetArg1_Zero, sizeof(kSetArg1_Zero));
		current += sizeof(kSetArg1_Zero);
		DBGLOG("igfx", "BLT: [COMM] Patched %s() to invoke hwSetBacklight() with a brightness level of zero.", descriptor.name);
	}
	
	// Step 3: Set the controller instance which will be the 1st argument
	lilu_os_memcpy(current, kSetArg0, sizeof(kSetArg0));
	current[2] += (invocationContext.registerController % 8) * 8;
	if (invocationContext.registerController >= 8) {
		// %r8, %r9, %r10, %r11, %r12, %r13, %r14, %r15
		current[0] += 4;
	}
	current += sizeof(kSetArg0);
	
	// Step 4: Invoke `AppleIntelFramebufferController::hwSetBacklight(level)`
	// Step 4.1: Copy the instruction
	lilu_os_memcpy(current, kCall, sizeof(kCall));
	current += sizeof(kCall);
	// Step 4.2: Calculate the address of the next instruction (after `call` returns)
	mach_vm_address_t next = descriptor.address + invocationContext.start + (current - patch);
	// Step 4.3: Calculate and set the offset for the call instruction
	*reinterpret_cast<uint32_t*>(current - sizeof(uint32_t)) = static_cast<uint32_t>(orgHwSetBacklight - next);
	
	// Step 5: Restore all caller-saved registers
	lilu_os_memcpy(current, kRestore, sizeof(kRestore));
	current += sizeof(kRestore);
	
	// Step 6: Jump to the first instruction after the inlined invocation of `hwSetBacklight()` returns
	// Step 6.1: Copy the instruction
	lilu_os_memcpy(current, kJump, sizeof(kJump));
	current += sizeof(kJump);
	// Step 6.2: Calculate and set the offset for the jump instruction
	size_t offset = invocationContext.end - (invocationContext.start + current - patch);
	PANIC_COND(offset > UINT8_MAX, "igfx", "BLT: [COMM] The offset for the jump instruction exceeds 255, which should not happen.");
	*(current - sizeof(uint8_t)) = static_cast<uint8_t>(offset);
	
	// The patch has been built for the given function
	size_t patchSize = current - patch;
	DBGLOG("igfx", "BLT: [COMM] Built the assembly patch (%zu bytes) for %s() at 0x%016llx: ", patchSize, descriptor.name, descriptor.address);
	for (size_t index = 0; index < patchSize; index += 1) {
		DBGLOG("igfx", "BLT: [COMM] [%04lu] 0x%02x", index, patch[index]);
	}
	
	// Guard: Apply the assembly patch
	if (patcher.routeBlock(descriptor.address + invocationContext.start, patch, patchSize) != 0) {
		SYSLOG("igfx", "BLT: [COMM] Failed to apply the assembly patch to %s() at 0x%016llx.", descriptor.name, descriptor.address);
		patcher.clearError();
		return false;
	}
	
	// All done
	DBGLOG("igfx", "BLT: [COMM] Patched %s() at 0x%016llx successfully.", descriptor.name, descriptor.address);
	return true;
}

/**
 *  Analyze `hwSetBacklight()` to find the offset of each required member field in the framebuffer controller
 *
 *  @param address The address of `hwSetBacklight()` to be analyzed
 *  @param instructions The maximum number of instructions to be analyzed
 *  @return A context object that stores the offset of each required member field in the framebuffer controller
 */
IGFX::BacklightRegistersAltFixKBL::ProbeContext IGFX::BacklightRegistersAltFixKBL::probeMemberOffsets(mach_vm_address_t address, size_t instructions) const {
	DBGLOG("igfx", "BLT: [KBL ] Analyzing the function at 0x%016llx to probe the offset of each required member field.", address);
	hde64s handle;
	
	// Record which register stores the implicit controller instance
	// By default, %rdi stores the implicit controller instance (i.e., the 1st argument)
	uint32_t registerController = 7;
	
	// Record which register stores the given brightness level
	// By default, %esi stores the given brightness level (i.e., the 2nd argument)
	uint32_t registerBrightnessLevel = 6;
	
	// Record the offset of the member field that stores the frequency divider
	size_t offsetFrequencyDivider = 0;
	
	// Record the offset of the member field that will store the given brightness level
	size_t offsetBrightnessLevel = 0;
	
	// Analyze at most the given number instructions to find the offsets
	for (size_t index = 0; index < instructions; index += 1) {
		// Guard: Should be able to disassemble the current instruction
		address += Disassembler::hdeDisasm(address, &handle);
		if (handle.flags & F_ERROR) {
			SYSLOG("igfx", "BLT: [KBL ] Error: Cannot disassemble the instruction.");
			break;
		}
		
		// Guard: Step 1: Identify which register stores the controller instance
		// Pattern: movq %rdi, %r??
		// Checks: MOV, 64-bit, Direct Mode, Source Register is %rdi
		if (Patterns::movqArg0(handle)) {
			registerController = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movq: Register %s now stores the controller instance.", registerName(registerController));
			continue;
		}
		
		// Guard: Step 2: Identify which register stores the given brightness level
		// Pattern: movl %esi, %r??
		// Checks: MOV, 32-bit, Direct Mode, Source Register is %esi
		if (Patterns::movlArg1(handle)) {
			registerBrightnessLevel = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movl: Register %s now stores the new brightness level.", registerName(registerBrightnessLevel));
			continue;
		}
		
		// Guard: Step 3: Identify the offset of the member field in the controller that stores the frequency divider
		// Pattern: movl <offset?>(%r??), %r??
		// Checks: MOV, 32-bit, Memory Mode, Source register is identical to the one that stores the controller
		if (Patterns::movlFromMemory(handle, registerController)) {
			offsetFrequencyDivider = handle.disp.disp32;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movl: The frequency divider is stored at offset 0x%zx.", offsetFrequencyDivider);
			continue;
		}
		
		// Guard: Step 4: Identify the offset of the member field in the controller that will store the given brightness level
		// Pattern: movl %r??, <offset?>(%r??)
		// Checks: MOV, 32-bit, Memory Mode,
		//         Source register is identical to the one that stores the brightness level,
		//         Destination register is identical to the one that stores the controller
		if (Patterns::movlToMemory(handle, registerBrightnessLevel, registerController)) {
			offsetBrightnessLevel = handle.disp.disp32;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movl: The brightness level is stored at offset 0x%zx.", offsetBrightnessLevel);
			break;
		}
	}
	
	// All done
	return {offsetFrequencyDivider, offsetBrightnessLevel};
}

/**
 *  Get all functions that contain an inlined invocation of `hwSetBacklight()` and thus will be analyzed and patched
 *
 *  @return A null-terminated array of descriptor, each of describes a function to be analyzed and patched.
 */
IGFX::BacklightRegistersAltFixKBL::FunctionDescriptor* IGFX::BacklightRegistersAltFixKBL::getFunctionDescriptors() const {
	static constexpr const char* kLightUpEDPSymbol = "__ZN31AppleIntelFramebufferController10LightUpEDPEP21AppleIntelFramebufferP21AppleIntelDisplayPathPK29IODetailedTimingInformationV2";
	static constexpr const char* kDisableDisplaySymbol = "__ZN31AppleIntelFramebufferController14DisableDisplayEP21AppleIntelFramebufferP21AppleIntelDisplayPathbh";
	static constexpr const char* kHwSetPanelPowerSymbol = "__ZN31AppleIntelFramebufferController15hwSetPanelPowerEj";
	
	static FunctionDescriptor kFunctionDescriptors[] = {
		{
			"AppleIntelFramebufferController::LightUpEDP",
			kLightUpEDPSymbol,
			0,
			IGFX::BacklightRegistersAltFixKBL::probeInlinedInvocation_LightUpEDP,
			IGFX::BacklightRegistersAltFixKBL::revertInlinedInvocation,
			true,
		},
		{
			"AppleIntelFramebufferController::DisableDisplay",
			kDisableDisplaySymbol,
			0,
			IGFX::BacklightRegistersAltFixKBL::probeInlinedInvocation_DisableDisplay,
			IGFX::BacklightRegistersAltFixKBL::revertInlinedInvocation,
			false,
		},
		{
			"AppleIntelFramebufferController::hwSetPanelPower",
			kHwSetPanelPowerSymbol,
			0,
			IGFX::BacklightRegistersAltFixKBL::probeInlinedInvocation_HwSetPanelPower,
			IGFX::BacklightRegistersAltFixKBL::revertInlinedInvocation,
			true,
		},
		{
			nullptr,
			nullptr,
			0,
			nullptr,
			nullptr,
			false,
		}
	};
	
	return kFunctionDescriptors;
}

/**
 *  Get the address of the wrapper function that sets the backlight compatible with the current machine
 *
 *  @return The address of the wrapper function.
 */
mach_vm_address_t IGFX::BacklightRegistersAltFixKBL::getHwSetBacklightWrapper() const {
	return reinterpret_cast<mach_vm_address_t>(&IGFX::BacklightRegistersAltFixKBL::wrapHwSetBacklight);
}

/**
 *  Find the location of an inlined invocation of `hwSetBacklight()` in the given function
 *
 *  @param descriptor A descriptor that describes the function to be analyzed
 *  @param probeContext A context object that stores the offset of each required member field in the framebuffer controller
 *  @return A context object that stores a pair of `<Start, End>` offsets, relative to the given address,
 *          which indicates the location of an inlined invocation of `hwSetBacklight()` in this function,
 *          and the register that stores the implicit framebuffer controller instance.
 *  @note This function is specialized for `LightUpEDP()`.
 */
IGFX::BacklightRegistersAltFixKBL::InvocationContext IGFX::BacklightRegistersAltFixKBL::probeInlinedInvocation_LightUpEDP(const FunctionDescriptor &descriptor, const ProbeContext &probeContext) {
	DBGLOG("igfx", "BLT: [KBL ] Analyzing %s() at 0x%016llx to identify the position of the inlined invocation of hwSetBacklight().", descriptor.name, descriptor.address);
	hde64s handle;
	
	// The address of the current instruction
	auto current = descriptor.address;
	
	// The context of the inlined invocation
	InvocationContext context;
	
	// Analyze at most the given number instructions to find the location of inlined invocation of `hwSetBacklight()`
	for (size_t index = 0; index < kMaxNumInstructions; index += 1) {
		// Guard: Should be able to disassemble the current instruction
		size_t size = Disassembler::hdeDisasm(current, &handle);
		if (handle.flags & F_ERROR) {
			SYSLOG("igfx", "BLT: [KBL ] Error: Cannot disassemble the instruction at 0x%016llx.", current);
			break;
		}
		
		// Guard: Step 1: Find the start address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Pattern: leal 0xfff37da7(%r??), %r?? where the source register stores the base address of the MMIO region
		// Note that the start address found in `LightUpEDP()` is after the invocation of `CamelliaBase::SetDPCDBacklight()`
		// and the retrieval of the base address of the MMIO region
		if (Patterns::lealWithOffset(handle, 0xFFF37DA7)) {
			context.start = current - descriptor.address;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction leal: The relative start address of the inlined invocation is 0x%zx.", context.start);
			current += size;
			continue;
		}
		
		// Guard: Step 2: Identify which register stores the controller instance
		// Pattern: movl <offset>(%r??), %r??
		// where the offset is identical to the one found in `hwSetBacklight()`
		if (Patterns::movlFromMemoryWithOffset(handle, probeContext.offsetFrequencyDivider)) {
			context.registerController = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movl: Register %s stores the controller instance.", registerName(context.registerController));
			current += size;
			continue;
		}
		
		// Guard: Step 3: Find the end address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Pattern: movl $??????????, 0xc8250(%r??)
		if (Patterns::movlImm32ToMemoryWithOffset(handle, 0xC8250)) {
			context.end = current + size - descriptor.address;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movl: The relative end address of the inlined invocation is 0x%zx.", context.end);
			break;
		}
		
		current += size;
	}
	
	// All done
	return context;
}

/**
 *  Find the location of an inlined invocation of `hwSetBacklight()` in the given function
 *
 *  @param descriptor A descriptor that describes the function to be analyzed
 *  @param probeContext A context object that stores the offset of each required member field in the framebuffer controller
 *  @return A context object that stores a pair of `<Start, End>` offsets, relative to the given address,
 *          which indicates the location of an inlined invocation of `hwSetBacklight()` in this function,
 *          and the register that stores the implicit framebuffer controller instance.
 *  @note This function is specialized for `DisableDisplay()`.
 */
IGFX::BacklightRegistersAltFixKBL::InvocationContext IGFX::BacklightRegistersAltFixKBL::probeInlinedInvocation_DisableDisplay(const FunctionDescriptor &descriptor, const ProbeContext &probeContext) {
	DBGLOG("igfx", "BLT: [KBL ] Analyzing %s() at 0x%016llx to identify the position of the inlined invocation of hwSetBacklight().", descriptor.name, descriptor.address);
	hde64s handle;
	
	// The address of the current instruction
	auto current = descriptor.address;
	
	// The context of the inlined invocation
	InvocationContext context;
	
	// Analyze at most the given number instructions to find the location of inlined invocation of `hwSetBacklight()`
	for (size_t index = 0; index < kMaxNumInstructions; index += 1) {
		// Guard: Should be able to disassemble the current instruction
		size_t size = Disassembler::hdeDisasm(current, &handle);
		if (handle.flags & F_ERROR) {
			SYSLOG("igfx", "BLT: [KBL ] Error: Cannot disassemble the instruction at 0x%016llx.", current);
			break;
		}
		
		// Guard: Step 1: Identify which register stores the controller instance
		// Pattern: movq %rdi, %r??
		if (Patterns::movqArg0(handle)) {
			context.registerController = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movq: Register %s stores the controller instance.", registerName(context.registerController));
			current += size;
			continue;
		}
		
		// Guard: Step 2: Find the start address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Pattern: leal 0xfff37da7(%r??), %r?? where the source register stores the base address of the MMIO region
		// Note that the start address found in `LightUpEDP()` is after the invocation of `CamelliaBase::SetDPCDBacklight()`
		// and the retrieval of the base address of the MMIO region
		if (Patterns::lealWithOffset(handle, 0xFFF37DA7)) {
			context.start = current - descriptor.address;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction leal: The relative start address of the inlined invocation is 0x%zx.", context.start);
			current += size;
			continue;
		}
		
		// Guard: Step 3: Find the end address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Pattern: movl $??????????, 0xc8250(%r??)
		if (Patterns::movlImm32ToMemoryWithOffset(handle, 0xC8250)) {
			context.end = current + size - descriptor.address;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movl: The relative end address of the inlined invocation is 0x%zx.", context.end);
			break;
		}
		
		current += size;
	}
	
	// All done
	return context;
}

/**
 *  Find the location of an inlined invocation of `hwSetBacklight()` in the given function
 *
 *  @param descriptor A descriptor that describes the function to be analyzed
 *  @param probeContext A context object that stores the offset of each required member field in the framebuffer controller
 *  @return A context object that stores a pair of `<Start, End>` offsets, relative to the given address,
 *          which indicates the location of an inlined invocation of `hwSetBacklight()` in this function,
 *          and the register that stores the implicit framebuffer controller instance.
 *  @note This function is specialized for `hwSetPanelPower()`.
 */
IGFX::BacklightRegistersAltFixKBL::InvocationContext IGFX::BacklightRegistersAltFixKBL::probeInlinedInvocation_HwSetPanelPower(const FunctionDescriptor &descriptor, const ProbeContext &probeContext) {
	DBGLOG("igfx", "BLT: [KBL ] Analyzing %s() at 0x%016llx to identify the position of the inlined invocation of hwSetBacklight().", descriptor.name, descriptor.address);
	hde64s handle;
	
	// The address of the current instruction
	auto current = descriptor.address;
	
	// The context of the inlined invocation
	InvocationContext context;
	
	// Analyze at most the given number instructions to find the location of inlined invocation of `hwSetBacklight()`
	for (size_t index = 0; index < kMaxNumInstructions; index += 1) {
		// Guard: Should be able to disassemble the current instruction
		size_t size = Disassembler::hdeDisasm(current, &handle);
		if (handle.flags & F_ERROR) {
			SYSLOG("igfx", "BLT: [KBL ] Error: Cannot disassemble the instruction at 0x%016llx.", current);
			break;
		}
		
		// Guard: Step 1: Identify which register stores the controller instance
		// Pattern: movq %rdi, %r??
		if (Patterns::movqArg0(handle)) {
			context.registerController = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movq: Register %s now stores the controller instance.", registerName(context.registerController));
			current += size;
			continue;
		}
		
		// Guard: Step 2: Find the start address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Unlike the Coffee Lake driver, the Kaby Lake driver writes to the register 0xC8250 without calling hwSetBacklight().
		// However, we will use our custom implementation of hwSetBacklight() which updates both the backlight and the register 0xC8250.
		// Pattern: addl $0xfff37dab, %r??
		if (Patterns::addlWithImm32(handle, 0xFFF37DAB)) {
			context.start = current - descriptor.address;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction addl: The relative start address of the inlined invocation is 0x%zx.", context.start);
			current += size;
			continue;
		}
		
		// Guard: Step 3: Find the end address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Pattern: movl $??????????, 0xc8250(%r??)
		if (Patterns::movlImm32ToMemoryWithOffset(handle, 0xC8250)) {
			context.end = current + size - descriptor.address;
			DBGLOG("igfx", "BLT: [KBL ] Found the instruction movl: The relative end address of the inlined invocation is 0x%zx.", context.end);
			break;
		}
		
		current += size;
	}
	
	// All done
	return context;
}

/**
 *  Wrapper to set the backlight compatible with the current machine
 *
 *  @param controller The implicit framebuffer controller
 *  @param brightness The new brightness level
 *  @return `kIOReturnSuccess`.
 *  @note When this function returns, the given brightness level should be stored into the given framebuffer controller.
 *  @note Unlike the original fix implemented in BLR, BLT computes the value of the frequency register and the duty register
 *        compatible with the current machine and commit those values using the original version of `WriteRegister32()`.
 *        Additionally, this function updates the PWM control register at 0xC8250.
 *        Apple's implementation will not be used by this submodule.
 */
IOReturn IGFX::BacklightRegistersAltFixKBL::wrapHwSetBacklight(void *controller, uint32_t brightness) {
	//
	// Differences between this function and the original one:
	//
	// - The wrapper no longer checks whether the current ig-platform-id has the bit `HasBacklight` set.
	// - The wrapper no longer checks whether Camellia is enabled and invokes `CamelliaBase::SetDPCDBacklight()`.
	// - The wrapper writes the PWM frequency set by the system firmware to `BXT_BLC_PWM_FREQ1` if the given brightness is non-zero.
	// - The wrapper uses the PWM frequency set by the system firmware, the given new brightness value and
	//   Apple's frequency divider (specified by the current ig-platform-id) to calculate the value of `BXT_BLC_PWM_DUTY1`.
	// - The wrapper uses the PWM control value set by the system firmware to `BXT_BLC_PWM_CTL1` if the given brightness is non-zero.
	//
	// When this submodule is enabled, the following functions will invoke `AppleIntelFramebufferController::hwSetBacklight()`:
	//
	// - AppleIntelFramebufferController::LightUpEDP() invokes it with the brightness level stored in the framebuffer controller.
	//   Note that Apple's original implementation invokes it with the brightness level bitwise ORed with the frequency divider.
	// - AppleIntelFramebufferController::DisableDisplay() invokes it with a brightness level of 0 to disable the display.
	//   Note that Apple's original implementation writes 0 to both `BXT_BLC_PWM_FREQ1` and `BXT_BLC_PWM_CTL1`.
	// - AppleIntelFramebufferController::hwSetPanelPower() invokes it with the brightness level stored in the framebuffer controller.
	//   Note that Apple's original implementation does not update `BXT_BLC_PWM_FREQ1` but writes 0xC0000000 to `BXT_BLC_PWM_CTL1`.
	//
	auto self = &callbackIGFX->modBacklightRegistersAltFixKBL;
	DBGLOG("igfx", "BLT: [KBL ] Called with the controller at 0x%016llx and the brightness level 0x%x.", reinterpret_cast<uint64_t>(controller), brightness);
	
	// Step 1: Fetch and preserve the PWM control value set by the system firmware
	self->fetchFirmwareBacklightControlIfNecessary(controller);
	
	// Step 2: Fetch and preserve the PWM frequency set by the system firmware
	//         Note that we need to restore the frequency after the system wakes up
	self->fetchFirmwareBacklightFrequencyIfNecessary(controller);
	
	// Step 3: Calculate the new duty cycle for the given brightness level
	uint32_t dutyCycle = self->calcDutyCycle(controller, brightness);
	DBGLOG("igfx", "BLT: [KBL ] Frequency = 0x%x, Divider = 0x%x, Duty Cycle = 0x%x.",
		   self->firmwareBacklightFrequency, self->getFrequencyDivider(controller), dutyCycle);
	
	// Step 4: Write the PWM frequency set by the system firmware to BXT_BLC_PWM_FREQ1
	self->writeFrequency(controller, brightness == 0 ? 0 : self->firmwareBacklightFrequency);
	
	// Step 5: Write the new duty cycle to BXT_BLC_PWM_DUTY1
	self->writeDutyCycle(controller, dutyCycle);
	
	// Step 6: Store the new brightness level in the controller
	if (brightness != 0)
		self->setBrightnessLevel(controller, brightness);

	// Step 7: Update the PWM control register
	self->writePWMControl(controller, brightness == 0 ? 0 : self->firmwareBacklightControl);
	
	// All done
	DBGLOG("igfx", "BLT: [KBL ] The new brightness level 0x%x is now effective.", brightness);
	return kIOReturnSuccess;
}

/**
 *  Analyze `hwSetBacklight()` to find the offset of each required member field in the framebuffer controller
 *
 *  @param address The address of `hwSetBacklight()` to be analyzed
 *  @param instructions The maximum number of instructions to be analyzed
 *  @return A context object that stores the offset of each required member field in the framebuffer controller
 */
IGFX::BacklightRegistersAltFixCFL::ProbeContext IGFX::BacklightRegistersAltFixCFL::probeMemberOffsets(mach_vm_address_t address, size_t instructions) const {
	DBGLOG("igfx", "BLT: [CFL ] Analyzing the function at 0x%016llx to probe the offset of each required member field.", address);
	hde64s handle;
	
	// Record which register stores the implicit controller instance
	// By default, %rdi stores the implicit controller instance (i.e., the 1st argument)
	uint32_t registerController = 7;
	
	// Record which register stores the given brightness level
	// By default, %esi stores the given brightness level (i.e., the 2nd argument)
	uint32_t registerBrightnessLevel = 6;
	
	// Record the offset of the member field that stores the frequency divider
	size_t offsetFrequencyDivider = 0;
	
	// Record the offset of the member field that will store the given brightness level
	size_t offsetBrightnessLevel = 0;
	
	// Analyze at most the given number instructions to find the offsets
	for (size_t index = 0; index < instructions; index += 1) {
		// Guard: Should be able to disassemble the current instruction
		address += Disassembler::hdeDisasm(address, &handle);
		if (handle.flags & F_ERROR) {
			SYSLOG("igfx", "BLT: [CFL ] Error: Cannot disassemble the instruction.");
			break;
		}
		
		// Guard: Step 1: Identify which register stores the controller instance
		// Pattern: movq %rdi, %r??
		// Checks: MOV, 64-bit, Direct Mode, Source Register is %rdi
		if (Patterns::movqArg0(handle)) {
			registerController = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [CFL ] Found the instruction movq: Register %s now stores the controller instance.", registerName(registerController));
			continue;
		}
		
		// Guard: Step 2: Identify which register stores the given brightness level
		// Pattern: movl %esi, %r??
		// Checks: MOV, 32-bit, Direct Mode, Source Register is %esi
		if (Patterns::movlArg1(handle)) {
			registerBrightnessLevel = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [CFL ] Found the instruction movl: Register %s now stores the new brightness level.", registerName(registerBrightnessLevel));
			continue;
		}
		
		// Guard: Step 3: Verify that the given brightness level is stored in the same register
		// Pattern: imull %r??, %r?? where the source register stores the given brightness level and the destination register stores the PWM frequency
		// Checks: IMUL, 32-bit, Direct Mode
		if (Patterns::imull(handle)) {
			if (uint32_t source = handle.rex_b << 3 | handle.modrm_rm; source != registerBrightnessLevel) {
				DBGLOG("igfx", "BLT: [CFL ] Found the instruction imull: Register %s instead of %s now stores the new brightness level.",
					   registerName(source), registerName(registerBrightnessLevel));
				registerBrightnessLevel = source;
			}
			continue;
		}
		
		// Guard: Step 4: Identify the offset of the member field in the controller that stores the frequency divider
		// Pattern: divl <offset?>(%r??)
		// Note that even though Apple initializes this field with a hard coded value of `0xFFFF` in `AppleIntelFramebufferController::getOSInformation()`,
		// we cannot assume that it is always set to `0xFFFF` in future macOS releases, so we will fetch the divider from the controller.
		// Checks: DIV, 32-bit, Memory Mode, Register is identical to the one found in previous steps
		if (Patterns::divlByMemory(handle, registerController)) {
			offsetFrequencyDivider = handle.disp.disp32;
			DBGLOG("igfx", "BLT: [CFL ] Found the instruction divl: The frequency divider is stored at offset 0x%zx.", offsetFrequencyDivider);
			continue;
		}
		
		// Guard: Step 5: Identify the offset of the member field in the controller that will store the given brightness level
		// Pattern: movl %r??, <offset?>(%r??)
		// Checks: MOV, 32-bit, Memory Mode,
		//         Source register is identical to the one that stores the brightness level,
		//         Destination register is identical to the one that stores the controller
		if (Patterns::movlToMemory(handle, registerBrightnessLevel, registerController)) {
			offsetBrightnessLevel = handle.disp.disp32;
			DBGLOG("igfx", "BLT: [CFL ] Found the instruction movl: The brightness level is stored at offset 0x%zx.", offsetBrightnessLevel);
			break;
		}
	}
	
	// All done
	return {offsetFrequencyDivider, offsetBrightnessLevel};
}

/**
 *  Get all functions that contain an inlined invocation of `hwSetBacklight()` and thus will be analyzed and patched
 *
 *  @return A null-terminated array of descriptor, each of describes a function to be analyzed and patched.
 */
IGFX::BacklightRegistersAltFixCFL::FunctionDescriptor* IGFX::BacklightRegistersAltFixCFL::getFunctionDescriptors() const {
	static constexpr const char* kLightUpEDPSymbol = "__ZN31AppleIntelFramebufferController10LightUpEDPEP21AppleIntelFramebufferP21AppleIntelDisplayPathPK29IODetailedTimingInformationV2";
	static constexpr const char* kHwSetPanelPowerSymbol = "__ZN31AppleIntelFramebufferController15hwSetPanelPowerEj";
	
	static FunctionDescriptor kFunctionDescriptors[] = {
		{
			"AppleIntelFramebufferController::LightUpEDP",
			kLightUpEDPSymbol,
			0,
			IGFX::BacklightRegistersAltFixCFL::probeInlinedInvocation,
			IGFX::BacklightRegistersAltFixCFL::revertInlinedInvocation,
			true,
		},
		{
			"AppleIntelFramebufferController::hwSetPanelPower",
			kHwSetPanelPowerSymbol,
			0,
			IGFX::BacklightRegistersAltFixCFL::probeInlinedInvocation,
			IGFX::BacklightRegistersAltFixCFL::revertInlinedInvocation,
			true,
		},
		{
			nullptr,
			nullptr,
			0,
			nullptr,
			nullptr,
			false,
		}
	};
	
	return kFunctionDescriptors;
}

/**
 *  Get the address of the wrapper function that sets the backlight compatible with the current machine
 *
 *  @return The address of the wrapper function.
 */
mach_vm_address_t IGFX::BacklightRegistersAltFixCFL::getHwSetBacklightWrapper() const {
	return reinterpret_cast<mach_vm_address_t>(&IGFX::BacklightRegistersAltFixCFL::wrapHwSetBacklight);
}

/**
 *  Find the location of an inlined invocation of `hwSetBacklight()` in the given function
 *
 *  @param descriptor A descriptor that describes the function to be analyzed
 *  @param probeContext A context object that stores the offset of each required member field in the framebuffer controller
 *  @return A context object that stores a pair of `<Start, End>` offsets, relative to the given address,
 *          which indicates the location of an inlined invocation of `hwSetBacklight()` in this function,
 *          and the register that stores the implicit framebuffer controller instance.
 *  @note This function is specialized for `LightUpEDP()` and `hwSetPanelPower()`.
 */
IGFX::BacklightRegistersAltFixCFL::InvocationContext IGFX::BacklightRegistersAltFixCFL::probeInlinedInvocation(const FunctionDescriptor &descriptor, const ProbeContext &probeContext) {
	DBGLOG("igfx", "BLT: [CFL ] Analyzing %s() at 0x%016llx to identify the position of the inlined invocation of hwSetBacklight().", descriptor.name, descriptor.address);
	hde64s handle;
	
	// The address of the current instruction
	auto current = descriptor.address;
	
	// The context of the inlined invocation
	InvocationContext context;
	
	// Analyze at most the given number instructions to find the location of inlined invocation of `hwSetBacklight()`
	for (size_t index = 0; index < kMaxNumInstructions; index += 1) {
		// Guard: Should be able to disassemble the current instruction
		size_t size = Disassembler::hdeDisasm(current, &handle);
		if (handle.flags & F_ERROR) {
			SYSLOG("igfx", "BLT: [CFL ] Error: Cannot disassemble the instruction.");
			break;
		}
		
		// Guard: Step 1: Find the start address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Pattern: leal 0xfff37da7(%r??), %r?? where the source register stores the base address of the MMIO region
		// Note that the start address found in `LightUpEDP()` is after the invocation of `CamelliaBase::SetDPCDBacklight()`
		// and the retrieval of the base address of the MMIO region
		if (Patterns::lealWithOffset(handle, 0xFFF37DA7)) {
			context.start = current - descriptor.address;
			DBGLOG("igfx", "BLT: [CFL ] Found the instruction leal: The relative start address of the inlined invocation is 0x%zx.", context.start);
			current += size;
			continue;
		}
		
		// Guard: Step 2: Identify which register stores the controller instance
		// Pattern: divl <offset?>(%r??)
		// where the offset is identical to the one found in `hwSetBacklight()`
		if (Patterns::divlByMemoryWithOffset(handle, probeContext.offsetFrequencyDivider)) {
			context.registerController = handle.rex_b << 3 | handle.modrm_rm;
			DBGLOG("igfx", "BLT: [CFL ] Found the instruction divl: Register %s stores the controller instance.", registerName(context.registerController));
			current += size;
			continue;
		}
		
		// Guard: Step 3: Find the end address, relative to the given address, of the inlined invocation of `hwSetBacklight()`
		// Pattern: addl $0xfff37dab, %r??
		if (Patterns::addlWithImm32(handle, 0xFFF37DAB)) {
			context.end = current - descriptor.address;
			DBGLOG("igfx", "BLT: [CFL ] Found the instruction addl: The relative end address of the inlined invocation is 0x%zx.", context.end);
			break;
		}
		
		current += size;
	}
	
	// All done
	return context;
}

/**
 *  Wrapper to set the backlight compatible with the current machine
 *
 *  @param controller The implicit framebuffer controller
 *  @param brightness The new brightness level
 *  @return `kIOReturnSuccess`.
 *  @note When this function returns, the given brightness level should be stored into the given framebuffer controller.
 *  @note Unlike the original fix implemented in BLR, BLT computes the value of the frequency register and the duty register
 *        compatible with the current machine and commit those values using the original version of `WriteRegister32()`.
 *        Apple's implementation will not be used by this submodule.
 */
IOReturn IGFX::BacklightRegistersAltFixCFL::wrapHwSetBacklight(void *controller, uint32_t brightness) {
	//
	// Differences between this function and the original one:
	//
	// - The wrapper no longer checks whether Camellia is enabled and invokes `CamelliaBase::SetDPCDBacklight()`.
	// - The wrapper no longer writes Apple's PWM frequency (0x56CE or 0x4571 depending upon whether the bit 8
	//   is set in `SFUSE_STRAP` (0xC2014), i.e. whether the raw frequency is used) to `BXT_BLC_PWM_FREQ1` (0xC8254).
	// - The wrapper uses the PWM frequency set by the system firmware, the given new brightness value and
	//   Apple's frequency divider (set to 0xFFFF in getOSInformation()) to calculate the value of `BXT_BLC_PWM_DUTY1`.
	//
	auto self = &callbackIGFX->modBacklightRegistersAltFixCFL;
	DBGLOG("igfx", "BLT: [CFL ] Called with the controller at 0x%016llx and the brightness level 0x%x.", reinterpret_cast<uint64_t>(controller), brightness);
	
	// Step 1: Fetch and preserve the PWM frequency set by the system firmware
	//         Note that we need to restore the frequency after the system wakes up
	self->fetchFirmwareBacklightFrequencyIfNecessary(controller);
	
	// Step 2: Calculate the new duty cycle for the given brightness level
	uint32_t dutyCycle = self->calcDutyCycle(controller, brightness);
	DBGLOG("igfx", "BLT: [CFL ] Frequency = 0x%x, Divider = 0x%x, Duty Cycle = 0x%x.",
		   self->firmwareBacklightFrequency, self->getFrequencyDivider(controller), dutyCycle);
	
	// Step 3: Write the PWM frequency set by the system firmware to BXT_BLC_PWM_FREQ1
	self->writeFrequency(controller, self->firmwareBacklightFrequency);
	
	// Step 4: Write the new duty cycle to BXT_BLC_PWM_DUTY1
	self->writeDutyCycle(controller, dutyCycle);
	
	// Step 5: Store the new brightness level in the controller
	self->setBrightnessLevel(controller, brightness);
	
	// All done
	DBGLOG("igfx", "BLT: [CFL ] The new brightness level 0x%x is now effective.", brightness);
	return kIOReturnSuccess;
}

//
// MARK: - Brightness Request Event Source
//

/**
 *  Meta Class Definitions
 */
OSDefineMetaClassAndStructors(BrightnessRequestEventSource, IOEventSource);

/**
 *  Check whether a brightness adjustment request is pending and if so process the request on the workloop
 *
 *  @return `true` if the work loop should invoke this function again.
 *          i.e., One or more requests are pending after the workloop has processed the current one.
 */
bool BrightnessRequestEventSource::checkForWork() {
	// Get the brightness smoother submodule
	IGFX::BacklightSmoother *smoother = &IGFX::callbackIGFX->modBacklightSmoother;
	
	// Get a pending request
	// No work if the queue is empty
	BrightnessRequest request = smoother->request; // TODO: read it atomically, or compare the whole requests instead of ID
	if (!request.controller) {
		DBGLOG("igfx", "BLS: [COMM] The request is empty. Will wait for the next invocation.");
		return false;
	}
	
	// Prepare the request
	[[maybe_unused]] uint64_t snanosecs = getCurrentTimeNs();
	uint32_t current = IGFX::callbackIGFX->readRegister32(request.controller, request.address);
	uint32_t cbrightness = request.getCurrentBrightness(current);
	uint32_t tbrightness = request.getTargetBrightness();

	if (cbrightness == tbrightness) {
		DBGLOG("igfx", "BLS: [COMM] The request is already completed. Will wait for the next invocation.");
		return false;		
	}
	
	tbrightness = max(tbrightness, smoother->brightnessRange.first);  // Ensure that target >= lowerbound
	tbrightness = min(tbrightness, smoother->brightnessRange.second); // Ensure that target <= upperbound
	uint32_t distance = max(cbrightness, tbrightness) - min(cbrightness, tbrightness);
	DBGLOG("igfx", "BLS: [COMM] Processing the request: Current = 0x%08x; Target = 0x%08x; Distance = %04u; Steps = %u.",
		   cbrightness, tbrightness, distance, smoother->steps);
	
	// Process the request
	if (distance > smoother->threshold) {
		if (cbrightness < tbrightness) {
			// Increase the brightness
			for (uint32_t i = 1; i < smoother->steps; i++) {
				uint32_t value = cbrightness + i * distance / smoother->steps;
				IGFX::callbackIGFX->writeRegister32(request.controller, request.address, request.getTargetRegisterValue(value));
				IOSleep(smoother->interval);
				if (smoother->request.id != request.id) {
					return true; // Start again, because there is a new request
				}
			}
		} else if (cbrightness > tbrightness) {
			// Decrease the brightness
			for (uint32_t i = 1; i < smoother->steps; i++) {
				uint32_t value = cbrightness - i * distance / smoother->steps;
				IGFX::callbackIGFX->writeRegister32(request.controller, request.address, request.getTargetRegisterValue(value));
				IOSleep(smoother->interval);
				if (smoother->request.id != request.id) {
					return true; // Start again, because there is a new request
				}
			}
		}
	} else {
		DBGLOG("igfx", "BLS: [COMM] Distance is too short. Will set the target value directly.");
	}
	
	// Finish by writting the target value
	IGFX::callbackIGFX->writeRegister32(request.controller, request.address, request.getTargetRegisterValue(tbrightness));
	[[maybe_unused]] uint64_t enanosecs = getCurrentTimeNs();
	DBGLOG("igfx", "BLS: [COMM] The request completed in %llu nanoseconds.", enanosecs - snanosecs);
	
	// No need to invoke this function again if the request is the same
	return smoother->request.id != request.id;
}

/**
 *  Create an event source
 *
 *  @param owner The owner of the event source
 *  @return A non-null event source on success, `nullptr` otherwise.
 */
BrightnessRequestEventSource *BrightnessRequestEventSource::create(OSObject *owner) {
	auto instance = OSTypeAlloc(BrightnessRequestEventSource);
	if (instance == nullptr)
		return nullptr;
	
	if (!instance->init(owner)) {
		instance->release();
		return nullptr;
	}
	
	return instance;
}

//
// MARK: - Backlight Smoother
//

void IGFX::BacklightSmoother::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingFramebuffer = true;
	
	// We need R/W access to MMIO registers
	requiresMMIORegistersReadAccess = true;
	requiresMMIORegistersWriteAccess = true;
}

void IGFX::BacklightSmoother::deinit() {
	// `BacklightSmoother::processKernel()` guarantees that all pointers are nullptr on failure
	if (workloop != nullptr) {
		if (eventSource != nullptr) {
			workloop->removeEventSource(eventSource);
			OSSafeReleaseNULL(eventSource);
		}
		OSSafeReleaseNULL(workloop);
	}
	OSSafeReleaseNULL(owner);
}

void IGFX::BacklightSmoother::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Check the boot argument and the device property
	enabled = checkKernelArgument("-igfxbls");
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-backlight-smoother") != nullptr;
	if (!enabled)
		return;
	
	// Fetch user configurations
	if (WIOKit::getOSDataValue(info->videoBuiltin, "backlight-smoother-steps", steps))
		DBGLOG("igfx", "BLS: User requested steps = %u.", steps);
	if (WIOKit::getOSDataValue(info->videoBuiltin, "backlight-smoother-interval", interval))
		DBGLOG("igfx", "BLS: User requested interval = %u.", interval);
	if (WIOKit::getOSDataValue(info->videoBuiltin, "backlight-smoother-threshold", threshold))
		DBGLOG("igfx", "BLS: User requested threshold = %u.", threshold);
	if (WIOKit::getOSDataValue(info->videoBuiltin, "backlight-smoother-queue-size", queueSize))
		DBGLOG("igfx", "BLS: User requested queue size = %u.", queueSize);
	if (WIOKit::getOSDataValue(info->videoBuiltin, "backlight-smoother-lowerbound", brightnessRange.first) ||
		WIOKit::getOSDataValue(info->videoBuiltin, "backlight-smoother-upperbound", brightnessRange.second))
		DBGLOG("igfx", "BLS: User requested brightness range = [%u, %u].", brightnessRange.first, brightnessRange.second);
	
	// Sanitize user configurations
	if (steps == 0) {
		SYSLOG("igfx", "BLS: Warning: User requested steps value is invalid. Will use the default value %u.", kDefaultSteps);
		steps = kDefaultSteps;
	}
	
	if (queueSize < kMinimumQueueSize) {
		SYSLOG("igfx", "BLS: Warning: User requested queue size is too small. Will use the minimum value %u.", kMinimumQueueSize);
		queueSize = kMinimumQueueSize;
	}
	
	if (brightnessRange.first > brightnessRange.second) {
		SYSLOG("igfx", "BLS: Warning: User requested brightness range is invalid. Will use the default range.");
		brightnessRange.first = 0;
		brightnessRange.second = UINT32_MAX;
	}
	
	// Wrap this submodule as an OSObject
	owner = OSObjectWrapper::with(this);
	if (owner == nullptr) {
		SYSLOG("igfx", "BLS: Failed to create the owner of the event source.");
		deinit();
		enabled = false;
		return;
	}
	
	// Initialize the request queue
	request = BrightnessRequest();
	
	// Initialize the workloop
	workloop = IOWorkLoop::workLoop();
	if (workloop == nullptr) {
		SYSLOG("igfx", "BLS: Failed to create the workloop.");
		deinit();
		enabled = false;
		return;
	}
	
	// Initialize the request event source
	eventSource = BrightnessRequestEventSource::create(owner);
	if (eventSource == nullptr) {
		SYSLOG("igfx", "BLS: Failed to create the request event source.");
		deinit();
		enabled = false;
		return;
	}

	// Register the event source
	if (workloop->addEventSource(eventSource) != kIOReturnSuccess) {
		SYSLOG("igfx", "BLS: Failed to register the request event source.");
		deinit();
		enabled = false;
	}
}

void IGFX::BacklightSmoother::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	auto framebuffer = Value::of(callbackIGFX->getRealFramebuffer(index));
	if (framebuffer.isOneOf(&kextIntelCapriFb)) {
		DBGLOG("igfx", "BLS: [IVB ] Will setup the smoother for IVB platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dIVBPWMCCTRL);
	} else if (framebuffer.isOneOf(&kextIntelAzulFb, &kextIntelBDWFb, &kextIntelSKLFb, &kextIntelKBLFb)) {
		DBGLOG("igfx", "BLS: [HSW+] Will setup the smoother for HSW/BDW/SKL/KBL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dHSWPWMFreq1);
	} else if (framebuffer.isOneOf(&kextIntelCFLFb, &kextIntelICLLPFb)) {
		DBGLOG("igfx", "BLS: [CFL+] Will setup the smoother for CFL/ICL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dCFLPWMDuty1);
	} else {
		SYSLOG("igfx", "BLS: [ERR!] Unsupported platforms.");
	}
}

void IGFX::BacklightSmoother::smoothIVBWriteRegisterPWMCCTRL(void *controller, uint32_t address, uint32_t value) {
	DBGLOG("igfx", "BLS: [IVB ] WriteRegister32<BLC_PWM_CPU_CTL>: Called with register 0x%x and value 0x%x.", address, value);
	PANIC_COND(address != BLC_PWM_CPU_CTL, "igfx", "Fatal Error: Register should be BLC_PWM_CPU_CTL.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.request = BrightnessRequest(callbackIGFX->modBacklightSmoother.request.id + 1, controller, address, value);
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [IVB ] WriteRegister32<BLC_PWM_CPU_CTL>: The brightness request has been submitted.");
}

void IGFX::BacklightSmoother::smoothHSWWriteRegisterPWMFreq1(void *controller, uint32_t address, uint32_t value) {
	DBGLOG("igfx", "BLS: [HSW+] WriteRegister32<BXT_BLC_PWM_FREQ1>: Called with register 0x%x and value 0x%x.", address, value);
	PANIC_COND(address != BXT_BLC_PWM_FREQ1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_FREQ1.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.request = BrightnessRequest(callbackIGFX->modBacklightSmoother.request.id + 1, controller, address, value, 0xFFFF);
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [HSW+] WriteRegister32<BXT_BLC_PWM_FREQ1>: The brightness request has been submitted.");
}

void IGFX::BacklightSmoother::smoothCFLWriteRegisterPWMDuty1(void *controller, uint32_t address, uint32_t value) {
	DBGLOG("igfx", "BLS: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Called with register 0x%x and value 0x%x.", address, value);
	PANIC_COND(address != BXT_BLC_PWM_DUTY1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_DUTY1.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.request = BrightnessRequest(callbackIGFX->modBacklightSmoother.request.id + 1, controller, address, value);
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: The brightness request has been submitted.");
}
