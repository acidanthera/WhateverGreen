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
		SYSLOG("igfx", "BLS: [ERR!] Found an unsupported platform. Will not perform any injections.");
	}
}

void IGFX::BacklightRegistersFix::wrapKBLWriteRegisterPWMFreq1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_FREQ1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_FREQ1.");
	
	if (callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency == 0) {
		// Populate the hardware PWM frequency as initially set up by the system firmware.
		callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency = callbackIGFX->readRegister32(controller, BXT_BLC_PWM_FREQ1);
		DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = 0x%x.",
			   callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency);
		DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_CTL1 = 0x%x.",
			   callbackIGFX->readRegister32(controller, BXT_BLC_PWM_CTL1));

		if (callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency == 0) {
			// This should not happen with correctly written bootloader code, but in case it does, let's use a failsafe default value.
			callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency = FallbackTargetBacklightFrequency;
			SYSLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = ZERO.");
		}
	}

	// For the KBL driver, 0xc8254 (BLC_PWM_PCH_CTL2) controls the backlight intensity.
	// High 16 of this write are the denominator (frequency), low 16 are the numerator (duty cycle).
	// Translate this into a write to c8258 (BXT_BLC_PWM_DUTY1) for the CFL hardware, scaled by the system-provided value in c8254 (BXT_BLC_PWM_FREQ1).
	uint16_t frequency = (value & 0xffff0000U) >> 16U;
	uint16_t dutyCycle = value & 0xffffU;

	uint32_t rescaledValue = frequency == 0 ? 0 : static_cast<uint32_t>((dutyCycle * static_cast<uint64_t>(callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency)) / static_cast<uint64_t>(frequency));
	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_FREQ1>: Write PWM_DUTY1 0x%x/0x%x, rescaled to 0x%x/0x%x.",
		   dutyCycle, frequency, rescaledValue, callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency);

	// Reset the hardware PWM frequency. Write the original system value if the driver-requested value is nonzero. If the driver requests
	// zero, we allow that, since it's trying to turn off the backlight PWM for sleep.
	callbackIGFX->writeRegister32(controller, BXT_BLC_PWM_FREQ1, frequency ? callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency : 0);

	// Finish by writing the duty cycle.
	callbackIGFX->writeRegister32(controller, BXT_BLC_PWM_DUTY1, rescaledValue);
}

void IGFX::BacklightRegistersFix::wrapKBLWriteRegisterPWMCtrl1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_CTL1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_CTL1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_CTL1.");
	
	if (callbackIGFX->modBacklightRegistersFix.targetPwmControl == 0) {
		// Save the original hardware PWM control value
		callbackIGFX->modBacklightRegistersFix.targetPwmControl = callbackIGFX->readRegister32(controller, BXT_BLC_PWM_CTL1);
	}

	DBGLOG("igfx", "BLR: [KBL ] WriteRegister32<BXT_BLC_PWM_CTL1>: Write BXT_BLC_PWM_CTL1 0x%x, previous was 0x%x.",
		   value, callbackIGFX->readRegister32(controller, BXT_BLC_PWM_CTL1));

	if (value) {
		// Set the PWM frequency before turning it on to avoid the 3 minute blackout bug
		callbackIGFX->writeRegister32(controller, BXT_BLC_PWM_FREQ1, callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency);

		// Use the original hardware PWM control value.
		value = callbackIGFX->modBacklightRegistersFix.targetPwmControl;
	}
	
	// Finish by writing the new value
	callbackIGFX->writeRegister32(controller, reg, value);
}

void IGFX::BacklightRegistersFix::wrapCFLWriteRegisterPWMFreq1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_FREQ1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_FREQ1.");
	
	if (value && value != callbackIGFX->modBacklightRegistersFix.driverBacklightFrequency) {
		DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: Driver requested BXT_BLC_PWM_FREQ1 = 0x%x.", value);
		callbackIGFX->modBacklightRegistersFix.driverBacklightFrequency = value;
	}

	if (callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency == 0) {
		// Save the hardware PWM frequency as initially set up by the system firmware.
		// We'll need this to restore later after system sleep.
		callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency = callbackIGFX->readRegister32(controller, BXT_BLC_PWM_FREQ1);
		DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = 0x%x.", callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency);

		if (callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency == 0) {
			// This should not happen with correctly written bootloader code, but in case it does, let's use a failsafe default value.
			callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency = FallbackTargetBacklightFrequency;
			SYSLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_FREQ1>: System initialized with BXT_BLC_PWM_FREQ1 = ZERO.");
		}
	}

	if (value) {
		// Nonzero writes to this register need to use the original system value.
		// Yet the driver can safely write zero to this register as part of system sleep.
		value = callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency;
	}
	
	// Finish by writing the new value
	callbackIGFX->writeRegister32(controller, reg, value);
}

