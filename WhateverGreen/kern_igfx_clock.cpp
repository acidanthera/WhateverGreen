//
//  kern_igfx_clock.cpp
//  WhateverGreen
//
//  Created by FireWolf on 9/19/20.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"
#include <Headers/kern_util.hpp>
#include <IOKit/graphics/IOGraphicsTypes.h>

///
/// This file contains the following clock-related fixes
///
/// 1. Maximum Link Rate fix for eDP panel on CFL+.
/// 2. Core Display Clock fix for the graphics engine on ICL+.
/// 3. HDMI Dividers Calculation fix on SKL, KBL, CFL.
/// 4. Max pixel clock fix on SKL+.
///

// MARK: - Maximum Link Rate Fix

// MARK: Constant Definitions

/**
 *  The default DPCD address that stores receiver capabilities (16 bytes)
 */
static constexpr uint32_t DPCD_DEFAULT_RECEIVER_CAPS_ADDRESS = 0x0000;

/**
 *  The extended DPCD address that stores receiver capabilities (16 bytes)
 */
static constexpr uint32_t DPCD_EXTENDED_RECEIVER_CAPS_ADDRESS = 0x2200;

/**
 *  The DPCD address that stores the eDP version (1 byte)
 */
static constexpr uint32_t DPCD_EDP_VERSION_ADDRESS = 0x700;

/**
 *  The DPCD register value if eDP version is 1.4
 */
static constexpr uint32_t DPCD_EDP_VERSION_1_4_VALUE = 0x03;

/**
 *  The DPCD address that stores link rates supported by the eDP panel (2 bytes * 8)
 */
static constexpr uint32_t DPCD_EDP_SUPPORTED_LINK_RATES_ADDRESS = 0x010;

/**
 *  The maximum number of link rates stored in the table
 */
static constexpr size_t DP_MAX_NUM_SUPPORTED_RATES = 8;

/**
 *  Represents the first 16 fields of the receiver capabilities defined in DPCD
 *
 *  Main Reference:
 *  - DisplayPort Specification Version 1.2
 *
 *  Side Reference:
 *  - struct intel_dp @ line 1073 in intel_drv.h (Linux 4.19 Kernel)
 *  - DP_RECEIVER_CAP_SIZE @ line 964 in drm_dp_helper.h
 */
struct DPCDCap16 { // 16 bytes
	// DPCD Revision (DP Config Version)
	// Value: 0x10, 0x11, 0x12, 0x13, 0x14
	uint8_t revision {};

	// Maximum Link Rate
	// Value: 0x1E (HBR3) 8.1 Gbps
	//        0x14 (HBR2) 5.4 Gbps
	//        0x0C (3_24) 3.24 Gbps
	//        0x0A (HBR)  2.7 Gbps
	//        0x06 (RBR)  1.62 Gbps
	// Reference: 0x0C is used by Apple internally.
	uint8_t maxLinkRate {};

	// Maximum Number of Lanes
	// Value: 0x1 (HBR2)
	//        0x2 (HBR)
	//        0x4 (RBR)
	// Side Notes:
	// (1) Bit 7 is used to indicate whether the link is capable of enhanced framing.
	// (2) Bit 6 is used to indicate whether TPS3 is supported.
	uint8_t maxLaneCount {};

	// Maximum Downspread
	uint8_t maxDownspread {};

	// Other fields omitted in this struct
	// Detailed information can be found in the specification
	uint8_t others[12] {};
};

// MARK: Patch Submodule IMP

void IGFX::DPCDMaxLinkRateFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
	requiresGlobalFramebufferControllersAccess = true;
}

void IGFX::DPCDMaxLinkRateFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable maximum link rate patch if the corresponding boot argument is found
	enabled = checkKernelArgument("-igfxmlr");
	// Or if "enable-dpcd-max-link-rate-fix" is set in IGPU property
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-dpcd-max-link-rate-fix") != nullptr;
	if (!enabled)
		return;
	
	// Read the custom maximum link rate set by the user if present
	if (WIOKit::getOSDataValue(info->videoBuiltin, "dpcd-max-link-rate", maxLinkRate)) {
		// Guard: Verify the custom link rate before using it
		if (verifyLinkRateValue(maxLinkRate) != 0) {
			DBGLOG("igfx", "MLR: Found a valid custom maximum link rate value 0x%02x.", maxLinkRate);
		} else {
			SYSLOG("igfx", "MLR: Found an invalid custom maximum link rate value 0x%02x. Will probe the value automatically.", maxLinkRate);
			maxLinkRate = 0;
		}
	} else {
		DBGLOG("igfx", "MLR: No custom maximum link rate specified. Will probe the value automatically.");
	}
}

void IGFX::DPCDMaxLinkRateFix::processFramebufferKextForICL(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest routeRequest = {
		"__ZN14AppleIntelPort7readAUXEjPvj",
		wrapICLReadAUX,
		orgICLReadAUX
	};
	
	KernelPatcher::SolveRequest solveRequest = {
		"__ZN31AppleIntelFramebufferController13getFBFromPortEP14AppleIntelPort",
		orgICLGetFBFromPort
	};
	
	if (patcher.routeMultiple(index, &routeRequest, 1, address, size) &&
		patcher.solveMultiple(index, &solveRequest, 1, address, size))
		DBGLOG("igfx", "MLR: [ICL+] Functions have been routed successfully.");
	else
		SYSLOG("igfx", "MLR: [ICL+] Failed to route functions.");
}

