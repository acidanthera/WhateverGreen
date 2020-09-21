//
//  kern_igfx_clock.cpp
//  WhateverGreen
//
//  Created by FireWolf on 9/19/20.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"
#include <Headers/kern_util.hpp>

///
/// This file contains the following clock-related fixes
///
/// 1. Maximum Link Rate fix for eDP panel on CFL+.
/// 2. Core Display Clock fix for the graphics engine on ICL+.
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
	uint8_t revision;

	// Maximum Link Rate
	// Value: 0x1E (HBR3) 8.1 Gbps
	//        0x14 (HBR2) 5.4 Gbps
	//        0x0C (3_24) 3.24 Gbps
	//        0x0A (HBR)  2.7 Gbps
	//        0x06 (RBR)  1.62 Gbps
	// Reference: 0x0C is used by Apple internally.
	uint8_t maxLinkRate;

	// Maximum Number of Lanes
	// Value: 0x1 (HBR2)
	//        0x2 (HBR)
	//        0x4 (RBR)
	// Side Notes:
	// (1) Bit 7 is used to indicate whether the link is capable of enhanced framing.
	// (2) Bit 6 is used to indicate whether TPS3 is supported.
	uint8_t maxLaneCount;

	// Maximum Downspread
	uint8_t maxDownspread;

	// Other fields omitted in this struct
	// Detailed information can be found in the specification
	uint8_t others[12];
};

// MARK: Patch Submodule IMP

void IGFX::DPCDMaxLinkRateFix::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
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
	auto raux = patcher.solveSymbol(index, "__ZN14AppleIntelPort7readAUXEjPvj", address, index);
	auto gfbp = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController13getFBFromPortEP14AppleIntelPort", address, index);
	
	if (raux && gfbp) {
		patcher.eraseCoverageInstPrefix(raux);
		orgICLReadAUX = reinterpret_cast<decltype(orgICLReadAUX)>(patcher.routeFunction(raux, reinterpret_cast<mach_vm_address_t>(wrapICLReadAUX), true));
		orgICLGetFBFromPort = reinterpret_cast<decltype(orgICLGetFBFromPort)>(gfbp);
		if (orgICLReadAUX && orgICLGetFBFromPort) {
			DBGLOG("igfx", "MLR: [ICL+] Functions have been routed successfully.");
		} else {
			SYSLOG("igfx", "MLR: [ICL+] Failed to route functions.");
		}
	} else {
		SYSLOG("igfx", "MLR: [ICL+] Failed to find symbols.");
		patcher.clearError();
	}
}

void IGFX::DPCDMaxLinkRateFix::processFramebufferKextForCFL(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	auto raux = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController7ReadAUXEP21AppleIntelFramebufferjtPvP21AppleIntelDisplayPath", address, size);
	
	if (raux) {
		patcher.eraseCoverageInstPrefix(raux);
		orgCFLReadAUX = reinterpret_cast<decltype(orgCFLReadAUX)>(patcher.routeFunction(raux, reinterpret_cast<mach_vm_address_t>(wrapCFLReadAUX), true));
		if (orgCFLReadAUX) {
			DBGLOG("igfx", "MLR: [CFL-] Functions have been routed successfully.");
		} else {
			patcher.clearError();
			SYSLOG("igfx", "MLR: [CFL-] Failed to route functions.");
		}
	} else {
		SYSLOG("igfx", "MLR: [CFL-] Failed to find symbols.");
		patcher.clearError();
	}
}

void IGFX::DPCDMaxLinkRateFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (BaseDeviceInfo::get().cpuGeneration >= CPUInfo::CpuGeneration::IceLake) {
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
	// Phase 3: TODO: ADD LINK

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
	} else {
		// CFL-
		DBGLOG("igfx", "MLR: [COMM] orgReadAUX() Routed to CFL IMP with Address = 0x%x; Length = %u.", address, length);
		return orgCFLReadAUX(controller, framebuffer, address, length, buffer, displayPath);
	}
}