void IGFX::BacklightRegistersFix::wrapCFLWriteRegisterPWMDuty1(void *controller, uint32_t reg, uint32_t value) {
	DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Called with register 0x%x and value 0x%x.", reg, value);
	PANIC_COND(reg != BXT_BLC_PWM_DUTY1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_DUTY1.");
	
	if (callbackIGFX->modBacklightRegistersFix.driverBacklightFrequency && callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency) {
		// Translate the PWM duty cycle between the driver scale value and the HW scale value
		uint32_t rescaledValue = static_cast<uint32_t>((value * static_cast<uint64_t>(callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency)) / static_cast<uint64_t>(callbackIGFX->modBacklightRegistersFix.driverBacklightFrequency));
		DBGLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Write PWM_DUTY1 0x%x/0x%x, rescaled to 0x%x/0x%x.", value,
			   callbackIGFX->modBacklightRegistersFix.driverBacklightFrequency, rescaledValue, callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency);
		value = rescaledValue;
	} else {
		// This should never happen, but in case it does we should log it at the very least.
		SYSLOG("igfx", "BLR: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Write PWM_DUTY1 has zero frequency driver (%d) target (%d).",
			   callbackIGFX->modBacklightRegistersFix.driverBacklightFrequency, callbackIGFX->modBacklightRegistersFix.targetBacklightFrequency);
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
	BrightnessRequest request;
	if (!smoother->queue->pop(request)) {
		DBGLOG("igfx", "BLS: [COMM] The request is empty. Will wait for the next invocation.");
		return false;
	}
	
	// Prepare the request
	[[maybe_unused]] uint64_t snanosecs = getCurrentTimeNs();
	uint32_t current = IGFX::callbackIGFX->readRegister32(request.controller, request.address);
	uint32_t cbrightness = request.getCurrentBrightness(current);
	uint32_t tbrightness = request.getTargetBrightness();
	tbrightness = max(tbrightness, smoother->brightnessRange.first);  // Ensure that target >= lowerbound
	tbrightness = min(tbrightness, smoother->brightnessRange.second); // Ensure that target <= upperbound
	uint32_t distance = max(cbrightness, tbrightness) - min(cbrightness, tbrightness);
	uint32_t stride = distance / smoother->steps + ((distance % smoother->steps) ? 1 : 0);
	DBGLOG("igfx", "BLS: [COMM] Processing the request: Current = 0x%08x; Target = 0x%08x; Distance = %04u; Steps = %u; Stride = %u.",
		   cbrightness, tbrightness, distance, smoother->steps, stride);
	
	// Process the request
	if (distance > smoother->threshold) {
		if (cbrightness < tbrightness) {
			// Increase the brightness
			for (uint32_t value = cbrightness + stride; value <= tbrightness - stride; value += stride) {
				IGFX::callbackIGFX->writeRegister32(request.controller, request.address, request.getTargetRegisterValue(value));
				IOSleep(smoother->interval);
			}
		} else if (cbrightness > tbrightness) {
			// Decrease the brightness
			for (uint32_t value = cbrightness - stride; value >= tbrightness + stride; value -= stride) {
				IGFX::callbackIGFX->writeRegister32(request.controller, request.address, request.getTargetRegisterValue(value));
				IOSleep(smoother->interval);
			}
		}
	} else {
		DBGLOG("igfx", "BLS: [COMM] Distance is too short. Will set the target value directly.");
	}
	
	// Finish by writting the target value
	IGFX::callbackIGFX->writeRegister32(request.controller, request.address, request.getTargetRegisterValue(tbrightness));
	[[maybe_unused]] uint64_t enanosecs = getCurrentTimeNs();
	DBGLOG("igfx", "BLS: [COMM] The request completed in %llu nanoseconds.", enanosecs - snanosecs);
	
	// No need to invoke this function again if the queue is empty
	return !smoother->queue->isEmpty();
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
	BrightnessRequestQueue::safeDeleter(queue);
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
	queue = BrightnessRequestQueue::withCapacity(queueSize);
	if (queue == nullptr) {
		SYSLOG("igfx", "BLS: Failed to initialize the request queue.");
		deinit();
		enabled = false;
		return;
	}
	
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
	callbackIGFX->modBacklightSmoother.queue->push(BrightnessRequest(controller, address, value));
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [IVB ] WriteRegister32<BLC_PWM_CPU_CTL>: The brightness request has been submitted.");
}

void IGFX::BacklightSmoother::smoothHSWWriteRegisterPWMFreq1(void *controller, uint32_t address, uint32_t value) {
	DBGLOG("igfx", "BLS: [HSW+] WriteRegister32<BXT_BLC_PWM_FREQ1>: Called with register 0x%x and value 0x%x.", address, value);
	PANIC_COND(address != BXT_BLC_PWM_FREQ1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_FREQ1.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.queue->push(BrightnessRequest(controller, address, value, 0xFFFF));
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [HSW+] WriteRegister32<BXT_BLC_PWM_FREQ1>: The brightness request has been submitted.");
}

void IGFX::BacklightSmoother::smoothCFLWriteRegisterPWMDuty1(void *controller, uint32_t address, uint32_t value) {
	DBGLOG("igfx", "BLS: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Called with register 0x%x and value 0x%x.", address, value);
	PANIC_COND(address != BXT_BLC_PWM_DUTY1, "igfx", "Fatal Error: Register should be BXT_BLC_PWM_DUTY1.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.queue->push(BrightnessRequest(controller, address, value));
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: The brightness request has been submitted.");
}
