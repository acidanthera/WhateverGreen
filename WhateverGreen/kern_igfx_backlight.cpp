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
	if (!smoother->queue->read(request)) {
		DBGLOG("igfx", "BLS: [COMM] The request is empty. Will wait for the next invocation.");
		return false;
	}
	
	// Prepare the request
	[[maybe_unused]] uint64_t sabstime = mach_absolute_time();
	uint32_t current = IGFX::callbackIGFX->readRegister32(request.controller, request.address);
	uint32_t cbrightness = request.getCurrentBrightness(current);
	uint32_t tbrightness = request.getTargetBrightness();
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
	IGFX::callbackIGFX->writeRegister32(request.controller, request.address, request.target);
	[[maybe_unused]] uint64_t eabstime = mach_absolute_time();
	DBGLOG("igfx", "BLS: [COMM] The request completed in %llu nanoseconds.", MachAbsoluteTime2Nanoseconds(eabstime - sabstime));
	
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
	if (workloop != nullptr && eventSource != nullptr) {
		workloop->removeEventSource(eventSource);
		OSSafeReleaseNULL(eventSource);
		OSSafeReleaseNULL(workloop);
	}
	BrightnessRequestQueue::safeDestory(queue);
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
	
	// Wrap this submodule as an OSObject
	owner = OSObjectWrapper::of(this);
	if (owner == nullptr) {
		SYSLOG("igfx", "BLS: Failed to create the owner of the event source.");
		goto error0;
	}
	
	// Initialize the request queue
	queue = BrightnessRequestQueue::withCapacity(queueSize);
	if (queue == nullptr) {
		SYSLOG("igfx", "BLS: Failed to initialize the request queue.");
		goto error1;
	}
	
	// Initialize the workloop
	workloop = IOWorkLoop::workLoop();
	if (workloop == nullptr) {
		SYSLOG("igfx", "BLS: Failed to create the workloop.");
		goto error2;
	}
	
	// Initialize the request event source
	eventSource = BrightnessRequestEventSource::create(owner);
	if (eventSource == nullptr) {
		SYSLOG("igfx", "BLS: Failed to create the request event source.");
		goto error3;
	}

	// Register the event source
	if (workloop->addEventSource(eventSource) != kIOReturnSuccess) {
		SYSLOG("igfx", "BLS: Failed to register the request event source.");
		goto error3;
	}
	
	return;
	
error3:
	OSSafeReleaseNULL(workloop);
	
error2:
	BrightnessRequestQueue::safeDestory(queue);
	
error1:
	OSSafeReleaseNULL(owner);
	
error0:
	enabled = false;
	SYSLOG("igfx", "BLS: Backlight smoother has been disabled due to initialization failures.");
}

void IGFX::BacklightSmoother::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	auto framebuffer = Value::of(callbackIGFX->getRealFramebuffer(index));
	if (framebuffer.isOneOf(&kextIntelCapriFb)) {
		DBGLOG("igfx", "BLS: [IVB*] Will setup the smoother for IVB platform.");
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
	DBGLOG("igfx", "BLS: [IVB*] WriteRegister32<BLC_PWM_CPU_CTL>: Called with register 0x%x and value 0x%x.", address, value);
	assertf(address == BLC_PWM_CPU_CTL, "Fatal Error: Register should be BLC_PWM_CPU_CTL.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.queue->write(BrightnessRequest(controller, address, value));
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [IVB*] WriteRegister32<BLC_PWM_CPU_CTL>: The brightness request has been submitted.");
}

void IGFX::BacklightSmoother::smoothHSWWriteRegisterPWMFreq1(void *controller, uint32_t address, uint32_t value) {
	DBGLOG("igfx", "BLS: [HSW+] WriteRegister32<BXT_BLC_PWM_FREQ1>: Called with register 0x%x and value 0x%x.", address, value);
	assertf(address == BXT_BLC_PWM_FREQ1, "Fatal Error: Register should be BXT_BLC_PWM_FREQ1.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.queue->write(BrightnessRequest(controller, address, value, 0xFFFF));
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [HSW+] WriteRegister32<BXT_BLC_PWM_FREQ1>: The brightness request has been submitted.");
}

void IGFX::BacklightSmoother::smoothCFLWriteRegisterPWMDuty1(void *controller, uint32_t address, uint32_t value) {
	DBGLOG("igfx", "BLS: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: Called with register 0x%x and value 0x%x.", address, value);
	assertf(address == BXT_BLC_PWM_DUTY1, "Fatal Error: Register should be BXT_BLC_PWM_DUTY1.");
	
	// Submit the request and notify the event source
	callbackIGFX->modBacklightSmoother.queue->write(BrightnessRequest(controller, address, value));
	callbackIGFX->modBacklightSmoother.eventSource->enable();
	DBGLOG("igfx", "BLS: [CFL+] WriteRegister32<BXT_BLC_PWM_DUTY1>: The brightness request has been submitted.");
}