void IGFX::DPCDMaxLinkRateFix::processFramebufferKextForCFL(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest request = {
		"__ZN31AppleIntelFramebufferController7ReadAUXEP21AppleIntelFramebufferjtPvP21AppleIntelDisplayPath",
		wrapCFLReadAUX,
		orgCFLReadAUX
	};
	
	if (patcher.routeMultiple(index, &request, 1, address, size))
		DBGLOG("igfx", "MLR: [CFL-] Functions have been routed successfully.");
	else
		SYSLOG("igfx", "MLR: [CFL-] Failed to route functions.");
}

void IGFX::DPCDMaxLinkRateFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	// Comet Lake processors have a higher raw value than Ice Lake ones but still rely on Coffee Lake drivers.
	if (BaseDeviceInfo::get().cpuGeneration == CPUInfo::CpuGeneration::IceLake) {
		DBGLOG("igfx", "MLR: Found ICL+ platforms. Will setup the fix for the ICL+ graphics driver.");
		processFramebufferKextForICL(patcher, index, address, size);
	} else {
		DBGLOG("igfx", "MLR: Found CFL- platforms. Will setup the fix for the CFL- graphics driver.");
		processFramebufferKextForCFL(patcher, index, address, size);
	}
}

IOReturn IGFX::DPCDMaxLinkRateFix::wrapCFLReadAUX(AppleIntelFramebufferController *that, IORegistryEntry *framebuffer, uint32_t address, uint16_t length, void *buffer, void *displayPath) {
	// Store required arguments for platform-independent function call
	callbackIGFX->modDPCDMaxLinkRateFix.controller = that;
	callbackIGFX->modDPCDMaxLinkRateFix.framebuffer = framebuffer;
	callbackIGFX->modDPCDMaxLinkRateFix.displayPath = displayPath;
	
	// Invoke the platform-independent wrapper function
	DBGLOG("igfx", "MLR: [CFL-] wrapReadAUX() Called with controller at 0x%llx and framebuffer at 0x%llx.", that, framebuffer);
	return callbackIGFX->modDPCDMaxLinkRateFix.wrapReadAUX(address, buffer, length);
}

IOReturn IGFX::DPCDMaxLinkRateFix::wrapICLReadAUX(AppleIntelPort *that, uint32_t address, void *buffer, uint32_t length) {
	// Store required arguments for platform-independent function call
	callbackIGFX->modDPCDMaxLinkRateFix.port = that;
	
	// Invoke the platform-independent wrapper function
	DBGLOG("igfx", "MLR: [ICL+] wrapReadAUX() Called with port at 0x%llx.", that);
	return callbackIGFX->modDPCDMaxLinkRateFix.wrapReadAUX(address, buffer, length);
}

