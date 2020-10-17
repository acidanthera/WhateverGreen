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

void IGFX::LSPCONDriverSupport::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
}

void IGFX::LSPCONDriverSupport::deinit() {
	for (auto &con : lspcons) {
		LSPCON::deleter(con.lspcon);
		con.lspcon = nullptr;
	}
}

void IGFX::LSPCONDriverSupport::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the LSPCON driver support if the corresponding boot argument is found
	enabled = checkKernelArgument("-igfxlspcon");
	// Or if "enable-lspcon-support" is set in IGPU property
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-lspcon-support") != nullptr;
	
	// Read the user-defined IGPU properties to know whether a connector has an onboard LSPCON chip
	if (enabled) {
		char name[48];
		uint32_t pmode = 0x01; // PCON mode as a fallback value
		for (size_t index = 0; index < arrsize(lspcons); index++) {
			bzero(name, sizeof(name));
			snprintf(name, sizeof(name), "framebuffer-con%lu-has-lspcon", index);
			(void)WIOKit::getOSDataValue(info->videoBuiltin, name, lspcons[index].hasLSPCON);
			snprintf(name, sizeof(name), "framebuffer-con%lu-preferred-lspcon-mode", index);
			(void)WIOKit::getOSDataValue(info->videoBuiltin, name, pmode);
			// Assuming PCON mode if invalid mode value (i.e. > 1) specified by the user
			lspcons[index].preferredMode = LSPCON::Mode::parse(pmode != 0);
		}
	}
}

void IGFX::LSPCONDriverSupport::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest routeRequest = {
		"__ZN31AppleIntelFramebufferController11GetDPCDInfoEP21AppleIntelFramebufferP21AppleIntelDisplayPath",
		wrapGetDPCDInfo,
		orgGetDPCDInfo
	};
	
	KernelPatcher::SolveRequest solveRequest = {
		"__ZN31AppleIntelFramebufferController7ReadAUXEP21AppleIntelFramebufferjtPvP21AppleIntelDisplayPath",
		orgReadAUX
	};
	
	if (patcher.routeMultiple(index, &routeRequest, 1, address, size) &&
		patcher.solveMultiple(index, &solveRequest, 1, address, size)) {
		DBGLOG("igfx", "SC: Functions have been routed successfully");
	} else {
		patcher.clearError();
		SYSLOG("igfx", "SC: Failed to route functions.");
	}
}

void IGFX::LSPCONDriverSupport::setupLSPCON(void *that, IORegistryEntry *framebuffer, void *displayPath) {
	// Retrieve the framebuffer index
	uint32_t index;
	if (!AppleIntelFramebufferExplorer::getIndex(framebuffer, index)) {
		SYSLOG("igfx", "SC: fbSetupLSPCON() Error: Failed to retrieve the framebuffer index.");
		return;
	}
	DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] called with controller at 0x%llx and framebuffer at 0x%llx.", index, that, framebuffer);

	// Retrieve the user preference
	LSPCON *lspcon = nullptr;
	auto pmode = getLSPCONPreferredMode(index);
#ifdef DEBUG
	framebuffer->setProperty("fw-framebuffer-has-lspcon", hasLSPCON(index));
	framebuffer->setProperty("fw-framebuffer-preferred-lspcon-mode", pmode.getRawValue(), 8);
#endif

	// Guard: Check whether this framebuffer connector has an onboard LSPCON chip
	if (!hasLSPCON(index)) {
		// No LSPCON chip associated with this connector
		DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] No LSPCON chip associated with this framebuffer.", index);
		return;
	}

	// Guard: Check whether the LSPCON driver has already been initialized for this framebuffer
	if (hasLSPCONInitialized(index)) {
		// Already initialized
		lspcon = getLSPCON(index);
		DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] LSPCON driver (at 0x%llx) has already been initialized for this framebuffer.", index, lspcon);
		// Confirm that the adapter is running in preferred mode
		if (lspcon->setModeIfNecessary(pmode) != kIOReturnSuccess) {
			SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] The adapter is not running in preferred mode. Failed to update the mode.", index);
		}
		// Wake up the native AUX channel if PCON mode is preferred
		if (pmode.supportsHDMI20() && lspcon->wakeUpNativeAUX() != kIOReturnSuccess) {
			SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] Failed to wake up the native AUX channel.", index);
		}
		return;
	}

	// User has specified the existence of onboard LSPCON
	// Guard: Initialize the driver for this framebuffer
	lspcon = LSPCON::create(that, framebuffer, displayPath);
	if (lspcon == nullptr) {
		SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] Failed to initialize the LSPCON driver.", index);
		return;
	}

	// Guard: Attempt to probe the onboard LSPCON chip
	if (lspcon->probe() != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] Failed to probe the LSPCON adapter.", index);
		LSPCON::deleter(lspcon);
		lspcon = nullptr;
		SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] Abort the LSPCON driver initialization.", index);
		return;
	}
	DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] LSPCON driver has detected the onboard chip successfully.", index);

	// LSPCON driver has been initialized successfully
	setLSPCON(index, lspcon);
	DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] LSPCON driver has been initialized successfully.", index);

	// Guard: Set the preferred adapter mode if necessary
	if (lspcon->setModeIfNecessary(pmode) != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] The adapter is not running in preferred mode. Failed to set the %s mode.", index, pmode.getDescription());
	}
	DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] The adapter is now running in preferred mode [%s].", index, pmode.getDescription());
}