bool IGFX::DPCDMaxLinkRateFix::getFramebufferIndex(uint32_t &index) {
	auto framebuffer = port != nullptr ? orgICLGetFBFromPort(*callbackIGFX->gFramebufferController, port) : this->framebuffer;
	DBGLOG("igfx", "MLR: [COMM] GetFBIndex() Port at 0x%llx; Framebuffer at 0x%llx.", port, framebuffer);
	return AppleIntelFramebufferExplorer::getIndex(framebuffer, index);
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
	for (int index = 0; index < arrsize(rates); index += 1) {
		// Guard: Table is terminated by a zero entry
		if (rates[index] == 0) {
			DBGLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() End of table.");
			break;
		}
		
		// Calculate the link rate value
		// Each element in the table is encoded as a multiple of 200 KHz
		// The decimal value (e.g. 0x14) is encoded as a multiple of 0.27 GHz (270000 KHz)
		last = rates[index] * 200 / 270000;
		DBGLOG("igfx", "MLR: [COMM] ProbeMaxLinkRate() Table[%d] = %5u; Link Rate = %llu; Decimal Value = 0x%02x.",
			   index, rates[index], static_cast<uint64_t>(rates[index]) * 200 * 1000, last);
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
}

void IGFX::CoreDisplayClockFix::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the Core Display Clock patch on ICL platforms
	enabled = checkKernelArgument("-igfxcdc");
	// Or if `enable-cdclk-frequency-fix` is set in IGPU property
	if (!enabled)
		enabled = info->videoBuiltin->getProperty("enable-cdclk-frequency-fix") != nullptr;
}

void IGFX::CoreDisplayClockFix::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	auto pcdcAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController21probeCDClockFrequencyEv", address, size);
	auto dcdcAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController14disableCDClockEv", address, size);
	auto scdcAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController19setCDClockFrequencyEy", address, size);
	
	if (pcdcAddress && dcdcAddress && scdcAddress && callbackIGFX->AppleIntelFramebufferController__ReadRegister32) {
		patcher.eraseCoverageInstPrefix(pcdcAddress);
		orgProbeCDClockFrequency = reinterpret_cast<decltype(orgProbeCDClockFrequency)>(patcher.routeFunction(pcdcAddress, reinterpret_cast<mach_vm_address_t>(wrapProbeCDClockFrequency), true));
		orgDisableCDClock = reinterpret_cast<decltype(orgDisableCDClock)>(dcdcAddress);
		orgSetCDClockFrequency = reinterpret_cast<decltype(orgSetCDClockFrequency)>(scdcAddress);
		orgIclReadRegister32 = callbackIGFX->AppleIntelFramebufferController__ReadRegister32;
		if (orgProbeCDClockFrequency && orgIclReadRegister32 && orgDisableCDClock && orgSetCDClockFrequency) {
			DBGLOG("igfx", "CDC: Functions have been routed successfully.");
		} else {
			patcher.clearError();
			SYSLOG("igfx", "CDC: Failed to route functions.");
		}
	} else {
		SYSLOG("igfx", "CDC: Failed to find symbols.");
		patcher.clearError();
	}
}

void IGFX::CoreDisplayClockFix::sanitizeCDClockFrequency(void *that) {
	// Read the hardware reference frequency from the DSSM register
	// Bits 29-31 store the reference frequency value
	auto referenceFrequency = callbackIGFX->modCoreDisplayClockFix.orgIclReadRegister32(that, ICL_REG_DSSM) >> 29;
	
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
	auto cdclk = callbackIGFX->modCoreDisplayClockFix.orgIclReadRegister32(that, ICL_REG_CDCLK_CTL) & 0x7FF;
	SYSLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock frequency is %s MHz now.",
		   coreDisplayClockDecimalFrequency2String(cdclk));
}

uint32_t IGFX::CoreDisplayClockFix::wrapProbeCDClockFrequency(void *that) {
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
	auto cdclk = callbackIGFX->modCoreDisplayClockFix.orgIclReadRegister32(that, ICL_REG_CDCLK_CTL) & 0x7FF;
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
