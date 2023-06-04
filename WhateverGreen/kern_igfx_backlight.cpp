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

///
/// This file contains the following backlight-related fixes and enhancements
///
/// 1. Backlight registers fix that solves the 3-minute black screen on CFL+.
/// 2. Backlight smoother that makes brightness transitions smoother on IVB+.
///

//
// MARK: - Backlight Registers Fix (Supplemental)
//

static constexpr const char* kHwSetBacklightSymbol = "__ZN31AppleIntelFramebufferController14hwSetBacklightEj";
static constexpr const char* kLightUpEDPSymbol = "__ZN31AppleIntelFramebufferController10LightUpEDPEP21AppleIntelFramebufferP21AppleIntelDisplayPathPK29IODetailedTimingInformationV2";
static constexpr const char* kHwSetPanelPowerSymbol = "__ZN31AppleIntelFramebufferController15hwSetPanelPowerEj";

//
// Supplemental Patch 1: Use WriteRegister32() to modify the register `BXT_BLC_PWM_FREQ1` instead of modifying mapped memory directly
//
// Function: AppleIntelFramebufferController::hwSetBacklight()
//   Offset: 53
//  Version: macOS 13.4
// Platform: CFL
//
//     Find: movl 0x2e84(%rbx), %eax  // Fetch the register value (this->field_0x2e84)
//           movq 0x1a08(%rbx), %rcx  // Fetch the base address of the MMIO region
//           movl %eax, 0xc8254(%rcx) // Write the register value to the register at 0xc8254
//
//  Replace: movl 0x2e84(%rbx), %edx  // The register value is the 3rd argument
//           movl $0xc8254, %esi      // The register address is the 2nd argument
//           movq %rbx, %rdi          // The implicit controller instance is the 1st argument
//           call 0x146d98f6          // Call AppleIntelFramebufferController::WriteRegister(address, value)
//           nop                      // Spare byte
//
//
static constexpr uint8_t kHwSetBacklightPatch1_CFL_134[] = {
	0x8B, 0x93, 0x84, 0x2E, 0x00, 0x00, 0xBE, 0x54, 0x82, 0x0C, 0x00, 0x48, 0x89, 0xDF, 0xE8, 0x00, 0xB4, 0xFE, 0xFF
};
static constexpr size_t kHwSetBacklightOffset1_CFL_134 = 53;

//
// Supplemental Patch 2: Use WriteRegister32() to modify the register `BXT_BLC_PWM_DUTY1` instead of modifying mapped memory directly
//
// Function: AppleIntelFramebufferController::hwSetBacklight()
//   Offset: 108
//  Version: macOS 13.4
// Platform: CFL
//
//     Find: movq 0x1a08(%rbx), %rcx  // Fetch the base address of the MMIO region
//           movl %eax, 0xc8258(%rcx) // Write the register value to the register at 0xc8258
//
//  Replace: movl %eax, %edx          // The register value is the 3rd argument
//           movl $0xc8258, %esi      // The register address is the 2nd argument
//           call 0x146d98f6          // Call AppleIntelFramebufferController::WriteRegister(address, value)
//           nop                      // Spare byte
//
// Note that this patch does not need to set the controller instance, because %rdi is set by the first patch.
//
static constexpr uint8_t kHwSetBacklightPatch2_CFL_134[] = {
	0x89, 0xC2, 0xBE, 0x58, 0x82, 0x0C, 0x00, 0xE8, 0xD0, 0xB3, 0xFE, 0xFF, 0x90
};
static constexpr size_t kHwSetBacklightOffset2_CFL_134 = 108;

//
// Supplemental Patch 3: Revert inlined hwSetBacklight() invocation
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
static constexpr uint8_t kLightUpEDPPatch_CFL_134[] = {
	0x41, 0x8B, 0xB7, 0x78, 0x2E, 0x00, 0x00, 0x4C, 0x89, 0xFF, 0xE8, 0x01, 0x6C, 0xFF, 0xFF, 0xEB, 0x71
};
static constexpr size_t kLightUpEDPOffset_CFL_134 = 488;

//
// Supplemental Patch 4: Revert inlined hwSetBacklight() invocation
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
static constexpr uint8_t kHwSetPanelPowerPatch_CFL_134[] = {
	0x51, 0x41, 0x8B, 0xB4, 0x24, 0x78, 0x2E, 0x00, 0x00, 0x4C, 0x89, 0xE7, 0xE8, 0xDC, 0x3A, 0x00, 0x00, 0x59, 0xEB, 0x47
};
static constexpr size_t kHwSetPanelPowerOffset_CFL_134 = 1505;