IOReturn IGFX::LSPCONDriverSupport::wrapGetDPCDInfo(void *that, IORegistryEntry *framebuffer, void *displayPath) {
	//
	// Abstract
	//
	// Recent laptops are now typically equipped with a HDMI 2.0 port.
	// For laptops with Intel IGPU + Nvidia DGPU, this HDMI 2.0 port could be either routed to IGPU or DGPU,
	// and we are only interested in the former case, because DGPU (Optimus) is not supported on macOS.
	// However, as indicated in Intel Product Specifications, Intel (U)HD Graphics card does not provide
	// native HDMI 2.0 output. As a result, Intel suggests that OEMs should add an additional hardware component
	// named LSPCON (Level Shifter and Protocol Converter) on the motherboard to convert DisplayPort to HDMI.
	//
	// LSPCON conforms to the DisplayPort Dual Mode (DP++) standard and works in either Level Shifter (LS) mode
	// or Protocol Converter (PCON) mode.
	// When the adapter is running in   LS mode, it supports DisplayPort to HDMI 1.4.
	// When the adapter is running in PCON mode, it supports DisplayPort to HDMI 2.0.
	// LSPCON on some laptops, Dell XPS 15 9570 for example, might be configured in the firmware to run in LS mode
	// by default, resulting in a black screen on a HDMI 2.0 connection.
	// In comparison, LSPCON on a DisplayPort to HDMI 2.0 cable could be configured to always run in PCON mode.
	// Fortunately, LSPCON is programmable and is a Type 2 DP++ adapter, so the graphics driver could communicate
	// with the adapter via either the native I2C protocol or the special I2C-over-AUX protocol to configure the
	// mode properly for HDMI connections.
	//
	// This reverse engineering research analyzes and exploits Apple's existing I2C-over-AUX transaction API layers,
	// and an additional API layer is implemented to provide R/W access to registers at specific offsets on the adapter.
	// AppleIntelFramebufferController::GetDPCDInfo() is then wrapped to inject code for LSPCON driver initialization,
	// so that the adapter is configured to run in PCON mode instead on plugging the HDMI 2.0 cable to the port.
	// Besides, the adapter is able to handle HDMI 1.4 connections when running in PCON mode, so we don't need to switch
	// back to LS mode again.
	//
	// If you are interested in the detailed theory behind this fix, please take a look at my blog post.
	// [ToDo: The article is still working in progress. I will add the link at here once I finish it. :D]
	//
	// Notes:
	// 1. This fix is applicable for all laptops and PCs with HDMI 2.0 routed to IGPU.
	//    (Laptops/Mobiles start from Skylake platform. e.g. Intel Skull Canyon NUC; Iris Pro 580 and HDMI 2.0)
	// 2. If your HDMI 2.0 with Intel (U)HD Graphics is working properly, you don't need this fix,
	//    as the adapter might already be configured in the firmware to run in PCON mode.
	//
	// - FireWolf
	// - 2019.06
	//

	// Configure the LSPCON adapter for the given framebuffer
	DBGLOG("igfx", "SC: GetDPCDInfo() DInfo: Start to configure the LSPCON adapter.");
	callbackIGFX->modLSPCONDriverSupport.setupLSPCON(that, framebuffer, displayPath);
	DBGLOG("igfx", "SC: GetDPCDInfo() DInfo: Finished configuring the LSPCON adapter.");

	// Call the original method
	DBGLOG("igfx", "SC: GetDPCDInfo() DInfo: Will call the original method.");
	IOReturn retVal = callbackIGFX->modLSPCONDriverSupport.orgGetDPCDInfo(that, framebuffer, displayPath);
	DBGLOG("igfx", "SC: GetDPCDInfo() DInfo: Returns 0x%llx.", retVal);
	return retVal;
}