IOReturn IGFX::DPCDMaxLinkRateFix::wrapReadAUX(uint32_t address, void *buffer, uint32_t length) {
	//
	// Abstract:
	//
	// Several fields in an `AppleIntelFramebuffer` instance are left zeroed because of
	// an invalid value of maximum link rate reported by DPCD of the builtin display.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// **Updated in Phase 3**:
	// Some panels report a maximum link rate of 0x00, but it is totally valid if the panel conforms to eDP 1.4.
	// In this case, an array of supported link rates can be found at DPCD register 0x010 (Link Rate Table).
	// As a result, if the laptop has an eDP 1.4 panel, we could try to probe the maximum link rate from the table.
	//
	// One of those fields, namely the number of lanes, is later used as a divisor during
	// the link training, resulting in a kernel panic triggered by a divison-by-zero.
	//
	// DPCD are retrieved from the display via a helper function named ReadAUX().
	// This wrapper function checks whether the driver is reading receiver capabilities
	// from DPCD of the builtin display and then provides a custom maximum link rate value,
	// so that we don't need to update the binary patch on each system update.
	//
	// If you are interested in the story behind this fix, take a look at my blog posts.
	// Phase 1: https://www.firewolf.science/2018/10/coffee-lake-intel-uhd-graphics-630-on-macos-mojave-a-compromise-solution-to-the-kernel-panic-due-to-division-by-zero-in-the-framebuffer-driver/
	// Phase 2: https://www.firewolf.science/2018/11/coffee-lake-intel-uhd-graphics-630-on-macos-mojave-a-nearly-ultimate-solution-to-the-kernel-panic-due-to-division-by-zero-in-the-framebuffer-driver/
	// Phase 3: https://www.firewolf.science/2020/10/coffee-lake-intel-uhd-graphics-630-on-macos-catalina-the-ultimate-solution-to-the-kernel-panic-due-to-division-by-zero-in-the-framebuffer-driver/

	// Call the original ReadAUX() function to read from DPCD
	IOReturn retVal = callbackIGFX->modDPCDMaxLinkRateFix.orgReadAUX(address, buffer, length);

	// Guard: Check the DPCD register address
	// The first 16 fields of the receiver capabilities reside at 0x0 (DPCD Register Address)
	if (address != DPCD_DEFAULT_RECEIVER_CAPS_ADDRESS && address != DPCD_EXTENDED_RECEIVER_CAPS_ADDRESS)
		return retVal;

	// The driver tries to read the first 16 bytes from DPCD (0x0000) or extended DPCD (0x2200)
	// Get the current framebuffer index (An UInt32 field at 0x1dc in a framebuffer instance)
	// We read the value of "IOFBDependentIndex" instead of accessing that field directly
	uint32_t index;
	// Guard: Should be able to retrieve the index from the registry
	if (!callbackIGFX->modDPCDMaxLinkRateFix.getFramebufferIndex(index)) {
		SYSLOG("igfx", "MLR: [COMM] wrapReadAUX() Failed to read the current framebuffer index.");
		return retVal;
	}

	// Guard: Check the framebuffer index
	// By default, FB 0 refers to the builtin display
	if (index != 0)
		// The driver is reading DPCD for an external display
		return retVal;

	// The driver tries to read the receiver capabilities for the builtin display
	auto caps = reinterpret_cast<DPCDCap16*>(buffer);

	// Set the custom maximum link rate value if user has specified one
	if (callbackIGFX->modDPCDMaxLinkRateFix.maxLinkRate != 0) {
		DBGLOG("igfx", "MLR: [COMM] wrapReadAUX() Will use the maximum link rate specified by user or cached by the previous probe call.");
		caps->maxLinkRate = callbackIGFX->modDPCDMaxLinkRateFix.maxLinkRate;
	} else {
		DBGLOG("igfx", "MLR: [COMM] wrapReadAUX() Will probe the maximum link rate from the table.");
		caps->maxLinkRate = callbackIGFX->modDPCDMaxLinkRateFix.probeMaxLinkRate();
		// The graphics driver tries to read the maximum link rate from both DPCD address 0x0 and 0x2200.
		// We save the probe result to avoid duplicated computation.
		callbackIGFX->modDPCDMaxLinkRateFix.maxLinkRate = caps->maxLinkRate;
	}
	
	// All done
	DBGLOG("igfx", "MLR: [COMM] wrapReadAUX() Maximum link rate 0x%02x has been set in the DPCD buffer.", caps->maxLinkRate);
	return retVal;
}

IOReturn IGFX::DPCDMaxLinkRateFix::orgReadAUX(uint32_t address, void *buffer, uint32_t length) {
	if (port != nullptr) {
		// ICL+
		DBGLOG("igfx", "MLR: [COMM] orgReadAUX() Routed to ICL IMP with Address = 0x%x; Length = %u.", address, length);
		return orgICLReadAUX(port, address, buffer, length);
	}
	
	// CFL-
	DBGLOG("igfx", "MLR: [COMM] orgReadAUX() Routed to CFL IMP with Address = 0x%x; Length = %u.", address, length);
	return orgCFLReadAUX(controller, framebuffer, address, length, buffer, displayPath);
}

bool IGFX::DPCDMaxLinkRateFix::getFramebufferIndex(uint32_t &index) {
	auto fb = port != nullptr ? orgICLGetFBFromPort(callbackIGFX->defaultController(), port) : this->framebuffer;
	DBGLOG("igfx", "MLR: [COMM] GetFBIndex() Port at 0x%llx; Framebuffer at 0x%llx.", port, fb);
	return AppleIntelFramebufferExplorer::getIndex(fb, index);
}

uint32_t IGFX::DPCDMaxLinkRateFix::probeMaxLinkRate() {
	// Precondition: This function is only called when the framebuffer index is 0 (i.e. builtin display)
	// Guard: Read the eDP version from DPCD
	uint8_t eDPVersion;
	if (orgReadAUX(DPCD_EDP_VERSION_ADDRESS, &eDPVersion, 1) != kIOReturnSuccess) {
		SYSLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() Failed to read the eDP version. Aborted.");
		return 0;
	}
	
	// Guard: Ensure that eDP is >= 1.4
	if (eDPVersion < DPCD_EDP_VERSION_1_4_VALUE) {
		SYSLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() eDP version is less than 1.4. Aborted.");
		return 0;
	}
	DBGLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() Found eDP version 1.4+ (Value = 0x%x).", eDPVersion);
	
	// Guard: Read all supported link rates
	uint16_t rates[DP_MAX_NUM_SUPPORTED_RATES] = {0};
	if (orgReadAUX(DPCD_EDP_SUPPORTED_LINK_RATES_ADDRESS, rates, sizeof(rates)) != kIOReturnSuccess) {
		SYSLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() Failed to read supported link rates from DPCD.");
		return 0;
	}
	
	// Parse all supported link rates reported by DPCD
	// The last non-zero entry in the table is the maximum link rate supported by the eDP 1.4 panel
	uint32_t last = 0;
	for (int index = 0; index < arrsize(rates); index++) {
		// Guard: Table is terminated by a zero entry
		if (rates[index] == 0) {
			DBGLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() End of table.");
			break;
		}
		
		// Calculate the link rate value
		// Each element in the table is encoded as a multiple of 200 KHz
		// The decimal value (e.g. 0x14) is encoded as a multiple of 0.27 GHz (270000 KHz)
		uint32_t current = rates[index] * 200 / 270000;
		
		DBGLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() Table[%d] = %5u; Link Rate = %llu; Decimal Value = 0x%02x.",
			   index, rates[index], static_cast<uint64_t>(rates[index]) * 200 * 1000, current);
		
		// Guard: Ensure that we are searching for the largest link rate in case of an unsorted table reported by the panel
		if (current > last)
			last = current;
		else
			SYSLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() Warning: Detected an unsorted table. Please report with your kernel log.");
	}
	
	// Ensure that the maximum link rate found in the table is supported by the driver
	return verifyLinkRateValue(last);
}

