//
//  kern_igfx_lspcon.cpp
//  WhateverGreen
//
//  Created by FireWolf on 9/21/20.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include "kern_igfx_lspcon.hpp"
#include "kern_igfx.hpp"

// MARK: - LSPCON Driver Foundation

LSPCON *LSPCON::create(void *controller, IORegistryEntry *framebuffer, void *displayPath) {
	// Guard: Framebuffer index should exist
	uint32_t index;
	if (!IGFX::AppleIntelFramebufferExplorer::getIndex(framebuffer, index))
		return nullptr;
	
	// Call the private constructor
	return new LSPCON(controller, framebuffer, displayPath, index);
}

LSPCON::LSPCON(void *controller, IORegistryEntry *framebuffer, void *displayPath, uint32_t index) {
	this->controller = controller;
	this->framebuffer = framebuffer;
	this->displayPath = displayPath;
	this->index = index;
}

IOReturn LSPCON::probe() {
	// Read the adapter info
	uint8_t buffer[128] {};
	IOReturn retVal = IGFX::AdvancedI2COverAUXSupport::advReadI2COverAUX(controller, framebuffer, displayPath, DP_DUAL_MODE_ADAPTER_I2C_ADDR, 0x00, 128, buffer, 0);
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: LSPCON::probe() Error: [FB%d] Failed to read the LSPCON adapter info. RV = 0x%llx.", index, retVal);
		return retVal;
	}

	// Start to parse the adapter info
	auto info = reinterpret_cast<DisplayPortDualModeAdapterInfo*>(buffer);
	// Guard: Check whether this is a LSPCON adapter
	if (!isLSPCONAdapter(info)) {
		SYSLOG("igfx", "SC: LSPCON::probe() Error: [FB%d] Not a LSPCON DP-HDMI adapter. AdapterID = 0x%02x.", index, info->adapterID);
		return kIOReturnNotFound;
	}

	// Parse the chip vendor
	char device[8] {};
	lilu_os_memcpy(device, info->deviceID, 6);
	DBGLOG("igfx", "SC: LSPCON::probe() DInfo: [FB%d] Found the LSPCON adapter: %s %s.", index, Vendor::parse(info).getDescription(), device);

	// Parse the current adapter mode
	Mode mode = Mode::parse(info->lspconCurrentMode);
	DBGLOG("igfx", "SC: LSPCON::probe() DInfo: [FB%d] The current adapter mode is %s.", index, mode.getDescription());
	if (mode.isInvalid())
		SYSLOG("igfx", "SC: LSPCON::probe() Error: [FB%d] Cannot detect the current adapter mode. Assuming Level Shifter mode.", index);
	return kIOReturnSuccess;
}

IOReturn LSPCON::getMode(Mode &mode) {
	IOReturn retVal = kIOReturnAborted;

	// Try to read the current mode from the adapter at most 5 times
	for (int attempt = 0; attempt < 5; attempt++) {
		// Read from the adapter @ 0x40; offset = 0x41
		uint8_t hwModeValue;
		retVal = IGFX::AdvancedI2COverAUXSupport::advReadI2COverAUX(controller, framebuffer, displayPath, DP_DUAL_MODE_ADAPTER_I2C_ADDR, DP_DUAL_MODE_LSPCON_CURRENT_MODE, 1, &hwModeValue, 0);

		// Guard: Can read the current adapter mode successfully
		if (retVal == kIOReturnSuccess) {
			DBGLOG("igfx", "SC: LSPCON::getMode() DInfo: [FB%d] The current mode value is 0x%02x.", index, hwModeValue);
			mode = Mode::parse(hwModeValue);
			break;
		}

		// Sleep 1 ms just in case the adapter
		// is busy processing other I2C requests
		IOSleep(1);
	}
	
	return retVal;
}

IOReturn LSPCON::setMode(Mode newMode) {
	// Guard: The given new mode must be valid
	if (newMode.isInvalid())
		return kIOReturnAborted;

	// Guard: Write the new mode
	uint8_t hwModeValue = newMode.getRawValue();
	IOReturn retVal = IGFX::AdvancedI2COverAUXSupport::advWriteI2COverAUX(controller, framebuffer, displayPath, DP_DUAL_MODE_ADAPTER_I2C_ADDR, DP_DUAL_MODE_LSPCON_CHANGE_MODE, 1, &hwModeValue, 0);
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: LSPCON::setMode() Error: [FB%d] Failed to set the new adapter mode. RV = 0x%llx.", index, retVal);
		return retVal;
	}

	// Read the register again and verify the mode
	uint32_t timeout = 200;
	Mode mode;
	while (timeout != 0) {
		retVal = getMode(mode);
		// Guard: Read the current effective mode
		if (retVal != kIOReturnSuccess) {
			SYSLOG("igfx", "SC: LSPCON::setMode() Error: [FB%d] Failed to read the new effective mode. RV = 0x%llx.", index, retVal);
			continue;
		}
		// Guard: The new mode is effective now
		if (mode == newMode) {
			DBGLOG("igfx", "SC: LSPCON::setMode() DInfo: [FB%d] The new mode is now effective.", index);
			return kIOReturnSuccess;
		}
		timeout -= 20;
		IOSleep(20);
	}

	SYSLOG("igfx", "SC: LSPCON::setMode() Error: [FB%d] Timed out while waiting for the new mode to be effective. Last RV = 0x%llx.", index, retVal);
	return retVal;
}

IOReturn LSPCON::setModeIfNecessary(Mode newMode) {
	if (isRunningInMode(newMode)) {
		DBGLOG("igfx", "SC: LSPCON::setModeIfNecessary() DInfo: [FB%d] The adapter is already running in %s mode. No need to update.", index, newMode.getDescription());
		return kIOReturnSuccess;
	}

	return setMode(newMode);
}

IOReturn LSPCON::wakeUpNativeAUX() {
	uint8_t byte;
	IOReturn retVal = IGFX::callbackIGFX->modLSPCONDriverSupport.orgReadAUX(controller, framebuffer, 0x00000, 1, &byte, displayPath);
	if (retVal != kIOReturnSuccess)
		SYSLOG("igfx", "SC: LSPCON::wakeUpNativeAUX() Error: [FB%d] Failed to wake up the native AUX channel. RV = 0x%llx.", index, retVal);
	else
		DBGLOG("igfx", "SC: LSPCON::wakeUpNativeAUX() DInfo: [FB%d] The native AUX channel is up. DPCD Rev = 0x%02x.", index, byte);
	return retVal;
}

bool LSPCON::isRunningInMode(Mode mode)
{
	Mode currentMode;
	if (getMode(currentMode) != kIOReturnSuccess) {
		DBGLOG("igfx", "LSPCON::isRunningInMode() Error: [FB%d] Failed to get the current adapter mode.", index);
		return false;
	}
	return mode == currentMode;
}

// MARK: - LSPCON Driver Support