//
// Supplemental patches for the Coffee Lake framebuffer driver on macOS 13.4
//
static AssemblyPatch kBacklightSupplementalPatches_CFL_134[] = {
	{kHwSetBacklightSymbol,  kHwSetBacklightOffset1_CFL_134, kHwSetBacklightPatch1_CFL_134},
	{kHwSetBacklightSymbol,  kHwSetBacklightOffset2_CFL_134, kHwSetBacklightPatch2_CFL_134},
	{kLightUpEDPSymbol, 	 kLightUpEDPOffset_CFL_134, 	 kLightUpEDPPatch_CFL_134},
	{kHwSetPanelPowerSymbol, kHwSetPanelPowerOffset_CFL_134, kHwSetPanelPowerPatch_CFL_134},
	{nullptr, 0, nullptr, 0},
};

const AssemblyPatch *IGFX::BacklightRegistersSupplementalFix::getPatches(size_t index) const {
	// Fetch the current framebuffer driver
	// Guard: Only CFL is supported at this moment (ICL driver does not have this issue, KBL driver is hard to fix)
	if (auto framebuffer = callbackIGFX->getRealFramebuffer(index); framebuffer != &kextIntelCFLFb) {
		SYSLOG("igfx", "BRS: Only CFL is supported at this moment.");
		return nullptr;
	}
	
	// Guard: Verify the kernel major version
	if (getKernelVersion() != KernelVersion::Ventura) {
		SYSLOG("igfx", "BRS: Only macOS Ventura 13.4.x is supported at this moment.");
		return nullptr;
	}
	
	// Guard: Verify the kernel minor version
	switch (getKernelMinorVersion()) {
		case 5: // macOS 13.4.x
			return kBacklightSupplementalPatches_CFL_134;
			
		default:
			SYSLOG("igfx", "BRS: Only macOS Ventura 13.4 is supported at this moment.");
			return nullptr;
	}
}

bool IGFX::BacklightRegistersSupplementalFix::applyPatches(KernelPatcher &patcher, size_t index, const AssemblyPatch *patches) const {
	// Apply each assembly patch
	for (const AssemblyPatch *patch = patches; patch->symbol != nullptr; patch += 1) {
		// Guard: Resolve the symbol of the function to be patched
		auto address = patcher.solveSymbol(index, patch->symbol);
		if (address == 0) {
			SYSLOG("igfx", "BRS: Failed to resolve the symbol %s. The supplement fix will not work properly.", patch->symbol);
			patcher.clearError();
			return false;
		}
		
		// Guard: Apply the assembly patch
		if (patcher.routeBlock(address + patch->offset, patch->patch, patch->patchSize) != 0) {
			SYSLOG("igfx", "BRS: Failed to apply the assembly patch to the function %s. The supplement fix will not work properly.", patch->symbol);
			patcher.clearError();
			return false;
		}
		
		DBGLOG("igfx", "BRS: Applied the assembly patch to the function %s successfully.", patch->symbol);
	}
	
	return true;
}

void IGFX::BacklightRegistersSupplementalFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingFramebuffer = true;
}

void IGFX::BacklightRegistersSupplementalFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	enabled = checkKernelArgument("-igfxbrs");
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-backlight-registers-supplemental-fix") != nullptr;
	if (!enabled)
		return;
}

void IGFX::BacklightRegistersSupplementalFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	//
	// Apple has "accidentally" simplified the implementation of the functions, `ReadRegister32` and `WriteRegister32`, in Coffee Lake's framebuffer driver shipped by macOS 13.4,
	// so the compiler chose to inline invocations of those functions as many as possible. As a result, `hwSetBacklight()` no longer invokes
	//  `WriteRegister32` to update backlight registers; instead it modifies the register value via mapped memory directly, making itself an inline helper.
	// `LightUpEDP()` and `hwSetPanelPower()` that invoke `hwSetBacklight()` now have the definition of `hwSetBacklight()` embedded in themselves.
	// As such, the `WriteRegister32` hooks registered by the Backlight Registers Fix (BLR) and the Backlight Smoother (BLS) submodules no longer work.
	// This patch submodule (BRS) reverts the optimizations done by the compiler in aforementioned three functions, thus making both BLR and BLS work properly on macOS 13.4.
	//
	// - FireWolf
	// - 2023.06
	//
	auto self = &callbackIGFX->modBacklightRegistersSupplementalFix;
	
	// Guard: Fetch the assembly patches for the current framebuffer driver
	auto patches = self->getPatches(index);
	if (patches == nullptr) {
		SYSLOG("igfx", "BRS: Your current kernel version and/or platform is not supported.");
		return;
	}
	
	// Guard: Apply the assemble patches to the current framebuffer driver
	if (self->applyPatches(patcher, index, patches)) {
		DBGLOG("igfx", "BRS: All assembly patches have been applied to the current framebuffer driver.");
	} else {
		SYSLOG("igfx", "BRS: Failed to apply some of the assembly patches to the current framebuffer driver.");
	}
}

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
	callbackIGFX->writeRegister32(controller, BXT_BLC_PWM_DUTY1, rescaledValue);
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