// MARK: - Core Display Clock Fix

// MARK: Constant Definitions

/**
 *  Address of the register used to retrieve the current Core Display Clock frequency
 */
static constexpr uint32_t ICL_REG_CDCLK_CTL = 0x46000;

/**
 *  Address of the register used to retrieve the hardware reference clock frequency
 */
static constexpr uint32_t ICL_REG_DSSM = 0x51004;

/**
 *  Enumerates all possible hardware reference clock frequencies on ICL platforms
 *
 *  Reference:
 *  - Intel Graphics Developer Manaual for Ice Lake Platforms, Volume 2c
 *    Command Reference: Registers Part 1 – Registers A through L, DSSM Register
 */
enum ICLReferenceClockFrequency {
	
	// 24 MHz
	ICL_REF_CLOCK_FREQ_24_0 = 0x0,
	
	// 19.2 MHz
	ICL_REF_CLOCK_FREQ_19_2 = 0x1,
	
	// 38.4 MHz
	ICL_REF_CLOCK_FREQ_38_4 = 0x2
};

/**
 *  Enumerates all possible Core Display Clock decimal frequency
 *
 *  Reference:
 *  - Intel Graphics Developer Manaual for Ice Lake Platforms, Volume 2c
 *    Command Reference: Registers Part 1 – Registers A through L, CDCLK_CTL Register
 */
enum ICLCoreDisplayClockDecimalFrequency {
	
	// 172.8 MHz
	ICL_CDCLK_FREQ_172_8 = 0x158,
	
	// 180 MHz
	ICL_CDCLK_FREQ_180_0 = 0x166,
	
	// 192 MHz
	ICL_CDCLK_FREQ_192_0 = 0x17E,
	
	// 307.2 MHz
	ICL_CDCLK_FREQ_307_2 = 0x264,
	
	// 312 MHz
	ICL_CDCLK_FREQ_312_0 = 0x26E,
	
	// 552 MHz
	ICL_CDCLK_FREQ_552_0 = 0x44E,
	
	// 556.8 MHz
	ICL_CDCLK_FREQ_556_8 = 0x458,
	
	// 648 MHz
	ICL_CDCLK_FREQ_648_0 = 0x50E,
	
	// 652.8 MHz
	ICL_CDCLK_FREQ_652_8 = 0x518
};

/**
 *  Get the string representation of the given Core Display Clock decimal frequency
 */
static inline const char* coreDisplayClockDecimalFrequency2String(uint32_t frequency) {
	switch (frequency) {
		case ICL_CDCLK_FREQ_172_8:
			return "172.8";
			
		case ICL_CDCLK_FREQ_180_0:
			return "180";
			
		case ICL_CDCLK_FREQ_192_0:
			return "192";
			
		case ICL_CDCLK_FREQ_307_2:
			return "307.2";
			
		case ICL_CDCLK_FREQ_312_0:
			return "312";
			
		case ICL_CDCLK_FREQ_552_0:
			return "552";
			
		case ICL_CDCLK_FREQ_556_8:
			return "556.8";
			
		case ICL_CDCLK_FREQ_648_0:
			return "648";
			
		case ICL_CDCLK_FREQ_652_8:
			return "652.8";
			
		default:
			return "INVALID";
	}
}

/**
 *  Any Core Display Clock frequency lower than this value is not supported by the driver
 *
 *  @note This threshold is derived from the ICL framebuffer driver on macOS 10.15.6.
 */
static constexpr uint32_t ICL_CDCLK_DEC_FREQ_THRESHOLD = ICL_CDCLK_FREQ_648_0;

/**
 *  Core Display Clock PLL frequency in Hz for the 24 MHz hardware reference frequency
 *
 *  Main Reference:
 *  - Intel Graphics Developer Manaual for Ice Lake Platforms, Volume 12 Display Engine
 *    Page 171, CDCLK PLL Ratio and Divider Programming and Resulting Frequencies.
 *
 *  Side Reference:
 *  - Intel Graphics Driver for Linux (5.8.3) bxt_calc_cdclk_pll_vco() in intel_cdclk.c
 *
 *  @note 54 is the PLL ratio when the reference frequency is 24 MHz
 */
static constexpr uint32_t ICL_CDCLK_PLL_FREQ_REF_24_0 = 24000000 * 54;

/**
 *  Core Display Clock PLL frequency in Hz for the 19.2 MHz hardware reference frequency
 *
 *  @note 68 is the PLL ratio when the reference frequency is 19.2 MHz
 */
static constexpr uint32_t ICL_CDCLK_PLL_FREQ_REF_19_2 = 19200000 * 68;

/**
 *  Core Display Clock PLL frequency in Hz for the 38.4 MHz hardware reference frequency
 *
 *  @note 34 is the PLL ratio when the reference frequency is 19.2 MHz
 */
static constexpr uint32_t ICL_CDCLK_PLL_FREQ_REF_38_4 = 38400000 * 34;

// MARK: Patch Submodule IMP

void IGFX::CoreDisplayClockFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
	
	// Requires read access to MMIO registers
	requiresMMIORegistersReadAccess = true;
}

void IGFX::CoreDisplayClockFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the Core Display Clock patch on ICL platforms
	enabled = checkKernelArgument("-igfxcdc");
	// Or if `enable-cdclk-frequency-fix` is set in IGPU property
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-cdclk-frequency-fix") != nullptr;
}

void IGFX::CoreDisplayClockFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest routeRequest = {
		"__ZN31AppleIntelFramebufferController21probeCDClockFrequencyEv",
		wrapProbeCDClockFrequency,
		orgProbeCDClockFrequency
	};
	
	KernelPatcher::SolveRequest solveRequests[] = {
		{"__ZN31AppleIntelFramebufferController14disableCDClockEv", orgDisableCDClock},
		{"__ZN31AppleIntelFramebufferController19setCDClockFrequencyEy", orgSetCDClockFrequency}
	};
	
	if (patcher.routeMultiple(index, &routeRequest, 1, address, size) &&
		patcher.solveMultiple(index, solveRequests, address, size))
		DBGLOG("igfx", "CDC: Functions have been routed successfully.");
	else
		SYSLOG("igfx", "CDC: Failed to route functions.");
}

void IGFX::CoreDisplayClockFix::sanitizeCDClockFrequency(AppleIntelFramebufferController *that) {
	// Read the hardware reference frequency from the DSSM register
	// Bits 29-31 store the reference frequency value
	auto referenceFrequency = callbackIGFX->readRegister32(that, ICL_REG_DSSM) >> 29;
	
	// Frequency of Core Display Clock PLL is determined by the reference frequency
	uint32_t newCdclkFrequency = 0;
	uint32_t newPLLFrequency = 0;
	switch (referenceFrequency) {
		case ICL_REF_CLOCK_FREQ_19_2:
			DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Reference frequency is 19.2 MHz.");
			newCdclkFrequency = ICL_CDCLK_FREQ_652_8;
			newPLLFrequency = ICL_CDCLK_PLL_FREQ_REF_19_2;
			break;
			
		case ICL_REF_CLOCK_FREQ_24_0:
			DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Reference frequency is 24.0 MHz.");
			newCdclkFrequency = ICL_CDCLK_FREQ_648_0;
			newPLLFrequency = ICL_CDCLK_PLL_FREQ_REF_24_0;
			break;
			
		case ICL_REF_CLOCK_FREQ_38_4:
			DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Reference frequency is 38.4 MHz.");
			newCdclkFrequency = ICL_CDCLK_FREQ_652_8;
			newPLLFrequency = ICL_CDCLK_PLL_FREQ_REF_38_4;
			break;
			
		default:
			SYSLOG("igfx", "CDC: sanitizeCDClockFrequency() Error: Reference frequency is invalid. Will panic later.");
			return;
	}
	
	// Debug: Print the new frequencies
	SYSLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock frequency will be set to %s MHz.",
		   coreDisplayClockDecimalFrequency2String(newCdclkFrequency));
	DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock PLL frequency will be set to %u Hz.", newPLLFrequency);
	
	// Disable the Core Display Clock PLL
	callbackIGFX->modCoreDisplayClockFix.orgDisableCDClock(that);
	DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock PLL has been disabled.");
	
	// Set the new PLL frequency and reenable the Core Display Clock PLL
	callbackIGFX->modCoreDisplayClockFix.orgSetCDClockFrequency(that, newPLLFrequency);
	DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock has been reprogrammed and PLL has been re-enabled.");
	
	// "Verify" that the new frequency is effective
	auto cdclk = callbackIGFX->readRegister32(that, ICL_REG_CDCLK_CTL) & 0x7FF;
	SYSLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock frequency is %s MHz now.",
		   coreDisplayClockDecimalFrequency2String(cdclk));
}

uint32_t IGFX::CoreDisplayClockFix::wrapProbeCDClockFrequency(AppleIntelFramebufferController *that) {
	//
	// Abstract
	//
	// Core Display Clock (CDCLK) is one of the primary clocks used by the display engine to do its work.
	// Apple's graphics driver expects that the firmware has already set the clock frequency to either 652.8 MHz or 648 MHz,
	// but quite a few laptops set it to a much lower value, such as 172.8 MHz,
	// and hence a kernel panic occurs to indicate the precondition failure.
	//
	// This reverse engineering research analyzes functions related to configuring the Core Display Clock
	// and exploits existing APIs to add support for those valid yet non-supported clock frequencies.
	// Since there are multiple spots in the graphics driver that heavily rely on the above assumption,
	// `AppleIntelFramebufferController::probeCDClockFrequency()` is wrapped to adjust the frequency if necessary.
	//
	// If the current Core Display Clock frequency is not natively supported by the driver,
	// this patch will reprogram the clock to set its frequency to a supported value that is appropriate for the hardware.
	// As such, the kernel panic can be avoided, and the built-in display can be lit up successfully.
	//
	// If you are interested in the story behind this fix, please take a look at my blog post.
	// https://www.firewolf.science/2020/08/ice-lake-intel-iris-plus-graphics-on-macos-catalina-a-solution-to-the-kernel-panic-due-to-unsupported-core-display-clock-frequencies-in-the-framebuffer-driver/
	//
	// - FireWolf
	// - 2020.08
	//
	
	DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: Called with controller at 0x%llx.", that);
	
	// Read the Core Display Clock frequency from the CDCLK_CTL register
	// Bit 0 - 11 stores the decimal frequency
	auto cdclk = callbackIGFX->readRegister32(that, ICL_REG_CDCLK_CTL) & 0x7FF;
	SYSLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The current core display clock frequency is %s MHz.",
		   coreDisplayClockDecimalFrequency2String(cdclk));
	
	// Guard: Check whether the current frequency is supported by the graphics driver
	if (cdclk < ICL_CDCLK_DEC_FREQ_THRESHOLD) {
		DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The currrent core display clock frequency is not supported.");
		callbackIGFX->modCoreDisplayClockFix.sanitizeCDClockFrequency(that);
		DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The core display clock has been switched to a supported frequency.");
	}
	
	// Invoke the original method to ensure everything works as expected
	DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: Will invoke the original function.");
	auto retVal = callbackIGFX->modCoreDisplayClockFix.orgProbeCDClockFrequency(that);
	DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The original function returns 0x%llx.", retVal);
	return retVal;
}

// MARK: - HDMI Dividers Calculation Fix

// MARK: Constant Definitions

/**
 *  The maximum positive deviation from the DCO central frequency
 *
 *  @note DCO frequency must be within +1% of the DCO central frequency.
 *  @warning This is a hardware requirement.
 *           See "Intel Graphics Programmer Reference Manual for Kaby Lake platform"
 *           Volume 12 Display, Page 134, Formula for HDMI and DVI DPLL Programming
 *  @link https://01.org/sites/default/files/documentation/intel-gfx-prm-osrc-kbl-vol12-display.pdf
 *  @note This value is appropriate for graphics on Skylake, Kaby Lake and Coffee Lake platforms.
 *  @seealso Intel Linux Graphics Driver
 *  https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/i915/intel_dpll_mgr.c?h=v5.1.13#n1080
 */
static constexpr uint64_t SKL_DCO_MAX_POS_DEVIATION = 100;

/**
 *  The maximum negative deviation from the DCO central frequency
 *
 *  @note DCO frequency must be within -6% of the DCO central frequency.
 *  @seealso See `SKL_DCO_MAX_POS_DEVIATION` above for details.
 */
static constexpr uint64_t SKL_DCO_MAX_NEG_DEVIATION = 600;

/**
 *  Reflect the `AppleIntelFramebufferController::CRTCParams` struct
 *
 *  @note Unlike the Intel Linux Graphics Driver,
 *  - Apple does not transform the `pdiv`, `qdiv` and `kdiv` fields.
 *  - Apple records the final central frequency divided by 15625.
 *  @ref static void skl_wrpll_params_populate(params:afe_clock:central_freq:p0:p1:p2:)
 *  @seealso https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/i915/intel_dpll_mgr.c?h=v5.1.13#n1171
 */
struct CRTCParams {
	/// Uninvestigated fields
	uint8_t uninvestigated[32] {};

	/// P0                          [`CRTCParams` field offset 0x20]
	uint32_t pdiv {};

	/// P1                          [`CRTCParams` field offset 0x24]
	uint32_t qdiv {};

	/// P2                          [`CRTCParams` field offset 0x28]
	uint32_t kdiv {};

	/// Difference in Hz            [`CRTCParams` field offset 0x2C]
	uint32_t fraction {};

	/// Multiplier of 24 MHz        [`CRTCParams` field offset 0x30]
	uint32_t multiplier {};

	/// Central Frequency / 15625   [`CRTCParams` field offset 0x34]
	uint32_t cf15625 {};

	/// The rest fields are not of interest
};

static_assert(offsetof(CRTCParams, pdiv) == 0x20, "Invalid pdiv offset, please check your compiler.");
static_assert(sizeof(CRTCParams) == 56, "Invalid size of CRTCParams struct, please check your compiler.");

// MARK: Patch Submodule IMP

void IGFX::HDMIDividersCalcFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
}

void IGFX::HDMIDividersCalcFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the fix for computing HDMI dividers on SKL, KBL, CFL platforms if the corresponding boot argument is found
	enabled = checkKernelArgument("-igfxhdmidivs");
	// Of if "enable-hdmi-dividers-fix" is set in IGPU property
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-hdmi-dividers-fix") != nullptr;
}

void IGFX::HDMIDividersCalcFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest request("__ZN31AppleIntelFramebufferController17ComputeHdmiP0P1P2EjP21AppleIntelDisplayPathPNS_10CRTCParamsE", wrapComputeHdmiP0P1P2);
	if (!patcher.routeMultiple(index, &request, 1, address, size))
		SYSLOG("igfx", "HDC: Failed to route the function.");
}

void IGFX::HDMIDividersCalcFix::populateP0P1P2(struct ProbeContext *context) {
	uint32_t p = context->divider;
	uint32_t p0 = 0;
	uint32_t p1 = 0;
	uint32_t p2 = 0;

	// Even divider
	if (p % 2 == 0) {
		uint32_t half = p / 2;
		if (half == 1 || half == 2 || half == 3 || half == 5) {
			p0 = 2;
			p1 = 1;
			p2 = half;
		} else if (half % 2 == 0) {
			p0 = 2;
			p1 = half / 2;
			p2 = 2;
		} else if (half % 3 == 0) {
			p0 = 3;
			p1 = half / 3;
			p2 = 2;
		} else if (half % 7 == 0) {
			p0 = 7;
			p1 = half / 7;
			p2 = 2;
		}
	}
	// Odd divider
	else if (p == 3 || p == 9) {
		p0 = 3;
		p1 = 1;
		p2 = p / 3;
	} else if (p == 5 || p == 7) {
		p0 = p;
		p1 = 1;
		p2 = 1;
	} else if (p == 15) {
		p0 = 3;
		p1 = 1;
		p2 = 5;
	} else if (p == 21) {
		p0 = 7;
		p1 = 1;
		p2 = 3;
	} else if (p == 35) {
		p0 = 7;
		p1 = 1;
		p2 = 5;
	}

	context->pdiv = p0;
	context->qdiv = p1;
	context->kdiv = p2;
}

void IGFX::HDMIDividersCalcFix::wrapComputeHdmiP0P1P2(AppleIntelFramebufferController *that, uint32_t pixelClock, void *displayPath, void *parameters) {
	//
	// Abstract
	//
	// ComputeHdmiP0P1P2 is used to compute required parameters to establish the HDMI connection.
	// Apple's original implementation cannot find appropriate dividers for a higher pixel clock
	// value like 533.25 MHz (HDMI 2.0 4K @ 60Hz) and then causes an infinite loop in the kernel.
	//
	// This reverse engineering research focuses on identifying fields in CRTC parameters by
	// carefully analyzing Apple's original implementation, and a better implementation that
	// conforms to Intel Graphics Programmer Reference Manual is provided to fix the issue.
	//
	// This is the first stage to try to solve the HDMI 2.0 output issue on Dell XPS 15 9570,
	// and is now succeeded by the LSPCON driver solution.
	// LSPCON is used to convert DisplayPort signal to HDMI 2.0 signal.
	// When the onboard LSPCON chip is running in LS mode, macOS recognizes the HDMI port as
	// a HDMI port. Consequently, ComputeHdmiP0P1P2() is called by SetupClock() to compute
	// the parameters for the connection.
	// In comparison, when the chip is running in PCON mode, macOS recognizes the HDMI port as
	// a DisplayPort port. As a result, this method is never called by SetupClock().
	//
	// This fix is left here as an emergency fallback and for reference purposes,
	// and is compatible for graphics on Skylake, Kaby Lake and Coffee Lake platforms.
	// Note that it is still capable of finding appropriate dividers for a 1080p HDMI connection
	// and is more robust than the original implementation.
	//
	// For those who want to have "limited" 2K/4K experience (i.e. 2K@59Hz or 4K@30Hz) with their
	// HDMI 1.4 port, you might find this fix helpful.
	//
	// For those who have a laptop or PC with HDMI 2.0 routed to IGPU and have HDMI output issues,
	// it is still recommended to enable the LSPCON driver support to have full HDMI 2.0 experience.
	//
	// - FireWolf
	// - 2019.06
	//

	DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() DInfo: Called with pixel clock = %d Hz.", pixelClock);

	/// All possible dividers
	static constexpr uint32_t dividers[] = {
		// Even dividers
		4,  6,  8, 10, 12, 14, 16, 18, 20,
		24, 28, 30, 32, 36, 40, 42, 44, 48,
		52, 54, 56, 60, 64, 66, 68, 70, 72,
		76, 78, 80, 84, 88, 90, 92, 96, 98,

		// Odd dividers
		3, 5, 7, 9, 15, 21, 35
	};

	/// All possible central frequency values
	static constexpr uint64_t centralFrequencies[3] = {8400000000ULL, 9000000000ULL, 9600000000ULL};

	// Calculate the AFE clock
	uint64_t afeClock = static_cast<uint64_t>(pixelClock) * 5;

	// Prepare the context for probing P0, P1 and P2
	ProbeContext context {};

	// Apple chooses 400 as the initial minimum deviation
	// However 400 is too small for a pixel clock like 533.25 MHz (HDMI 2.0 4K @ 60Hz)
	// Raise the value to UInt64 MAX
	// It's OK because the deviation is still bound by MAX_POS_DEV and MAX_NEG_DEV.
	context.minDeviation = UINT64_MAX;

	for (auto divider : dividers) {
		for (auto central : centralFrequencies) {
			// Calculate the current DCO frequency
			uint64_t frequency = divider * afeClock;
			// Calculate the deviation
			uint64_t deviation = (frequency > central ? frequency - central : central - frequency) * 10000 / central;
			DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() DInfo: Dev = %6llu; Central = %10llu Hz; DCO Freq = %12llu Hz; Divider = %2d.\n", deviation, central, frequency, divider);

			// Guard: Positive deviation is within the allowed range
			if (frequency >= central && deviation >= SKL_DCO_MAX_POS_DEVIATION)
				continue;

			// Guard: Negative deviation is within the allowed range
			if (frequency < central && deviation >= SKL_DCO_MAX_NEG_DEVIATION)
				continue;

			// Guard: Less than the current minimum deviation value
			if (deviation >= context.minDeviation)
				continue;

			// Found a better one
			// Update the value
			context.minDeviation = deviation;
			context.central = central;
			context.frequency = frequency;
			context.divider = divider;
			DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() DInfo: FOUND: Min Dev = %8llu; Central = %10llu Hz; Freq = %12llu Hz; Divider = %d\n", deviation, central, frequency, divider);

			// Guard: Check whether the new minmimum deviation has been reduced to 0
			if (deviation != 0)
				continue;

			// Guard: An even divider is preferred
			if (divider % 2 == 0) {
				DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() DInfo: Found an even divider [%d] with deviation 0.\n", divider);
				break;
			}
		}
	}

	// Guard: A valid divider has been found
	if (context.divider == 0) {
		SYSLOG("igfx", "HDC: ComputeHdmiP0P1P2() Error: Cannot find a valid divider for the given pixel clock %d Hz.\n", pixelClock);
		return;
	}

	// Calculate the p,q,k dividers
	populateP0P1P2(&context);
	DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() DInfo: Divider = %d --> P0 = %d; P1 = %d; P2 = %d.\n", context.divider, context.pdiv, context.qdiv, context.kdiv);

	// Calculate the CRTC parameters
	uint32_t multiplier = (uint32_t) (context.frequency / 24000000);
	uint32_t fraction = (uint32_t) (context.frequency - multiplier * 24000000);
	uint32_t cf15625 = (uint32_t) (context.central / 15625);
	DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() DInfo: Multiplier = %d; Fraction = %d; CF15625 = %d.\n", multiplier, fraction, cf15625);
	
	// Guard: The given CRTC parameters should never be NULL
	if (parameters == nullptr) {
		DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() Error: The given CRTC parameters should not be NULL.");
		return;
	}
	
	// Save all parameters
	auto params = reinterpret_cast<CRTCParams*>(parameters);
	params->pdiv = context.pdiv;
	params->qdiv = context.qdiv;
	params->kdiv = context.kdiv;
	params->multiplier = multiplier;
	params->fraction = fraction;
	params->cf15625 = cf15625;
	DBGLOG("igfx", "HDC: ComputeHdmiP0P1P2() DInfo: CTRC parameters have been populated successfully.");
	return;
}

// MARK: - Max Pixel Clock Override

// MARK: Patch Submodule IMP

void IGFX::MaxPixelClockOverride::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
}

void IGFX::MaxPixelClockOverride::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the max pixel clock override patch if the corresponding boot argument is found
	enabled = checkKernelArgument("-igfxmpc");
	// Or if `enable-max-pixel-clock-override` is set in IGPU property
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-max-pixel-clock-override") != nullptr;

	// Read the custom max pixel clock frequency set by the user if present
	if (enabled)
		WIOKit::getOSDataValue<uint32_t>(info->videoBuiltin, "max-pixel-clock-frequency", maxPixelClockFrequency);
}

void IGFX::MaxPixelClockOverride::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest routeRequest = {
		"__ZN21AppleIntelFramebuffer15connectionProbeEjj",
		wrapConnectionProbe,
		orgConnectionProbe
	};

	if (!patcher.routeMultiple(index, &routeRequest, 1, address, size))
		SYSLOG("igfx", "MPC: Failed to route the function.");
}

IOReturn IGFX::MaxPixelClockOverride::wrapConnectionProbe(IOService *that, unsigned int unk1, unsigned int unk2) {
	// Call the original function, which will set the IOFBTimingRange property
	IOReturn retVal = callbackIGFX->modMaxPixelClockOverride.orgConnectionProbe(that, unk1, unk2);

	// Update the max pixel clock in the IODisplayTimingRange structure
	auto fbTimingRange = OSDynamicCast(OSData, that->getProperty(kIOFBTimingRangeKey));
	if (fbTimingRange) {
		auto displayTimingRange = const_cast<IODisplayTimingRangeV1 *>(reinterpret_cast<const IODisplayTimingRangeV1 *>(fbTimingRange->getBytesNoCopy()));
		DBGLOG("igfx", "MPC: Changing max pixel clock from %llu Hz to %llu Hz", displayTimingRange->maxPixelClock, callbackIGFX->modMaxPixelClockOverride.maxPixelClockFrequency);
		displayTimingRange->maxPixelClock = callbackIGFX->modMaxPixelClockOverride.maxPixelClockFrequency;
	} else {
		SYSLOG("igfx", "MPC: Failed to read IOFBTimingRange property");
	}

	return retVal;
}
