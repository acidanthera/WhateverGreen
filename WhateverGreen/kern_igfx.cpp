//
//  kern_igfx.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"
#include "kern_fb.hpp"
#include "kern_guc.hpp"
#include "kern_agdc.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_file.hpp>
#include <Headers/kern_iokit.hpp>

static const char *pathIntelHD[]      { "/System/Library/Extensions/AppleIntelHDGraphics.kext/Contents/MacOS/AppleIntelHDGraphics" };
static const char *pathIntelHDFb[]    { "/System/Library/Extensions/AppleIntelHDGraphicsFB.kext/Contents/MacOS/AppleIntelHDGraphicsFB" };
static const char *pathIntelHD3000[]  { "/System/Library/Extensions/AppleIntelHD3000Graphics.kext/Contents/MacOS/AppleIntelHD3000Graphics" };
static const char *pathIntelSNBFb[]   { "/System/Library/Extensions/AppleIntelSNBGraphicsFB.kext/Contents/MacOS/AppleIntelSNBGraphicsFB" };
static const char *pathIntelHD4000[]  { "/System/Library/Extensions/AppleIntelHD4000Graphics.kext/Contents/MacOS/AppleIntelHD4000Graphics" };
static const char *pathIntelCapriFb[] { "/System/Library/Extensions/AppleIntelFramebufferCapri.kext/Contents/MacOS/AppleIntelFramebufferCapri" };
static const char *pathIntelHD5000[]  { "/System/Library/Extensions/AppleIntelHD5000Graphics.kext/Contents/MacOS/AppleIntelHD5000Graphics" };
static const char *pathIntelAzulFb[]  { "/System/Library/Extensions/AppleIntelFramebufferAzul.kext/Contents/MacOS/AppleIntelFramebufferAzul" };
static const char *pathIntelBDW[]     { "/System/Library/Extensions/AppleIntelBDWGraphics.kext/Contents/MacOS/AppleIntelBDWGraphics" };
static const char *pathIntelBDWFb[]   { "/System/Library/Extensions/AppleIntelBDWGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelBDWGraphicsFramebuffer" };
static const char *pathIntelSKL[]     { "/System/Library/Extensions/AppleIntelSKLGraphics.kext/Contents/MacOS/AppleIntelSKLGraphics" };
static const char *pathIntelSKLFb[]   { "/System/Library/Extensions/AppleIntelSKLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelSKLGraphicsFramebuffer" };
static const char *pathIntelKBL[]     { "/System/Library/Extensions/AppleIntelKBLGraphics.kext/Contents/MacOS/AppleIntelKBLGraphics" };
static const char *pathIntelKBLFb[]   { "/System/Library/Extensions/AppleIntelKBLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelKBLGraphicsFramebuffer" };
static const char *pathIntelCFLFb[]   { "/System/Library/Extensions/AppleIntelCFLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelCFLGraphicsFramebuffer" };
static const char *pathIntelCNL[]     { "/System/Library/Extensions/AppleIntelCNLGraphics.kext/Contents/MacOS/AppleIntelCNLGraphics" };
static const char *pathIntelCNLFb[]   { "/System/Library/Extensions/AppleIntelCNLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelCNLGraphicsFramebuffer" };
static const char *pathIntelICL[]     { "/System/Library/Extensions/AppleIntelICLGraphics.kext/Contents/MacOS/AppleIntelICLGraphics" };
static const char *pathIntelICLLPFb[] { "/System/Library/Extensions/AppleIntelICLLPGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelICLLPGraphicsFramebuffer" };
static const char *pathIntelICLHPFb[] { "/System/Library/Extensions/AppleIntelICLHPGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelICLHPGraphicsFramebuffer" };

static KernelPatcher::KextInfo kextIntelHD      { "com.apple.driver.AppleIntelHDGraphics", pathIntelHD, arrsize(pathIntelHD), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelHDFb    { "com.apple.driver.AppleIntelHDGraphicsFB", pathIntelHDFb, arrsize(pathIntelHDFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelHD3000  { "com.apple.driver.AppleIntelHD3000Graphics", pathIntelHD3000, arrsize(pathIntelHD3000), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelSNBFb   { "com.apple.driver.AppleIntelSNBGraphicsFB", pathIntelSNBFb, arrsize(pathIntelSNBFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelHD4000  { "com.apple.driver.AppleIntelHD4000Graphics", pathIntelHD4000, arrsize(pathIntelHD4000), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelCapriFb { "com.apple.driver.AppleIntelFramebufferCapri", pathIntelCapriFb, arrsize(pathIntelCapriFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelHD5000  { "com.apple.driver.AppleIntelHD5000Graphics", pathIntelHD5000, arrsize(pathIntelHD5000), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelAzulFb  { "com.apple.driver.AppleIntelFramebufferAzul", pathIntelAzulFb, arrsize(pathIntelAzulFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelBDW     { "com.apple.driver.AppleIntelBDWGraphics", pathIntelBDW, arrsize(pathIntelBDW), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelBDWFb   { "com.apple.driver.AppleIntelBDWGraphicsFramebuffer", pathIntelBDWFb, arrsize(pathIntelBDWFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelSKL     { "com.apple.driver.AppleIntelSKLGraphics", pathIntelSKL, arrsize(pathIntelSKL), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelSKLFb   { "com.apple.driver.AppleIntelSKLGraphicsFramebuffer", pathIntelSKLFb, arrsize(pathIntelSKLFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelKBL     { "com.apple.driver.AppleIntelKBLGraphics", pathIntelKBL, arrsize(pathIntelKBL), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelKBLFb   { "com.apple.driver.AppleIntelKBLGraphicsFramebuffer", pathIntelKBLFb, arrsize(pathIntelKBLFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelCFLFb   { "com.apple.driver.AppleIntelCFLGraphicsFramebuffer", pathIntelCFLFb, arrsize(pathIntelCFLFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelCNL     { "com.apple.driver.AppleIntelCNLGraphics", pathIntelCNL, arrsize(pathIntelCNL), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelCNLFb   { "com.apple.driver.AppleIntelCNLGraphicsFramebuffer", pathIntelCNLFb, arrsize(pathIntelCNLFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelICL     { "com.apple.driver.AppleIntelICLGraphics", pathIntelICL, arrsize(pathIntelICL), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelICLLPFb { "com.apple.driver.AppleIntelICLLPGraphicsFramebuffer", pathIntelICLLPFb, arrsize(pathIntelICLLPFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelICLHPFb { "com.apple.driver.AppleIntelICLHPGraphicsFramebuffer", pathIntelICLHPFb, arrsize(pathIntelICLHPFb), {}, {}, KernelPatcher::KextInfo::Unloaded };

IGFX *IGFX::callbackIGFX;

void IGFX::init() {
	callbackIGFX = this;
	// Initialize each submodule
	modDVMTCalcFix.init();
	auto &bdi = BaseDeviceInfo::get();
	auto generation = bdi.cpuGeneration;
	auto family = bdi.cpuFamily;
	auto model = bdi.cpuModel;
	switch (generation) {
		case CPUInfo::CpuGeneration::Penryn:
		case CPUInfo::CpuGeneration::Nehalem:
			// Do not warn about legacy processors (e.g. Xeon).
			break;
		case CPUInfo::CpuGeneration::Westmere:
			currentGraphics = &kextIntelHD;
			currentFramebuffer = &kextIntelHDFb;
			break;
		case CPUInfo::CpuGeneration::SandyBridge: {
			int tmp = 1;
			PE_parse_boot_argn("igfxsnb", &tmp, sizeof(tmp));
			moderniseAccelerator = tmp == 1;
			currentGraphics = &kextIntelHD3000;
			currentFramebuffer = &kextIntelSNBFb;
			break;
		}
		case CPUInfo::CpuGeneration::IvyBridge:
			currentGraphics = &kextIntelHD4000;
			currentFramebuffer = &kextIntelCapriFb;
			break;
		case CPUInfo::CpuGeneration::Haswell:
			currentGraphics = &kextIntelHD5000;
			currentFramebuffer = &kextIntelAzulFb;
			break;
		case CPUInfo::CpuGeneration::Broadwell:
			currentGraphics = &kextIntelBDW;
			currentFramebuffer = &kextIntelBDWFb;
			break;
		case CPUInfo::CpuGeneration::Skylake:
			supportsGuCFirmware = true;
			currentGraphics = &kextIntelSKL;
			currentFramebuffer = &kextIntelSKLFb;
			forceCompleteModeset.supported = forceCompleteModeset.legacy = true; // not enabled, as on legacy operating systems it casues crashes.
			break;
		case CPUInfo::CpuGeneration::KabyLake:
			supportsGuCFirmware = true;
			currentGraphics = &kextIntelKBL;
			currentFramebuffer = &kextIntelKBLFb;
			forceCompleteModeset.supported = forceCompleteModeset.enable = true;
			RPSControl.available = true;
			ForceWakeWorkaround.enabled = true;
			disableTypeCCheck = true;
			break;
		case CPUInfo::CpuGeneration::CoffeeLake:
			supportsGuCFirmware = true;
			currentGraphics = &kextIntelKBL;
			currentFramebuffer = &kextIntelCFLFb;
			// Allow faking ask KBL
			currentFramebufferOpt = &kextIntelKBLFb;
			// Note, several CFL GPUs are completely broken. They freeze in IGMemoryManager::initCache due to incompatible
			// configuration, supposedly due to Apple not supporting new MOCS table and forcing Skylake-based format.
			// See: https://github.com/torvalds/linux/blob/135c5504a600ff9b06e321694fbcac78a9530cd4/drivers/gpu/drm/i915/intel_mocs.c#L181
			forceCompleteModeset.supported = forceCompleteModeset.enable = true;
			RPSControl.available = true;
			ForceWakeWorkaround.enabled = true;
			disableTypeCCheck = true;
			break;
		case CPUInfo::CpuGeneration::CannonLake:
			supportsGuCFirmware = true;
			currentGraphics = &kextIntelCNL;
			currentFramebuffer = &kextIntelCNLFb;
			forceCompleteModeset.supported = forceCompleteModeset.enable = true;
			disableTypeCCheck = true;
			break;
		case CPUInfo::CpuGeneration::IceLake:
			supportsGuCFirmware = true;
			currentGraphics = &kextIntelICL;
			currentFramebuffer = &kextIntelICLLPFb;
			currentFramebufferOpt = &kextIntelICLHPFb;
			forceCompleteModeset.supported = forceCompleteModeset.enable = true;
			disableTypeCCheck = true;
			modDVMTCalcFix.available = true;
			break;
		case CPUInfo::CpuGeneration::CometLake:
			supportsGuCFirmware = true;
			currentGraphics = &kextIntelKBL;
			currentFramebuffer = &kextIntelCFLFb;
			// Allow faking ask KBL
			currentFramebufferOpt = &kextIntelKBLFb;
			// Note, several CFL GPUs are completely broken. They freeze in IGMemoryManager::initCache due to incompatible
			// configuration, supposedly due to Apple not supporting new MOCS table and forcing Skylake-based format.
			// See: https://github.com/torvalds/linux/blob/135c5504a600ff9b06e321694fbcac78a9530cd4/drivers/gpu/drm/i915/intel_mocs.c#L181
			forceCompleteModeset.supported = forceCompleteModeset.enable = true;
			disableTypeCCheck = true;
			break;
		default:
			SYSLOG("igfx", "found an unsupported processor 0x%X:0x%X, please report this!", family, model);
			break;
	}

	if (currentGraphics)
		lilu.onKextLoadForce(currentGraphics);

	if (currentFramebuffer)
		lilu.onKextLoadForce(currentFramebuffer);

	if (currentFramebufferOpt)
		lilu.onKextLoadForce(currentFramebufferOpt);
}

void IGFX::deinit() {
	for (auto &con : lspcons) {
		LSPCON::deleter(con.lspcon);
		con.lspcon = nullptr;
	}
}

void IGFX::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	bool switchOffGraphics = false;
	bool switchOffFramebuffer = false;
	framebufferPatch.framebufferId = info->reportedFramebufferId;

	auto cpuGeneration = BaseDeviceInfo::get().cpuGeneration;

	if (info->videoBuiltin) {
		applyFramebufferPatch = loadPatchesFromDevice(info->videoBuiltin, info->reportedFramebufferId);

#ifdef DEBUG
		dumpFramebufferToDisk = checkKernelArgument("-igfxdump");
		dumpPlatformTable = checkKernelArgument("-igfxfbdump");
		debugFramebuffer = checkKernelArgument("-igfxfbdbg");
#endif
		
		uint32_t rpsc = 0;
		if (PE_parse_boot_argn("igfxrpsc", &rpsc, sizeof(rpsc)) ||
			WIOKit::getOSDataValue(info->videoBuiltin, "rps-control", rpsc)) {
			RPSControl.enabled = rpsc > 0 && RPSControl.available;
			DBGLOG("weg", "RPS control patch overriden (%u) availabile %d", rpsc, RPSControl.available);
		}

		uint32_t forceCompleteModeSet = 0;
		if (PE_parse_boot_argn("igfxfcms", &forceCompleteModeSet, sizeof(forceCompleteModeSet))) {
			forceCompleteModeset.enable = forceCompleteModeset.supported && forceCompleteModeSet != 0;
			DBGLOG("weg", "force complete-modeset overriden by boot-argument %u -> %d", forceCompleteModeSet, forceCompleteModeset.enable);
		} else if (WIOKit::getOSDataValue(info->videoBuiltin, "complete-modeset", forceCompleteModeSet)) {
			forceCompleteModeset.enable = forceCompleteModeset.supported && forceCompleteModeSet != 0;
			DBGLOG("weg", "force complete-modeset overriden by device property %u -> %d", forceCompleteModeSet, forceCompleteModeset.enable);
		} else if (info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple) {
			forceCompleteModeset.enable = false; // may interfere with FV2
			DBGLOG("weg", "force complete-modeset overriden by Apple firmware -> %d", forceCompleteModeset.enable);
		}

		if (forceCompleteModeset.enable) {
			uint64_t fbs;
			if (PE_parse_boot_argn("igfxfcmsfbs", &fbs, sizeof(fbs)) ||
				WIOKit::getOSDataValue(info->videoBuiltin, "complete-modeset-framebuffers", fbs)) {
				for (size_t i = 0; i < arrsize(forceCompleteModeset.fbs); i++)
					forceCompleteModeset.fbs[i] = (fbs >> (8 * i)) & 0xffU;
				forceCompleteModeset.customised = true;
			}
		}

		uint32_t forceOnline = 0;
		if (PE_parse_boot_argn("igfxonln", &forceOnline, sizeof(forceOnline))) {
			forceOnlineDisplay.enable = forceOnline != 0;
			DBGLOG("weg", "force online overriden by boot-argument %u", forceOnline);
		} else if (WIOKit::getOSDataValue(info->videoBuiltin, "force-online", forceOnline)) {
			forceOnlineDisplay.enable = forceOnline != 0;
			DBGLOG("weg", "force online overriden by device property %u", forceOnline);
		}

		if (forceOnlineDisplay.enable) {
			uint64_t fbs;
			if (PE_parse_boot_argn("igfxonlnfbs", &fbs, sizeof(fbs)) ||
				WIOKit::getOSDataValue(info->videoBuiltin, "force-online-framebuffers", fbs)) {
				for (size_t i = 0; i < arrsize(forceOnlineDisplay.fbs); i++)
					forceOnlineDisplay.fbs[i] = (fbs >> (8 * i)) & 0xffU;
				forceOnlineDisplay.customised = true;
			}
		}

		if (supportsGuCFirmware && getKernelVersion() >= KernelVersion::HighSierra) {
			if (!PE_parse_boot_argn("igfxfw", &fwLoadMode, sizeof(fwLoadMode)))
				WIOKit::getOSDataValue<int32_t>(info->videoBuiltin, "igfxfw", fwLoadMode);
			if (fwLoadMode == FW_AUTO)
				fwLoadMode = info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple ? FW_APPLE : FW_DISABLE;
		} else {
			fwLoadMode = FW_APPLE; /* Do nothing, GuC is either unsupported due to low OS or Apple */
		}

		// Enable the fix for computing HDMI dividers on SKL, KBL, CFL platforms if the corresponding boot argument is found
		hdmiP0P1P2Patch = checkKernelArgument("-igfxhdmidivs");
		// Of if "enable-hdmi-dividers-fix" is set in IGPU property
		if (!hdmiP0P1P2Patch)
			hdmiP0P1P2Patch = info->videoBuiltin->getProperty("enable-hdmi-dividers-fix") != nullptr;

		// Enable the LSPCON driver support if the corresponding boot argument is found
		supportLSPCON = checkKernelArgument("-igfxlspcon");
		// Or if "enable-lspcon-support" is set in IGPU property
		if (!supportLSPCON)
			supportLSPCON = info->videoBuiltin->getProperty("enable-lspcon-support") != nullptr;

		// Read the user-defined IGPU properties to know whether a connector has an onboard LSPCON chip
		if (supportLSPCON) {
			char name[48];
			uint32_t pmode = 0x01; // PCON mode as a fallback value
			for (size_t index = 0; index < arrsize(lspcons); index++) {
				bzero(name, sizeof(name));
				snprintf(name, sizeof(name), "framebuffer-con%lu-has-lspcon", index);
				(void)WIOKit::getOSDataValue(info->videoBuiltin, name, lspcons[index].hasLSPCON);
				snprintf(name, sizeof(name), "framebuffer-con%lu-preferred-lspcon-mode", index);
				(void)WIOKit::getOSDataValue(info->videoBuiltin, name, pmode);
				// Assuming PCON mode if invalid mode value (i.e. > 1) specified by the user
				lspcons[index].preferredMode = LSPCON::parseMode(pmode != 0);
			}
		}

		// Enable the verbose output in I2C-over-AUX transactions if the corresponding boot argument is found
		verboseI2C = checkKernelArgument("-igfxi2cdbg");

		// Enable maximum link rate patch if the corresponding boot argument is found
		maxLinkRatePatch = checkKernelArgument("-igfxmlr");
		// Or if "enable-dpcd-max-link-rate-fix" is set in IGPU property
		if (!maxLinkRatePatch)
			maxLinkRatePatch = info->videoBuiltin->getProperty("enable-dpcd-max-link-rate-fix") != nullptr;
		
		// Enable the Core Display Clock patch on ICL platforms
		coreDisplayClockPatch = checkKernelArgument("-igfxcdc");
		// Or if `enable-cdclk-frequency-fix` is set in IGPU property
		if (!coreDisplayClockPatch)
			coreDisplayClockPatch = info->videoBuiltin->getProperty("enable-cdclk-frequency-fix") != nullptr;
		
		// Example of redirecting the request to each submodule
		modDVMTCalcFix.processKernel(patcher, info);
		
		disableAccel = checkKernelArgument("-igfxvesa");
		
		disableTypeCCheck &= !checkKernelArgument("-igfxtypec");

		// Read the custom maximum link rate if present
		if (WIOKit::getOSDataValue(info->videoBuiltin, "dpcd-max-link-rate", maxLinkRate)) {
			// Guard: Verify the custom link rate before using it
			switch (maxLinkRate) {
				case 0x1E: // HBR3 8.1  Gbps
				case 0x14: // HBR2 5.4  Gbps
				case 0x0C: // 3_24 3.24 Gbps Used by Apple internally
				case 0x0A: // HBR  2.7  Gbps
				case 0x06: // RBR  1.62 Gbps
					DBGLOG("igfx", "MLR: Found a valid custom maximum link rate value 0x%02x", maxLinkRate);
					break;

				default:
					// Invalid link rate value
					SYSLOG("igfx", "MLR: Found an invalid custom maximum link rate value. Will use 0x14 as a fallback value.");
					maxLinkRate = 0x14;
					break;
			}
		} else {
			DBGLOG("igfx", "MLR: No custom max link rate specified. Will use 0x14 as the default value.");
		}

		// Enable CFL backlight patch on mobile CFL or if IGPU propery enable-cfl-backlight-fix is set
		int bkl = 0;
		if (PE_parse_boot_argn("igfxcflbklt", &bkl, sizeof(bkl)))
			cflBacklightPatch = bkl == 1 ? CoffeeBacklightPatch::On : CoffeeBacklightPatch::Off;
		else if (info->videoBuiltin->getProperty("enable-cfl-backlight-fix"))
			cflBacklightPatch = CoffeeBacklightPatch::On;
		else if (currentFramebuffer == &kextIntelCFLFb && BaseDeviceInfo::get().modelType == WIOKit::ComputerModel::ComputerLaptop)
			cflBacklightPatch = CoffeeBacklightPatch::Auto;

		// Enable new backlight patch type on KBL and older if the corresponding boot argument is found
		newBacklightPatch = checkKernelArgument("-igfxnewbklt");
		// Or if "enable-new-backlight-fix" is set in IGPU property
		if (!newBacklightPatch)
			newBacklightPatch = info->videoBuiltin->getProperty("enable-new-backlight-fix") != nullptr;
		// New backlight patch is only for KBL and older
		if (newBacklightPatch && cpuGeneration > CPUInfo::CpuGeneration::KabyLake)
			SYSLOG("igfx", "new backlight patch is only for KBL and older. CFL and above must use another patch.");

		if (WIOKit::getOSDataValue(info->videoBuiltin, "max-backlight-freq", targetBacklightFrequency))
			DBGLOG("igfx", "read custom backlight frequency %u", targetBacklightFrequency);

		bool connectorLessFrame = info->reportedFramebufferIsConnectorLess;

		// Black screen (ComputeLaneCount) happened from 10.12.4
		// It only affects SKL, KBL, and CFL drivers with a frame with connectors.
		if (!connectorLessFrame && cpuGeneration >= CPUInfo::CpuGeneration::Skylake &&
			((getKernelVersion() == KernelVersion::Sierra && getKernelMinorVersion() >= 5) || getKernelVersion() >= KernelVersion::HighSierra)) {
			blackScreenPatch = info->firmwareVendor != DeviceInfo::FirmwareVendor::Apple;
		}

		// PAVP patch is only necessary when we have no discrete GPU.
		int pavpMode = connectorLessFrame || info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple;
		if (!PE_parse_boot_argn("igfxpavp", &pavpMode, sizeof(pavpMode)))
			WIOKit::getOSDataValue(info->videoBuiltin, "igfxpavp", pavpMode);
		pavpDisablePatch = pavpMode == 0 && cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge;

		int gl = info->videoBuiltin->getProperty("disable-metal") != nullptr;
		PE_parse_boot_argn("igfxgl", &gl, sizeof(gl));
		forceOpenGL = gl == 1;

		int metal = info->videoBuiltin->getProperty("enable-metal") != nullptr;
		PE_parse_boot_argn("igfxmetal", &metal, sizeof(metal));
		forceMetal = metal == 1;

		int agdc = info->videoBuiltin->getProperty("disable-agdc") != nullptr ? 0 : 1;
		PE_parse_boot_argn("igfxagdc", &agdc, sizeof(agdc));
		disableAGDC = agdc == 0;

		// Starting from 10.14.4b1 Skylake+ graphics randomly kernel panics on GPU usage
		readDescriptorPatch = cpuGeneration >= CPUInfo::CpuGeneration::Skylake && getKernelVersion() >= KernelVersion::Mojave;

		// Automatically enable HDMI -> DP patches
		bool nohdmi = info->videoBuiltin->getProperty("disable-hdmi-patches") != nullptr;
		hdmiAutopatch = !applyFramebufferPatch && !connectorLessFrame && getKernelVersion() >= Yosemite &&
			cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge && !checkKernelArgument("-igfxnohdmi") && !nohdmi;
		
		// Set AAPL00,DualLink to zero on Westmere if applying single link patches.
		if (cpuGeneration == CPUInfo::CpuGeneration::Westmere && framebufferWestmerePatches.SingleLink) {
			uint8_t dualLinkBytes[] { 0x00, 0x00, 0x00, 0x00 };
			info->videoBuiltin->setProperty("AAPL00,DualLink", dualLinkBytes, sizeof(dualLinkBytes));
		}

		auto requiresFramebufferPatches = [this]() {
			if (blackScreenPatch)
				return true;
			if (applyFramebufferPatch || hdmiAutopatch)
				return true;
			if (dumpFramebufferToDisk || dumpPlatformTable || debugFramebuffer)
				return true;
			if (cflBacklightPatch != CoffeeBacklightPatch::Off)
				return true;
			if (newBacklightPatch)
				return true;
			if (maxLinkRatePatch)
				return true;
			if (hdmiP0P1P2Patch)
				return true;
			if (supportLSPCON)
				return true;
			if (coreDisplayClockPatch)
				return true;
			// Similarly, if IGFX maintains a sequence of submodules,
			// we could iterate through each submodule and performs OR operations.
			if (modDVMTCalcFix.requiresPatchingFramebuffer)
				return true;
			if (forceCompleteModeset.enable)
				return true;
			if (forceOnlineDisplay.enable)
				return true;
			if (disableAGDC)
				return true;
			if (RPSControl.enabled)
				return true;
			if (ForceWakeWorkaround.enabled)
				return true;
			if (disableTypeCCheck)
				return true;
			return false;
		};

		auto requiresGraphicsPatches = [this]() {
			if (modDVMTCalcFix.requiresPatchingGraphics)
				return true;
			if (pavpDisablePatch)
				return true;
			if (forceOpenGL)
				return true;
			if (forceMetal)
				return true;
			if (moderniseAccelerator)
				return true;
			if (fwLoadMode != FW_APPLE)
				return true;
			if (readDescriptorPatch)
				return true;
			if (RPSControl.enabled)
				return true;
			if (disableAccel)
				return true;
			return false;
		};

		// Disable kext patching if we have nothing to do.
		switchOffFramebuffer = !requiresFramebufferPatches();
		switchOffGraphics = !requiresGraphicsPatches();
	} else {
		switchOffGraphics = switchOffFramebuffer = true;
	}

	if (switchOffGraphics && currentGraphics)
		currentGraphics->switchOff();

	if (switchOffFramebuffer) {
		if (currentFramebuffer)
			currentFramebuffer->switchOff();
		if (currentFramebufferOpt)
			currentFramebufferOpt->switchOff();
	}

	if (moderniseAccelerator) {
		KernelPatcher::RouteRequest request("__ZN9IOService20copyExistingServicesEP12OSDictionaryjj", wrapCopyExistingServices, orgCopyExistingServices);
		patcher.routeMultiple(KernelPatcher::KernelID, &request, 1);
	}
}

bool IGFX::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	auto cpuGeneration = BaseDeviceInfo::get().cpuGeneration;

	if (currentGraphics && currentGraphics->loadIndex == index) {
		if (pavpDisablePatch) {
			auto callbackSym = "__ZN16IntelAccelerator19PAVPCommandCallbackE22PAVPSessionCommandID_tjPjb";
			if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge)
				callbackSym = "__ZN15Gen6Accelerator19PAVPCommandCallbackE22PAVPSessionCommandID_t18PAVPSessionAppID_tPjb";
			else if (cpuGeneration == CPUInfo::CpuGeneration::IvyBridge)
				callbackSym = "__ZN16IntelAccelerator19PAVPCommandCallbackE22PAVPSessionCommandID_t18PAVPSessionAppID_tPjb";

			KernelPatcher::RouteRequest request(callbackSym, wrapPavpSessionCallback, orgPavpSessionCallback);
			patcher.routeMultiple(index, &request, 1, address, size);
		}

		if (forceOpenGL || forceMetal || moderniseAccelerator || fwLoadMode != FW_APPLE || disableAccel) {
			KernelPatcher::RouteRequest request("__ZN16IntelAccelerator5startEP9IOService", wrapAcceleratorStart, orgAcceleratorStart);
			patcher.routeMultiple(index, &request, 1, address, size);

			if (fwLoadMode == FW_GENERIC && getKernelVersion() <= KernelVersion::Mojave)
				loadIGScheduler4Patches(patcher, index, address, size);
		}

		if (readDescriptorPatch) {
			KernelPatcher::RouteRequest request("__ZNK25IGHardwareGlobalPageTable4readEyRyS0_", globalPageTableRead);
			patcher.routeMultiple(index, &request, 1, address, size);
		}

		if (RPSControl.enabled)
			RPSControl.initGraphics(patcher, index, address, size);
		
		if (ForceWakeWorkaround.enabled)
			ForceWakeWorkaround.initGraphics(*this, patcher, index, address, size);
		
		if (modDVMTCalcFix.enabled)
			modDVMTCalcFix.processGraphicsKext(patcher, index, address, size);

		return true;
	}

	if ((currentFramebuffer && currentFramebuffer->loadIndex == index) ||
		(currentFramebufferOpt && currentFramebufferOpt->loadIndex == index)) {
		// Find actual framebuffer used (kaby or coffee)
		auto realFramebuffer = (currentFramebuffer && currentFramebuffer->loadIndex == index) ? currentFramebuffer : currentFramebufferOpt;
		// Accept Coffee FB and enable backlight patches unless Off (Auto turns them on by default).
		bool bklCoffeeFb = realFramebuffer == &kextIntelCFLFb && cflBacklightPatch != CoffeeBacklightPatch::Off;
		// Accept Kaby FB and enable backlight patches if On (Auto is irrelevant here).
		bool bklKabyFb = realFramebuffer == &kextIntelKBLFb && cflBacklightPatch == CoffeeBacklightPatch::On;
		// Solve ReadRegister32 just once as it is shared
		if (bklCoffeeFb || bklKabyFb || newBacklightPatch ||
			RPSControl.enabled || ForceWakeWorkaround.enabled || coreDisplayClockPatch) {
			AppleIntelFramebufferController__ReadRegister32 = patcher.solveSymbol<decltype(AppleIntelFramebufferController__ReadRegister32)>
			(index, "__ZN31AppleIntelFramebufferController14ReadRegister32Em", address, size);
			if (!AppleIntelFramebufferController__ReadRegister32)
				SYSLOG("igfx", "Failed to find ReadRegister32");
		}
		if (bklCoffeeFb || bklKabyFb || newBacklightPatch ||
			RPSControl.enabled || ForceWakeWorkaround.enabled) {
			AppleIntelFramebufferController__WriteRegister32 = patcher.solveSymbol<decltype(AppleIntelFramebufferController__WriteRegister32)>
			(index, "__ZN31AppleIntelFramebufferController15WriteRegister32Emj", address, size);
			if (!AppleIntelFramebufferController__WriteRegister32)
				SYSLOG("igfx", "Failed to find WriteRegister32");
		}
		if (RPSControl.enabled || ForceWakeWorkaround.enabled)
			gFramebufferController = patcher.solveSymbol<decltype(gFramebufferController)>(index, "_gController", address, size);
		if (bklCoffeeFb || bklKabyFb) {
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

			if (AppleIntelFramebufferController__ReadRegister32 &&
				AppleIntelFramebufferController__WriteRegister32) {
				(bklCoffeeFb ? orgCflReadRegister32 : orgKblReadRegister32) = AppleIntelFramebufferController__ReadRegister32;

				patcher.eraseCoverageInstPrefix(reinterpret_cast<mach_vm_address_t>(AppleIntelFramebufferController__WriteRegister32));
				auto orgRegWrite = reinterpret_cast<decltype(orgCflWriteRegister32)>
					(patcher.routeFunction(reinterpret_cast<mach_vm_address_t>(AppleIntelFramebufferController__WriteRegister32), reinterpret_cast<mach_vm_address_t>(bklCoffeeFb ? wrapCflWriteRegister32 : wrapKblWriteRegister32), true));

				if (orgRegWrite) {
					(bklCoffeeFb ? orgCflWriteRegister32 : orgKblWriteRegister32) = orgRegWrite;
				} else {
					SYSLOG("igfx", "failed to route WriteRegister32 for cfl %d", bklCoffeeFb);
					patcher.clearError();
				}
			} else {
				SYSLOG("igfx", "failed to find ReadRegister32 for cfl %d", bklCoffeeFb);
				patcher.clearError();
			}
		}

		if (newBacklightPatch) {
			// The traditional way to get backlight working on Kaby Lake and older is using SSDT-PNLF to set custom PWM frequency at boot to match the value used by macOS.
			// This patch uses a different approach based on the Coffee Lake backlight patch (see above for more details).
			// Basically, it overwrites WriteRegister32 function to rescale all the register writes to match the default PWM frequency.
			// As a result, only a simple SSDT-PNLF is needed (see SSDT-PNLFCFL).
			//
			// Ivy Bridge and before use two registers: BXT_BLC_PWM_FREQ1 (for frequency) and BLC_PWM_CPU_CTL (for duty cycle)
			// Haswell to Kaby Lake use only one register: BXT_BLC_PWM_FREQ1 for both frequency and duty cycle
			// See wrapHswWriteRegister32 and wrapIvyWriteRegister32 for more details

			bool hswPlus = cpuGeneration >= CPUInfo::CpuGeneration::Haswell;
			if (AppleIntelFramebufferController__ReadRegister32 &&
				AppleIntelFramebufferController__WriteRegister32) {
				(hswPlus ? orgHswReadRegister32 : orgIvyReadRegister32) = AppleIntelFramebufferController__ReadRegister32;

				patcher.eraseCoverageInstPrefix(reinterpret_cast<mach_vm_address_t>(AppleIntelFramebufferController__WriteRegister32));
				auto orgRegWrite = reinterpret_cast<decltype(orgHswWriteRegister32)>
					(patcher.routeFunction(reinterpret_cast<mach_vm_address_t>(AppleIntelFramebufferController__WriteRegister32), reinterpret_cast<mach_vm_address_t>(hswPlus ? wrapHswWriteRegister32 : wrapIvyWriteRegister32), true));

				if (orgRegWrite) {
					(hswPlus ? orgHswWriteRegister32 : orgIvyWriteRegister32) = orgRegWrite;
				} else {
					SYSLOG("igfx", "failed to route WriteRegister32 for hsw %d", hswPlus);
					patcher.clearError();
				}
			} else {
				SYSLOG("igfx", "failed to find ReadRegister32/WriteRegister32 for hsw %d", hswPlus);
				patcher.clearError();
			}
		}

		if (maxLinkRatePatch) {
			auto readAUXAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController7ReadAUXEP21AppleIntelFramebufferjtPvP21AppleIntelDisplayPath", address, size);
			if (readAUXAddress) {
				patcher.eraseCoverageInstPrefix(readAUXAddress);
				orgReadAUX = reinterpret_cast<decltype(orgReadAUX)>(patcher.routeFunction(readAUXAddress, reinterpret_cast<mach_vm_address_t>(wrapReadAUX), true));
				if (orgReadAUX) {
					DBGLOG("igfx", "MLR: ReadAUX() has been routed successfully");
				} else {
					patcher.clearError();
					SYSLOG("igfx", "MLR: Failed to route ReadAUX()");
				}
			} else {
				SYSLOG("igfx", "MLR: Failed to find ReadAUX()");
				patcher.clearError();
			}
		}
		
		if (coreDisplayClockPatch) {
			auto pcdcAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController21probeCDClockFrequencyEv", address, size);
			auto dcdcAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController14disableCDClockEv", address, size);
			auto scdcAddress = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController19setCDClockFrequencyEy", address, size);
			if (pcdcAddress && dcdcAddress && scdcAddress && AppleIntelFramebufferController__ReadRegister32) {
				patcher.eraseCoverageInstPrefix(pcdcAddress);
				orgProbeCDClockFrequency = reinterpret_cast<decltype(orgProbeCDClockFrequency)>(patcher.routeFunction(pcdcAddress, reinterpret_cast<mach_vm_address_t>(wrapProbeCDClockFrequency), true));
				orgDisableCDClock = reinterpret_cast<decltype(orgDisableCDClock)>(dcdcAddress);
				orgSetCDClockFrequency = reinterpret_cast<decltype(orgSetCDClockFrequency)>(scdcAddress);
				orgIclReadRegister32 = AppleIntelFramebufferController__ReadRegister32;
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

		// We could iterate through each submodule and redirect the request if and only if the submodule is enabled
		if (modDVMTCalcFix.enabled)
			modDVMTCalcFix.processFramebufferKext(patcher, index, address, size);
		
		if (forceCompleteModeset.enable) {
			const char *sym = "__ZN31AppleIntelFramebufferController16hwRegsNeedUpdateEP21AppleIntelFramebufferP21AppleIntelDisplayPathPNS_10CRTCParamsEPK29IODetailedTimingInformationV2";
			if (forceCompleteModeset.legacy)
				sym = "__ZN31AppleIntelFramebufferController16hwRegsNeedUpdateEP21AppleIntelFramebufferP21AppleIntelDisplayPathPNS_10CRTCParamsE";
			KernelPatcher::RouteRequest request(sym, wrapHwRegsNeedUpdate, orgHwRegsNeedUpdate);
			if (!patcher.routeMultiple(index, &request, 1, address, size))
				SYSLOG("igfx", "failed to route hwRegsNeedUpdate");
		}

		if (forceOnlineDisplay.enable) {
			KernelPatcher::RouteRequest request("__ZN21AppleIntelFramebuffer16getDisplayStatusEP21AppleIntelDisplayPath", wrapGetDisplayStatus, orgGetDisplayStatus);
			if (!patcher.routeMultiple(index, &request, 1, address, size))
				SYSLOG("igfx", "failed to route getDisplayStatus");
		}
		
		if (disableTypeCCheck) {
			KernelPatcher::RouteRequest req("__ZN31AppleIntelFramebufferController17IsTypeCOnlySystemEv", wrapIsTypeCOnlySystem);
			if (!patcher.routeMultiple(index, &req, 1, address, size))
				SYSLOG("igfx", "failed to route IsTypeCOnlySystem");
		}
		
		if (RPSControl.enabled)
			RPSControl.initFB(*this, patcher, index, address, size);

		if (disableAGDC) {
			KernelPatcher::RouteRequest request {"__ZN20IntelFBClientControl11doAttributeEjPmmS0_S0_P25IOExternalMethodArguments", wrapFBClientDoAttribute, orgFBClientDoAttribute};
			if (!patcher.routeMultiple(index, &request, 1, address, size))
				SYSLOG("igfx", "failed to route FBClientControl::doAttribute");
		}

		if (debugFramebuffer)
			loadFramebufferDebug(patcher, index, address, size);

		if (hdmiP0P1P2Patch) {
			KernelPatcher::RouteRequest request("__ZN31AppleIntelFramebufferController17ComputeHdmiP0P1P2EjP21AppleIntelDisplayPathPNS_10CRTCParamsE", wrapComputeHdmiP0P1P2);
			if (!patcher.routeMultiple(index, &request, 1, address, size))
				SYSLOG("igfx", "failed to route ComputeHdmiP0P1P2");
		}

		if (supportLSPCON) {
			auto roa = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController14ReadI2COverAUXEP21AppleIntelFramebufferP21AppleIntelDisplayPathjtPhbh", address, size);
			auto woa = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController15WriteI2COverAUXEP21AppleIntelFramebufferP21AppleIntelDisplayPathjtPhb", address, size);
			auto gdi = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController11GetDPCDInfoEP21AppleIntelFramebufferP21AppleIntelDisplayPath", address, size);
			if (roa && woa && gdi) {
				patcher.eraseCoverageInstPrefix(roa);
				patcher.eraseCoverageInstPrefix(woa);
				patcher.eraseCoverageInstPrefix(gdi);
				orgReadI2COverAUX = reinterpret_cast<decltype(orgReadI2COverAUX)>(patcher.routeFunction(roa, reinterpret_cast<mach_vm_address_t>(wrapReadI2COverAUX), true));
				orgWriteI2COverAUX = reinterpret_cast<decltype(orgWriteI2COverAUX)>(patcher.routeFunction(woa, reinterpret_cast<mach_vm_address_t>(wrapWriteI2COverAUX), true));
				orgGetDPCDInfo = reinterpret_cast<decltype(orgGetDPCDInfo)>(patcher.routeFunction(gdi, reinterpret_cast<mach_vm_address_t>(wrapGetDPCDInfo), true));
				if (orgReadI2COverAUX && orgWriteI2COverAUX && orgGetDPCDInfo) {
					DBGLOG("igfx", "SC: ReadI2COverAUX(), etc. have been routed successfully");
				} else {
					patcher.clearError();
					SYSLOG("igfx", "SC: ReadI2COverAUX(), etc. cannot be routed");
				}
			} else {
				SYSLOG("igfx", "SC: Failed to find ReadI2COverAUX(), etc");
				patcher.clearError();
			}
		}

		if (blackScreenPatch) {
			bool foundSymbol = false;

			// Currently it is 10.14.1 and Kaby+...
			if (getKernelVersion() >= KernelVersion::Mojave && cpuGeneration >= CPUInfo::CpuGeneration::KabyLake) {
				KernelPatcher::RouteRequest request("__ZN31AppleIntelFramebufferController16ComputeLaneCountEPK29IODetailedTimingInformationV2jPj", wrapComputeLaneCountNouveau, orgComputeLaneCount);
				foundSymbol = patcher.routeMultiple(index, &request, 1, address, size);
			}

			if (!foundSymbol) {
				KernelPatcher::RouteRequest request("__ZN31AppleIntelFramebufferController16ComputeLaneCountEPK29IODetailedTimingInformationV2jjPj", wrapComputeLaneCount, orgComputeLaneCount);
				patcher.routeMultiple(index, &request, 1, address, size);
			}
		}

		if (applyFramebufferPatch || dumpFramebufferToDisk || dumpPlatformTable || hdmiAutopatch) {
			framebufferStart = reinterpret_cast<uint8_t *>(address);
			framebufferSize = size;
			
			if (cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge) {
				if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge) {
					gPlatformListIsSNB = true;
					gPlatformInformationList = patcher.solveSymbol<void *>(index, "_PlatformInformationList", address, size);
				} else {
					gPlatformListIsSNB = false;
					gPlatformInformationList = patcher.solveSymbol<void *>(index, "_gPlatformInformationList", address, size);
				}

				DBGLOG("igfx", "platform is snb %d and list " PRIKADDR, gPlatformListIsSNB, CASTKADDR(gPlatformInformationList));
			}

			if ((gPlatformInformationList && cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge) || cpuGeneration == CPUInfo::CpuGeneration::Westmere) {
				auto fbGetOSInformation = "__ZN31AppleIntelFramebufferController16getOSInformationEv";
				if (cpuGeneration == CPUInfo::CpuGeneration::Westmere)
					fbGetOSInformation = "__ZN22AppleIntelHDGraphicsFB16getOSInformationEv";
				else if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge)
					fbGetOSInformation = "__ZN23AppleIntelSNBGraphicsFB16getOSInformationEv";
				else if (cpuGeneration == CPUInfo::CpuGeneration::IvyBridge)
					fbGetOSInformation = "__ZN25AppleIntelCapriController16getOSInformationEv";
				else if (cpuGeneration == CPUInfo::CpuGeneration::Haswell)
					fbGetOSInformation = "__ZN24AppleIntelAzulController16getOSInformationEv";
				else if (cpuGeneration == CPUInfo::CpuGeneration::Broadwell)
					fbGetOSInformation = "__ZN22AppleIntelFBController16getOSInformationEv";

				KernelPatcher::RouteRequest request(fbGetOSInformation, wrapGetOSInformation, orgGetOSInformation);
				patcher.routeMultiple(index, &request, 1, address, size);
			} else if (cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge) {
				SYSLOG("igfx", "failed to obtain gPlatformInformationList pointer with code %d", patcher.getError());
				patcher.clearError();
			}
			
			if (applyFramebufferPatch && cpuGeneration == CPUInfo::CpuGeneration::Westmere)
				applyWestmerePatches(patcher);
		}
		return true;
	}

	return false;
}

IOReturn IGFX::wrapPavpSessionCallback(void *intelAccelerator, int32_t sessionCommand, uint32_t sessionAppId, uint32_t *a4, bool flag) {
	//DBGLOG("igfx, "pavpCallback: cmd = %d, flag = %d, app = %u, a4 = %s", sessionCommand, flag, sessionAppId, a4 == nullptr ? "null" : "not null");

	if (sessionCommand == 4) {
		DBGLOG("igfx", "pavpSessionCallback: enforcing error on cmd 4 (send to ring?)!");
		return kIOReturnTimeout; // or kIOReturnSuccess
	}

	return FunctionCast(wrapPavpSessionCallback, callbackIGFX->orgPavpSessionCallback)(intelAccelerator, sessionCommand, sessionAppId, a4, flag);
}

bool IGFX::globalPageTableRead(void *hardwareGlobalPageTable, uint64_t address, uint64_t &physAddress, uint64_t &flags) {
	uint64_t pageNumber = address >> PAGE_SHIFT;
	uint64_t pageEntry = getMember<uint64_t *>(hardwareGlobalPageTable, 0x28)[pageNumber];
	// PTE: Page Table Entry for 4KB Page, page 82:
	// https://01.org/sites/default/files/documentation/intel-gfx-prm-osrc-kbl-vol05-memory_views.pdf.
	physAddress = pageEntry & 0x7FFFFFF000ULL; // HAW-1:12, where HAW is 39.
	flags = pageEntry & PAGE_MASK; // 11:0
	// Relevant flag bits are as follows:
	// 2 Ignored          Ignored (h/w does not care about values behind ignored registers)
	// 1 R/W: Read/Write  Write permission rights. If 0, write permission not granted for requests with user-level privilege
	//                    (and requests with supervisor-level privilege, if WPE=1 in the relevant extended-context-entry) to
	//                    the memory region controlled by this entry. See a later section for access rights.
	//                    GPU does not support Supervisor mode contexts.
	// 0 P: Present       This bit must be “1” to point to a valid Page.
	//
	// In 10.14.4b1 the return condition changed from (flags & 7U) != 0 to (flags & 1U) != 0. The change makes good sense to me, but
	// it results in crashes on 10.14.4b1 on many ASUS Z170 and Z270 boards with connector-less IGPU framebuffer.
	// The reason for that is that __ZNK31IGHardwarePerProcessPageTable6422readDescriptorForRangeERK14IGAddressRangePPN10IGPagePool14PageDescriptorE
	// unconditionally returns true but actually returns NULL PageDescriptor, which subsequently results in OSAddAtomic64(&this->counter)
	// __ZN31IGHardwarePerProcessPageTable6421mapDescriptorForRangeERK14IGAddressRangePN10IGPagePool14PageDescriptorE writing to invalid address.
	//
	// Fault CR2: 0x0000000000000028, Error code: 0x0000000000000002, Fault CPU: 0x1, PL: 0, VF: 0
	// 0xffffff821885b8f0 : 0xffffff801d3c7dc4 mach_kernel : _OSAddAtomic64 + 0x4
	// 0xffffff821885b9e0 : 0xffffff7f9f3f1845 com.apple.driver.AppleIntelKBLGraphics :
	//                      __ZN31IGHardwarePerProcessPageTable6421mapDescriptorForRangeERK14IGAddressRangePN10IGPagePool14PageDescriptorE + 0x6b
	// 0xffffff821885ba20 : 0xffffff7f9f40a3c3 com.apple.driver.AppleIntelKBLGraphics :
	//                      __ZN29IGHardwarePerProcessPageTable25synchronizePageDescriptorEPKS_RK14IGAddressRangeb + 0x51
	// 0xffffff821885ba50 : 0xffffff7f9f40a36b com.apple.driver.AppleIntelKBLGraphics :
	//                      __ZN29IGHardwarePerProcessPageTable15synchronizeWithIS_EEvPKT_RK14IGAddressRangeb + 0x37
	// 0xffffff821885ba70 : 0xffffff7f9f3b1716 com.apple.driver.AppleIntelKBLGraphics : __ZN15IGMemoryManager19newPageTableForTaskEP11IGAccelTask + 0x98
	// 0xffffff821885baa0 : 0xffffff7f9f40a716 com.apple.driver.AppleIntelKBLGraphics : __ZN11IGAccelTask15initWithOptionsEP16IntelAccelerator + 0x108
	// 0xffffff821885bad0 : 0xffffff7f9f40a5f7 com.apple.driver.AppleIntelKBLGraphics : __ZN11IGAccelTask11withOptionsEP16IntelAccelerator + 0x31
	// 0xffffff821885baf0 : 0xffffff7f9f3bfaf0 com.apple.driver.AppleIntelKBLGraphics : __ZN16IntelAccelerator17createUserGPUTaskEv + 0x30
	// 0xffffff821885bb10 : 0xffffff7f9f2f7520 com.apple.iokit.IOAcceleratorFamily2 : __ZN14IOAccelShared24initEP22IOGraphicsAccelerator2P4task + 0x2e
	// 0xffffff821885bb50 : 0xffffff7f9f30c157 com.apple.iokit.IOAcceleratorFamily2 : __ZN22IOGraphicsAccelerator212createSharedEP4task + 0x33
	// 0xffffff821885bb80 : 0xffffff7f9f2faa95 com.apple.iokit.IOAcceleratorFamily2 : __ZN24IOAccelSharedUserClient211sharedStartEv + 0x2b
	// 0xffffff821885bba0 : 0xffffff7f9f401ca2 com.apple.driver.AppleIntelKBLGraphics : __ZN23IGAccelSharedUserClient11sharedStartEv + 0x16
	// 0xffffff821885bbc0 : 0xffffff7f9f2f8aaa com.apple.iokit.IOAcceleratorFamily2 : __ZN24IOAccelSharedUserClient25startEP9IOService + 0x9c
	// 0xffffff821885bbf0 : 0xffffff7f9f30ba3c com.apple.iokit.IOAcceleratorFamily2 : __ZN22IOGraphicsAccelerator213newUserClientEP4taskPvjPP12IOUserClient + 0x43e
	// 0xffffff821885bc80 : 0xffffff801d42a040 mach_kernel : __ZN9IOService13newUserClientEP4taskPvjP12OSDictionaryPP12IOUserClient + 0x30
	// 0xffffff821885bcd0 : 0xffffff801d48ef9a mach_kernel : _is_io_service_open_extended + 0x10a
	// 0xffffff821885bd30 : 0xffffff801ce96b52 mach_kernel : _iokit_server_routine + 0x58d2
	// 0xffffff821885bd80 : 0xffffff801cdb506c mach_kernel : _ipc_kobject_server + 0x12c
	// 0xffffff821885bdd0 : 0xffffff801cd8fa91 mach_kernel : _ipc_kmsg_send + 0xd1
	// 0xffffff821885be50 : 0xffffff801cda42fe mach_kernel : _mach_msg_overwrite_trap + 0x3ce
	// 0xffffff821885bef0 : 0xffffff801cec3e87 mach_kernel : _mach_call_munger64 + 0x257
	// 0xffffff821885bfa0 : 0xffffff801cd5d2c6 mach_kernel : _hndl_mach_scall64 + 0x16
	//
	// Even so the change makes good sense to me, and most likely the real bug is elsewhere. The change workarounds the issue by also checking
	// for the W (writeable) bit in addition to P (present). Presumably this works because some code misuses ::read method to iterate
	// over page table instead of obtaining valid mapped physical address.
	return (flags & 3U) != 0;
}

bool IGFX::wrapComputeLaneCount(void *that, void *timing, uint32_t bpp, int32_t availableLanes, int32_t *laneCount) {
	DBGLOG("igfx", "computeLaneCount: bpp = %u, available = %d", bpp, availableLanes);

	// It seems that AGDP fails to properly detect external boot monitors. As a result computeLaneCount
	// is mistakengly called for any boot monitor (e.g. HDMI/DVI), while it is only meant to be used for
	// DP (eDP) displays. More details could be found at:
	// https://github.com/vit9696/Lilu/issues/27#issuecomment-372103559
	// Since the only problematic function is AppleIntelFramebuffer::validateDetailedTiming, there are
	// multiple ways to workaround it.
	// 1. In 10.13.4 Apple added an additional extended timing validation call, which happened to be
	// guardded by a HDMI 2.0 enable boot-arg, which resulted in one bug fixing the other, and 10.13.5
	// broke it again.
	// 2. Another good way is to intercept AppleIntelFramebufferController::RegisterAGDCCallback and
	// make sure AppleGraphicsDevicePolicy::_VendorEventHandler returns mode 2 (not 0) for event 10.
	// 3. Disabling AGDC by nopping AppleIntelFramebufferController::RegisterAGDCCallback is also fine.
	// Simply returning true from computeLaneCount and letting 0 to be compared against zero so far was
	// least destructive and most reliable. Let's stick with it until we could solve more problems.
	bool r = FunctionCast(wrapComputeLaneCount, callbackIGFX->orgComputeLaneCount)(that, timing, bpp, availableLanes, laneCount);
	if (!r && *laneCount == 0) {
		DBGLOG("igfx", "reporting worked lane count (legacy)");
		r = true;
	}

	return r;
}

bool IGFX::wrapComputeLaneCountNouveau(void *that, void *timing, int32_t availableLanes, int32_t *laneCount) {
	bool r = FunctionCast(wrapComputeLaneCountNouveau, callbackIGFX->orgComputeLaneCount)(that, timing, availableLanes, laneCount);
	if (!r && *laneCount == 0) {
		DBGLOG("igfx", "reporting worked lane count (nouveau)");
		r = true;
	}

	return r;
}

OSObject *IGFX::wrapCopyExistingServices(OSDictionary *matching, IOOptionBits inState, IOOptionBits options) {
	if (matching && inState == kIOServiceMatchedState && options == 0) {
		auto name = OSDynamicCast(OSString, matching->getObject(gIONameMatchKey));
		if (name) {
			DBGLOG("igfx", "found matching request by name %s", name->getCStringNoCopy());
			if (name->isEqualTo("Gen6Accelerator")) {
				DBGLOG("igfx", "found and fixed Gen6Accelerator request");
				auto accel = OSString::withCString("IntelAccelerator");
				if (accel) {
					matching->setObject(gIONameMatchKey, accel);
					accel->release();
				}
			}
		}
	}

	return FunctionCast(wrapCopyExistingServices, callbackIGFX->orgCopyExistingServices)(matching, inState, options);
}

bool IGFX::wrapAcceleratorStart(IOService *that, IOService *provider) {
	if (callbackIGFX->disableAccel)
		return false;
	
	OSDictionary* developmentDictCpy {};

	if (callbackIGFX->fwLoadMode != FW_APPLE || callbackIGFX->ForceWakeWorkaround.enabled) {
		auto developmentDict = OSDynamicCast(OSDictionary, that->getProperty("Development"));
		if (developmentDict) {
			auto c = developmentDict->copyCollection();
			if (c)
				developmentDictCpy = OSDynamicCast(OSDictionary, c);
			if (c && !developmentDictCpy)
				c->release();
		}
	}

	// By default Apple drivers load Apple-specific firmware, which is incompatible.
	// On KBL they do it unconditionally, which causes infinite loop.
	// On 10.13 there is an option to ignore/load a generic firmware, which we set here.
	// On 10.12 it is not necessary.
	// On 10.15 an option is differently named but still there.
	// There are some laptops that support Apple firmware, for them we want it to be loaded explicitly.
	// REF: https://github.com/acidanthera/bugtracker/issues/748
	if (callbackIGFX->fwLoadMode != FW_APPLE && developmentDictCpy) {
		// 1 - Automatic scheduler (Apple -> fallback to disabled)
		// 2 - Force disable via plist (removed as of 10.15)
		// 3 - Apple Scheduler
		// 4 - Reference Scheduler
		// 5 - Host Preemptive (as of 10.15)
		uint32_t scheduler;
		if (callbackIGFX->fwLoadMode == FW_GENERIC)
			scheduler = 4;
		else if (getKernelVersion() >= KernelVersion::Catalina)
			scheduler = 5;
		else
			scheduler = 2;
		auto num = OSNumber::withNumber(scheduler, 32);
		if (num) {
			developmentDictCpy->setObject("GraphicsSchedulerSelect", num);
			num->release();
		}
	}
	
	// 0: Framebuffer's SafeForceWake
	// 1: IntelAccelerator::SafeForceWakeMultithreaded (or ForceWakeWorkaround when enabled)
	// The default is 1. Forcing 0 will result in hangs (due to misbalanced number of calls?)
	if (callbackIGFX->ForceWakeWorkaround.enabled && developmentDictCpy) {
		auto num = OSNumber::withNumber(1ull, 32);
		if (num) {
			developmentDictCpy->setObject("MultiForceWakeSelect", num);
			num->release();
		}
	}
	
	if (developmentDictCpy) {
		that->setProperty("Development", developmentDictCpy);
		developmentDictCpy->release();
	}

	OSObject *metalPluginName = that->getProperty("MetalPluginName");
	if (metalPluginName) {
		metalPluginName->retain();
	}

	if (callbackIGFX->forceOpenGL) {
		DBGLOG("igfx", "disabling metal support");
		that->removeProperty("MetalPluginClassName");
		that->removeProperty("MetalPluginName");
		that->removeProperty("MetalStatisticsName");
	}

	if (callbackIGFX->moderniseAccelerator)
		that->setName("IntelAccelerator");

	bool ret = FunctionCast(wrapAcceleratorStart, callbackIGFX->orgAcceleratorStart)(that, provider);

	if (metalPluginName) {
		if (callbackIGFX->forceMetal) {
			DBGLOG("igfx", "enabling metal support");
			that->setProperty("MetalPluginName", metalPluginName);
		}
		metalPluginName->release();
	}

	return ret;
}

bool IGFX::wrapHwRegsNeedUpdate(void *controller, IOService *framebuffer, void *displayPath, void *crtParams, void *detailedInfo) {
	// The framebuffer controller can perform panel fitter, partial, or a
	// complete modeset (see AppleIntelFramebufferController::hwSetMode).
	// In a dual-monitor CFL DVI+HDMI setup, only HDMI output was working after
	// boot: it was observed that for HDMI framebuffer a complete modeset
	// eventually occured, but for DVI it never did until after sleep and wake
	// sequence.
	//
	// Function AppleIntelFramebufferController::hwRegsNeedUpdate checks
	// whether a complete modeset needs to be issued. It does so by comparing
	// sets of pipes and transcoder parameters. For some reason, the result was
	// never true in the above scenario, so a complete modeset never occured.
	// Consequently, AppleIntelFramebufferController::LightUpTMDS was never
	// called for that framebuffer.
	//
	// Patching hwRegsNeedUpdate to always return true seems to be a rather
	// safe solution to that. Note that the root cause of the problem is
	// somewhere deeper.

	// On older Skylake versions this function has no detailedInfo and does not use framebuffer argument.
	// As a result the compiler does not pass framebuffer to the target function. Since the fix is disabled
	// by default for Skylake, just force complete modeset on all framebuffers when actually requested.
	if (callbackIGFX->forceCompleteModeset.legacy)
		return true;

	// Either this framebuffer is in override list
	if (callbackIGFX->forceCompleteModeset.customised) {
		return callbackIGFX->forceCompleteModeset.inList(framebuffer)
		|| FunctionCast(callbackIGFX->wrapHwRegsNeedUpdate, callbackIGFX->orgHwRegsNeedUpdate)(
			controller, framebuffer, displayPath, crtParams, detailedInfo);
	}

	// Or it is not built-in, as indicated by AppleBacklightDisplay setting property "built-in" for
	// this framebuffer.
	// Note we need to check this at every invocation, as this property may reappear
	return !framebuffer->getProperty("built-in")
			|| FunctionCast(callbackIGFX->wrapHwRegsNeedUpdate, callbackIGFX->orgHwRegsNeedUpdate)(
			controller, framebuffer, displayPath, crtParams, detailedInfo);
}

IOReturn IGFX::wrapFBClientDoAttribute(void *fbclient, uint32_t attribute, unsigned long *unk1, unsigned long unk2, unsigned long *unk3, unsigned long *unk4, void *externalMethodArguments) {
	if (attribute == kAGDCRegisterCallback) {
		DBGLOG("igfx", "ignoring AGDC registration in FBClientControl::doAttribute");
		return kIOReturnUnsupported;
	}

	return FunctionCast(wrapFBClientDoAttribute, callbackIGFX->orgFBClientDoAttribute)(fbclient, attribute, unk1, unk2, unk3, unk4, externalMethodArguments);
}

/**
 * Apparently, platforms with (ig-platform-id & 0xf != 0) have only Type C connectivity.
 * Framebuffer kext uses this fact to sanitise connector type, forcing it to DP.
 * This breaks many systems, so we undo this check.
 * Affected drivers: KBL and newer?
 */
uint64_t IGFX::wrapIsTypeCOnlySystem(void*) {
	DBGLOG("igfx", "Forcing IsTypeCOnlySystem 0");
	return 0;
}

uint32_t IGFX::wrapGetDisplayStatus(IOService *framebuffer, void *displayPath) {
	// 0 - offline, 1 - online, 2 - empty dongle.
	uint32_t ret = FunctionCast(wrapGetDisplayStatus, callbackIGFX->orgGetDisplayStatus)(framebuffer, displayPath);
	if (ret != 1) {
		if (callbackIGFX->forceOnlineDisplay.customised)
			ret = callbackIGFX->forceOnlineDisplay.inList(framebuffer) ? 1 : ret;
		else
			ret = 1;
	}

	DBGLOG("igfx", "getDisplayStatus forces %u", ret);
	return ret;
}

IOReturn IGFX::wrapReadAUX(void *that, IORegistryEntry *framebuffer, uint32_t address, uint16_t length, void *buffer, void *displayPath) {

	//
	// Abstract:
	//
	// Several fields in an `AppleIntelFramebuffer` instance are left zeroed because of
	// an invalid value of maximum link rate reported by DPCD of the builtin display.
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

	// Call the original ReadAUX() function to read from DPCD
	IOReturn retVal = callbackIGFX->orgReadAUX(that, framebuffer, address, length, buffer, displayPath);

	// Guard: Check the DPCD register address
	// The first 16 fields of the receiver capabilities reside at 0x0 (DPCD Register Address)
	if (address != DPCD_DEFAULT_ADDRESS && address != DPCD_EXTENDED_ADDRESS)
		return retVal;

	// The driver tries to read the first 16 bytes from DPCD (0x0000) or extended DPCD (0x2200)
	// Get the current framebuffer index (An UInt32 field at 0x1dc in a framebuffer instance)
	// We read the value of "IOFBDependentIndex" instead of accessing that field directly
	uint32_t index;
	// Guard: Should be able to retrieve the index from the registry
	if (!AppleIntelFramebufferExplorer::getIndex(framebuffer, index)) {
		SYSLOG("igfx", "MLR: wrapReadAUX: Failed to read the current framebuffer index.");
		return retVal;
	}

	// Guard: Check the framebuffer index
	// By default, FB 0 refers the builtin display
	if (index != 0)
		// The driver is reading DPCD for an external display
		return retVal;

	// The driver tries to read the receiver capabilities for the builtin display
	auto caps = reinterpret_cast<DPCDCap16*>(buffer);

	// Set the custom maximum link rate value
	caps->maxLinkRate = callbackIGFX->maxLinkRate;

	DBGLOG("igfx", "MLR: wrapReadAUX: Maximum link rate 0x%02x has been set in the DPCD buffer.", caps->maxLinkRate);

	return retVal;
}

void IGFX::populateP0P1P2(struct ProbeContext *context) {
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

int IGFX::wrapComputeHdmiP0P1P2(void *that, uint32_t pixelClock, void *displayPath, void *parameters) {
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

	DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() DInfo: Called with pixel clock = %d Hz.", pixelClock);

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
			DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() DInfo: Dev = %6llu; Central = %10llu Hz; DCO Freq = %12llu Hz; Divider = %2d.\n", deviation, central, frequency, divider);

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
			DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() DInfo: FOUND: Min Dev = %8llu; Central = %10llu Hz; Freq = %12llu Hz; Divider = %d\n", deviation, central, frequency, divider);

			// Guard: Check whether the new minmimum deviation has been reduced to 0
			if (deviation != 0)
				continue;

			// Guard: An even divider is preferred
			if (divider % 2 == 0) {
				DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() DInfo: Found an even divider [%d] with deviation 0.\n", divider);
				break;
			}
		}
	}

	// Guard: A valid divider has been found
	if (context.divider == 0) {
		SYSLOG("igfx", "SC: ComputeHdmiP0P1P2() Error: Cannot find a valid divider for the given pixel clock %d Hz.\n", pixelClock);
		return 0;
	}

	// Calculate the p,q,k dividers
	populateP0P1P2(&context);
	DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() DInfo: Divider = %d --> P0 = %d; P1 = %d; P2 = %d.\n", context.divider, context.pdiv, context.qdiv, context.kdiv);

	// Calculate the CRTC parameters
	uint32_t multiplier = (uint32_t) (context.frequency / 24000000);
	uint32_t fraction = (uint32_t) (context.frequency - multiplier * 24000000);
	uint32_t cf15625 = (uint32_t) (context.central / 15625);
	DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() DInfo: Multiplier = %d; Fraction = %d; CF15625 = %d.\n", multiplier, fraction, cf15625);
	// Guard: The given CRTC parameters should never be NULL
	if (parameters == nullptr) {
		DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() Error: The given CRTC parameters should not be NULL.");
		return 0;
	}
	auto params = reinterpret_cast<CRTCParams*>(parameters);
	params->pdiv = context.pdiv;
	params->qdiv = context.qdiv;
	params->kdiv = context.kdiv;
	params->multiplier = multiplier;
	params->fraction = fraction;
	params->cf15625 = cf15625;
	DBGLOG("igfx", "SC: ComputeHdmiP0P1P2() DInfo: CTRC parameters have been populated successfully.");
	return 0;
}

IOReturn IGFX::LSPCON::probe() {
	// Read the adapter info
	uint8_t buffer[128] {};
	IOReturn retVal = callbackIGFX->advReadI2COverAUX(controller, framebuffer, displayPath, DP_DUAL_MODE_ADAPTER_I2C_ADDR, 0x00, 128, buffer, 0);
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
	DBGLOG("igfx", "SC: LSPCON::probe() DInfo: [FB%d] Found the LSPCON adapter: %s %s.", index, getVendorString(parseVendor(info)), device);

	// Parse the current adapter mode
	Mode mode = parseMode(info->lspconCurrentMode);
	DBGLOG("igfx", "SC: LSPCON::probe() DInfo: [FB%d] The current adapter mode is %s.", index, getModeString(mode));
	if (mode == Mode::Invalid)
		SYSLOG("igfx", "SC: LSPCON::probe() Error: [FB%d] Cannot detect the current adapter mode. Assuming Level Shifter mode.", index);
	return kIOReturnSuccess;
}

IOReturn IGFX::LSPCON::getMode(Mode *mode) {
	IOReturn retVal = kIOReturnAborted;

	// Guard: The given `mode` pointer cannot be NULL
	if (mode != nullptr) {
		// Try to read the current mode from the adapter at most 5 times
		for (int attempt = 0; attempt < 5; attempt++) {
			// Read from the adapter @ 0x40; offset = 0x41
			uint8_t hwModeValue;
			retVal = callbackIGFX->advReadI2COverAUX(controller, framebuffer, displayPath, DP_DUAL_MODE_ADAPTER_I2C_ADDR, DP_DUAL_MODE_LSPCON_CURRENT_MODE, 1, &hwModeValue, 0);

			// Guard: Can read the current adapter mode successfully
			if (retVal == kIOReturnSuccess) {
				DBGLOG("igfx", "SC: LSPCON::getMode() DInfo: [FB%d] The current mode value is 0x%02x.", index, hwModeValue);
				*mode = parseMode(hwModeValue);
				break;
			}

			// Sleep 1 ms just in case the adapter
			// is busy processing other I2C requests
			IOSleep(1);
		}
	}

	return retVal;
}

IOReturn IGFX::LSPCON::setMode(Mode newMode) {
	// Guard: The given new mode must be valid
	if (newMode == Mode::Invalid)
		return kIOReturnAborted;

	// Guard: Write the new mode
	uint8_t hwModeValue = getModeValue(newMode);
	IOReturn retVal = callbackIGFX->advWriteI2COverAUX(controller, framebuffer, displayPath, DP_DUAL_MODE_ADAPTER_I2C_ADDR, DP_DUAL_MODE_LSPCON_CHANGE_MODE, 1, &hwModeValue, 0);
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: LSPCON::setMode() Error: [FB%d] Failed to set the new adapter mode. RV = 0x%llx.", index, retVal);
		return retVal;
	}

	// Read the register again and verify the mode
	uint32_t timeout = 200;
	Mode mode = Mode::Invalid;
	while (timeout != 0) {
		retVal = getMode(&mode);
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

IOReturn IGFX::LSPCON::setModeIfNecessary(Mode newMode) {
	if (isRunningInMode(newMode)) {
		DBGLOG("igfx", "SC: LSPCON::setModeIfNecessary() DInfo: [FB%d] The adapter is already running in %s mode. No need to update.", index, getModeString(newMode));
		return kIOReturnSuccess;
	}

	return setMode(newMode);
}

IOReturn IGFX::LSPCON::wakeUpNativeAUX() {
	uint8_t byte;
	IOReturn retVal = wrapReadAUX(controller, framebuffer, 0x00000, 1, &byte, displayPath);
	if (retVal != kIOReturnSuccess)
		SYSLOG("igfx", "SC: LSPCON::wakeUpNativeAUX() Error: [FB%d] Failed to wake up the native AUX channel. RV = 0x%llx.", index, retVal);
	else
		DBGLOG("igfx", "SC: LSPCON::wakeUpNativeAUX() DInfo: [FB%d] The native AUX channel is up. DPCD Rev = 0x%02x.", index, byte);
	return retVal;
}

IOReturn IGFX::wrapReadI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint16_t length, uint8_t *buffer, bool intermediate, uint8_t flags) {
	if (callbackIGFX->verboseI2C) {
		uint32_t index = 0xFF;
		AppleIntelFramebufferExplorer::getIndex(framebuffer, index);
		DBGLOG("igfx", "SC:  ReadI2COverAUX() called. FB%d: Addr = 0x%02x; Len = %02d; MOT = %d; Flags = %d.",
			   index, address, length, intermediate, flags);
		IOReturn retVal = callbackIGFX->orgReadI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate, flags);
		DBGLOG("igfx", "SC:  ReadI2COverAUX() returns 0x%x.", retVal);
		return retVal;
	} else {
		return callbackIGFX->orgReadI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate, flags);
	}
}

IOReturn IGFX::wrapWriteI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint16_t length, uint8_t *buffer, bool intermediate) {
	if (callbackIGFX->verboseI2C) {
		uint32_t index = 0xFF;
		AppleIntelFramebufferExplorer::getIndex(framebuffer, index);
		DBGLOG("igfx", "SC: WriteI2COverAUX() called. FB%d: Addr = 0x%02x; Len = %02d; MOT = %d; Flags = 0",
			   index, address, length, intermediate);
		IOReturn retVal = callbackIGFX->orgWriteI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate);
		DBGLOG("igfx", "SC: WriteI2COverAUX() returns 0x%x.", retVal);
		return retVal;
	} else {
		return callbackIGFX->orgWriteI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate);
	}
}

IOReturn IGFX::advSeekI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint8_t flags) {
	// No need to check the given `address` and `offset`
	// if they are invalid, the underlying RunAUXCommand() will return an error
	// First start the transaction by performing an empty write
	IOReturn retVal = wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, true);

	// Guard: Check the START transaction
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: AdvSeekI2COverAUX() Error: Failed to start the I2C transaction. Return value = 0x%x.\n", retVal);
		return retVal;
	}

	// Write a single byte to the given I2C slave
	// and set the Middle-of-Transaction bit to 1
	return wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 1, reinterpret_cast<uint8_t*>(&offset), true);
}

IOReturn IGFX::advReadI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint16_t length, uint8_t *buffer, uint8_t flags) {
	// Guard: Check the buffer length
	if (length == 0) {
		SYSLOG("igfx", "SC: AdvReadI2COverAUX() Error: Buffer length must be non-zero.");
		return kIOReturnInvalid;
	}

	// Guard: Check the buffer
	if (buffer == nullptr) {
		SYSLOG("igfx", "SC: AdvReadI2COverAUX() Error: Buffer cannot be NULL.");
		return kIOReturnInvalid;
	}

	// Guard: Start the transaction and set the access offset successfully
	IOReturn retVal = advSeekI2COverAUX(that, framebuffer, displayPath, address, offset, flags);
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: AdvReadI2COverAUX() Error: Failed to set the data offset.");
		return retVal;
	}

	// Process the read request
	// ReadI2COverAUX() can only process up to 16 bytes in one AUX transaction
	// because the burst data size is 20 bytes, in which the first 4 bytes are used for the AUX message header
	while (length != 0) {
		// Calculate the new length for this I2C-over-AUX transaction
		uint16_t newLength = length >= 16 ? 16 : length;

		// This is an intermediate transaction
		retVal = wrapReadI2COverAUX(that, framebuffer, displayPath, address, newLength, buffer, true, flags);

		// Guard: The intermediate transaction succeeded
		if (retVal != kIOReturnSuccess) {
			// Terminate the transaction
			wrapReadI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false, flags);
			return retVal;
		}

		// Update the buffer position and length
		length -= newLength;
		buffer += newLength;
	}

	// All intermediate transactions succeeded
	// Terminate the transaction
	return wrapReadI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false, flags);
}

IOReturn IGFX::advWriteI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint16_t length, uint8_t *buffer, uint8_t flags) {
	// Guard: Check the buffer length
	if (length == 0) {
		SYSLOG("igfx", "SC: AdvWriteI2COverAUX() Error: Buffer length must be non-zero.");
		return kIOReturnInvalid;
	}

	// Guard: Check the buffer
	if (buffer == nullptr) {
		SYSLOG("igfx", "SC: AdvWriteI2COverAUX() Error: Buffer cannot be NULL.");
		return kIOReturnInvalid;
	}

	// Guard: Start the transaction and set the access offset successfully
	IOReturn retVal = advSeekI2COverAUX(that, framebuffer, displayPath, address, offset, flags);
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: AdvWriteI2COverAUX() Error: Failed to set the data offset.");
		return retVal;
	}

	// Process the write request
	// WriteI2COverAUX() can only process up to 16 bytes in one AUX transaction
	// because the burst data size is 20 bytes, in which the first 4 bytes are used for the AUX message header
	while (length != 0) {
		// Calculate the new length for this I2C-over-AUX transaction
		uint16_t newLength = length >= 16 ? 16 : length;

		// This is an intermediate transaction
		retVal = wrapWriteI2COverAUX(that, framebuffer, displayPath, address, newLength, buffer, true);

		// Guard: The intermediate transaction succeeded
		if (retVal != kIOReturnSuccess) {
			// Terminate the transaction
			wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false);
			return retVal;
		}

		// Update the buffer position and length
		length -= newLength;
		buffer += newLength;
	}

	// All intermediate transactions succeeded
	// Terminate the transaction
	return wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false);
}

void IGFX::framebufferSetupLSPCON(void *that, IORegistryEntry *framebuffer, void *displayPath) {
	// Retrieve the framebuffer index
	uint32_t index;
	if (!AppleIntelFramebufferExplorer::getIndex(framebuffer, index)) {
		SYSLOG("igfx", "SC: fbSetupLSPCON() Error: Failed to retrieve the framebuffer index.");
		return;
	}
	DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] called with controller at 0x%llx and framebuffer at 0x%llx.", index, that, framebuffer);

	// Retrieve the user preference
	LSPCON *lspcon = nullptr;
	auto pmode = framebufferLSPCONGetPreferredMode(index);
#ifdef DEBUG
	framebuffer->setProperty("fw-framebuffer-has-lspcon", framebufferHasLSPCON(index));
	framebuffer->setProperty("fw-framebuffer-preferred-lspcon-mode", LSPCON::getModeValue(pmode), 8);
#endif

	// Guard: Check whether this framebuffer connector has an onboard LSPCON chip
	if (!framebufferHasLSPCON(index)) {
		// No LSPCON chip associated with this connector
		DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] No LSPCON chip associated with this framebuffer.", index);
		return;
	}

	// Guard: Check whether the LSPCON driver has already been initialized for this framebuffer
	if (framebufferHasLSPCONInitialized(index)) {
		// Already initialized
		lspcon = framebufferGetLSPCON(index);
		DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] LSPCON driver (at 0x%llx) has already been initialized for this framebuffer.", index, lspcon);
		// Confirm that the adapter is running in preferred mode
		if (lspcon->setModeIfNecessary(pmode) != kIOReturnSuccess) {
			SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] The adapter is not running in preferred mode. Failed to update the mode.", index);
		}
		// Wake up the native AUX channel if PCON mode is preferred
		if (pmode == LSPCON::Mode::ProtocolConverter && lspcon->wakeUpNativeAUX() != kIOReturnSuccess) {
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
	framebufferSetLSPCON(index, lspcon);
	DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] LSPCON driver has been initialized successfully.", index);

	// Guard: Set the preferred adapter mode if necessary
	if (lspcon->setModeIfNecessary(pmode) != kIOReturnSuccess) {
		SYSLOG("igfx", "SC: fbSetupLSPCON() Error: [FB%d] The adapter is not running in preferred mode. Failed to set the %s mode.", index,  LSPCON::getModeString(pmode));
	}
	DBGLOG("igfx", "SC: fbSetupLSPCON() DInfo: [FB%d] The adapter is now running in preferred mode [%s].", index, LSPCON::getModeString(pmode));
}

IOReturn IGFX::wrapGetDPCDInfo(void *that, IORegistryEntry *framebuffer, void *displayPath) {
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
	framebufferSetupLSPCON(that, framebuffer, displayPath);
	DBGLOG("igfx", "SC: GetDPCDInfo() DInfo: Finished configuring the LSPCON adapter.");

	// Call the original method
	DBGLOG("igfx", "SC: GetDPCDInfo() DInfo: Will call the original method.");
	IOReturn retVal = callbackIGFX->orgGetDPCDInfo(that, framebuffer, displayPath);
	DBGLOG("igfx", "SC: GetDPCDInfo() DInfo: Returns 0x%llx.", retVal);
	return retVal;
}

void IGFX::sanitizeCDClockFrequency(void *that)
{
	// Read the hardware reference frequency from the DSSM register
	// Bits 29-31 store the reference frequency value
	auto referenceFrequency = callbackIGFX->orgIclReadRegister32(that, ICL_REG_DSSM) >> 29;
	
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
	callbackIGFX->orgDisableCDClock(that);
	DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock PLL has been disabled.");
	
	// Set the new PLL frequency and reenable the Core Display Clock PLL
	callbackIGFX->orgSetCDClockFrequency(that, newPLLFrequency);
	DBGLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock has been reprogrammed and PLL has been re-enabled.");
	
	// "Verify" that the new frequency is effective
	auto cdclk = callbackIGFX->orgIclReadRegister32(that, ICL_REG_CDCLK_CTL) & 0x7FF;
	SYSLOG("igfx", "CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock frequency is %s MHz now.",
		   coreDisplayClockDecimalFrequency2String(cdclk));
}

uint32_t IGFX::wrapProbeCDClockFrequency(void *that) {
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
	auto cdclk = callbackIGFX->orgIclReadRegister32(that, ICL_REG_CDCLK_CTL) & 0x7FF;
	SYSLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The current core display clock frequency is %s MHz.",
		   coreDisplayClockDecimalFrequency2String(cdclk));
	
	// Guard: Check whether the current frequency is supported by the graphics driver
	if (cdclk < ICL_CDCLK_DEC_FREQ_THRESHOLD) {
		DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The currrent core display clock frequency is not supported.");
		callbackIGFX->sanitizeCDClockFrequency(that);
		DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The core display clock has been switched to a supported frequency.");
	}
	
	// Invoke the original method to ensure everything works as expected
	DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: Will invoke the original function.");
	auto retVal = callbackIGFX->orgProbeCDClockFrequency(that);
	DBGLOG("igfx", "CDC: ProbeCDClockFrequency() DInfo: The original function returns 0x%llx.", retVal);
	return retVal;
}

void IGFX::wrapCflWriteRegister32(void *that, uint32_t reg, uint32_t value) {
	if (reg == BXT_BLC_PWM_FREQ1) {
		if (value && value != callbackIGFX->driverBacklightFrequency) {
			DBGLOG("igfx", "wrapCflWriteRegister32: driver requested BXT_BLC_PWM_FREQ1 = 0x%x", value);
			callbackIGFX->driverBacklightFrequency = value;
		}

		if (callbackIGFX->targetBacklightFrequency == 0) {
			// Save the hardware PWM frequency as initially set up by the system firmware.
			// We'll need this to restore later after system sleep.
			callbackIGFX->targetBacklightFrequency = callbackIGFX->orgCflReadRegister32(that, BXT_BLC_PWM_FREQ1);
			DBGLOG("igfx", "wrapCflWriteRegister32: system initialized BXT_BLC_PWM_FREQ1 = 0x%x", callbackIGFX->targetBacklightFrequency);

			if (callbackIGFX->targetBacklightFrequency == 0) {
				// This should not happen with correctly written bootloader code, but in case it does, let's use a failsafe default value.
				callbackIGFX->targetBacklightFrequency = FallbackTargetBacklightFrequency;
				SYSLOG("igfx", "wrapCflWriteRegister32: system initialized BXT_BLC_PWM_FREQ1 is ZERO");
			}
		}

		if (value) {
			// Nonzero writes to this register need to use the original system value.
			// Yet the driver can safely write zero to this register as part of system sleep.
			value = callbackIGFX->targetBacklightFrequency;
		}
	} else if (reg == BXT_BLC_PWM_DUTY1) {
		if (callbackIGFX->driverBacklightFrequency && callbackIGFX->targetBacklightFrequency) {
			// Translate the PWM duty cycle between the driver scale value and the HW scale value
			uint32_t rescaledValue = static_cast<uint32_t>((static_cast<uint64_t>(value) * static_cast<uint64_t>(callbackIGFX->targetBacklightFrequency)) /
				static_cast<uint64_t>(callbackIGFX->driverBacklightFrequency));
			DBGLOG("igfx", "wrapCflWriteRegister32: write PWM_DUTY1 0x%x/0x%x, rescaled to 0x%x/0x%x", value,
				   callbackIGFX->driverBacklightFrequency, rescaledValue, callbackIGFX->targetBacklightFrequency);
			value = rescaledValue;
		} else {
			// This should never happen, but in case it does we should log it at the very least.
			SYSLOG("igfx", "wrapCflWriteRegister32: write PWM_DUTY1 has zero frequency driver (%d) target (%d)",
				   callbackIGFX->driverBacklightFrequency, callbackIGFX->targetBacklightFrequency);
		}
	}

	callbackIGFX->orgCflWriteRegister32(that, reg, value);
}

void IGFX::wrapKblWriteRegister32(void *that, uint32_t reg, uint32_t value) {
	if (reg == BXT_BLC_PWM_FREQ1) { // aka BLC_PWM_PCH_CTL2
		if (callbackIGFX->targetBacklightFrequency == 0) {
			// Populate the hardware PWM frequency as initially set up by the system firmware.
			callbackIGFX->targetBacklightFrequency = callbackIGFX->orgKblReadRegister32(that, BXT_BLC_PWM_FREQ1);
			DBGLOG("igfx", "wrapKblWriteRegister32: system initialized BXT_BLC_PWM_FREQ1 = 0x%x", callbackIGFX->targetBacklightFrequency);
			DBGLOG("igfx", "wrapKblWriteRegister32: system initialized BXT_BLC_PWM_CTL1 = 0x%x", callbackIGFX->orgKblReadRegister32(that, BXT_BLC_PWM_CTL1));

			if (callbackIGFX->targetBacklightFrequency == 0) {
				// This should not happen with correctly written bootloader code, but in case it does, let's use a failsafe default value.
				callbackIGFX->targetBacklightFrequency = FallbackTargetBacklightFrequency;
				SYSLOG("igfx", "wrapKblWriteRegister32: system initialized BXT_BLC_PWM_FREQ1 is ZERO");
			}
		}

		// For the KBL driver, 0xc8254 (BLC_PWM_PCH_CTL2) controls the backlight intensity.
		// High 16 of this write are the denominator (frequency), low 16 are the numerator (duty cycle).
		// Translate this into a write to c8258 (BXT_BLC_PWM_DUTY1) for the CFL hardware, scaled by the system-provided value in c8254 (BXT_BLC_PWM_FREQ1).
		uint16_t frequency = (value & 0xffff0000U) >> 16U;
		uint16_t dutyCycle = value & 0xffffU;

		uint32_t rescaledValue = frequency == 0 ? 0 : static_cast<uint32_t>((static_cast<uint64_t>(dutyCycle) * static_cast<uint64_t>(callbackIGFX->targetBacklightFrequency)) /
										    static_cast<uint64_t>(frequency));
		DBGLOG("igfx", "wrapKblWriteRegister32: write PWM_DUTY1 0x%x/0x%x, rescaled to 0x%x/0x%x",
			   dutyCycle, frequency, rescaledValue, callbackIGFX->targetBacklightFrequency);

		// Reset the hardware PWM frequency. Write the original system value if the driver-requested value is nonzero. If the driver requests
		// zero, we allow that, since it's trying to turn off the backlight PWM for sleep.
		callbackIGFX->orgKblWriteRegister32(that, BXT_BLC_PWM_FREQ1, frequency ? callbackIGFX->targetBacklightFrequency : 0);

		// Finish by writing the duty cycle.
		reg = BXT_BLC_PWM_DUTY1;
		value = rescaledValue;
	} else if (reg == BXT_BLC_PWM_CTL1) {
		if (callbackIGFX->targetPwmControl == 0) {
			// Save the original hardware PWM control value
			callbackIGFX->targetPwmControl = callbackIGFX->orgKblReadRegister32(that, BXT_BLC_PWM_CTL1);
		}

		DBGLOG("igfx", "wrapKblWriteRegister32: write BXT_BLC_PWM_CTL1 0x%x, previous was 0x%x", value, callbackIGFX->orgKblReadRegister32(that, BXT_BLC_PWM_CTL1));

		if (value) {
			// Set the PWM frequency before turning it on to avoid the 3 minute blackout bug
			callbackIGFX->orgKblWriteRegister32(that, BXT_BLC_PWM_FREQ1, callbackIGFX->targetBacklightFrequency);

			// Use the original hardware PWM control value.
			value = callbackIGFX->targetPwmControl;
		}
	}

	callbackIGFX->orgKblWriteRegister32(that, reg, value);
}

void IGFX::wrapHswWriteRegister32(void *that, uint32_t reg, uint32_t value) {
	if (reg == BXT_BLC_PWM_FREQ1) {
		// BXT_BLC_PWM_FREQ1 controls the backlight intensity.
		// High 16 of this register are the denominator (frequency), low 16 are the numerator (duty cycle).

		if (callbackIGFX->targetBacklightFrequency == 0) {
			// Populate the hardware PWM frequency as initially set up by the system firmware.
			uint32_t org_value = callbackIGFX->orgHswReadRegister32(that, BXT_BLC_PWM_FREQ1);
			callbackIGFX->targetBacklightFrequency = (org_value & 0xffff0000U) >> 16U;
			DBGLOG("igfx", "wrapHswWriteRegister32: system initialized PWM frequency = 0x%x", callbackIGFX->targetBacklightFrequency);

			if (callbackIGFX->targetBacklightFrequency == 0) {
				// This should not happen with correctly written bootloader code, but in case it does, let's use a failsafe default value.
				callbackIGFX->targetBacklightFrequency = FallbackTargetBacklightFrequency;
				SYSLOG("igfx", "wrapHswWriteRegister32: system initialized PWM frequency is ZERO");
			}
		}

		uint32_t dutyCycle = value & 0xffffU;
		uint32_t frequency = (value & 0xffff0000U) >> 16U;

		uint32_t rescaledDutyCycle = frequency == 0 ? 0 : static_cast<uint32_t>((static_cast<uint64_t>(dutyCycle) * static_cast<uint64_t>(callbackIGFX->targetBacklightFrequency)) /
											static_cast<uint64_t>(frequency));

		if (frequency) {
			// Nonzero writes to frequency need to use the original system frequency.
			// Yet the driver can safely write zero to this register as part of system sleep.
			frequency = callbackIGFX->targetBacklightFrequency;
		}

		// Write the rescaled duty cycle and frequency
		// FIXME: what if rescaled duty cycle overflow unsigned 16 bit int?
		value = (frequency << 16U) | rescaledDutyCycle;
	}

	callbackIGFX->orgHswWriteRegister32(that, reg, value);
}

void IGFX::wrapIvyWriteRegister32(void *that, uint32_t reg, uint32_t value) {
	if (reg == BXT_BLC_PWM_FREQ1) {
		if (value && value != callbackIGFX->driverBacklightFrequency) {
			DBGLOG("igfx", "wrapIvyWriteRegister32: driver requested BXT_BLC_PWM_FREQ1 = 0x%x", value);
			callbackIGFX->driverBacklightFrequency = value;
		}

		if (callbackIGFX->targetBacklightFrequency == 0) {
			// Save the hardware PWM frequency as initially set up by the system firmware.
			// We'll need this to restore later after system sleep.
			callbackIGFX->targetBacklightFrequency = callbackIGFX->orgIvyReadRegister32(that, BXT_BLC_PWM_FREQ1);
			DBGLOG("igfx", "wrapIvyWriteRegister32: system initialized BXT_BLC_PWM_FREQ1 = 0x%x", callbackIGFX->targetBacklightFrequency);

			if (callbackIGFX->targetBacklightFrequency == 0) {
				// This should not happen with correctly written bootloader code, but in case it does, let's use a failsafe default value.
				callbackIGFX->targetBacklightFrequency = FallbackTargetBacklightFrequency;
				SYSLOG("igfx", "wrapIvyWriteRegister32: system initialized BXT_BLC_PWM_FREQ1 is ZERO");
			}
		}

		if (value) {
			// Nonzero writes to this register need to use the original system value.
			// Yet the driver can safely write zero to this register as part of system sleep.
			value = callbackIGFX->targetBacklightFrequency;
		}
	} else if (reg == BLC_PWM_CPU_CTL) {
		if (callbackIGFX->driverBacklightFrequency && callbackIGFX->targetBacklightFrequency) {
			// Translate the PWM duty cycle between the driver scale value and the HW scale value
			uint32_t rescaledValue = static_cast<uint32_t>((static_cast<uint64_t>(value) * static_cast<uint64_t>(callbackIGFX->targetBacklightFrequency)) /
				static_cast<uint64_t>(callbackIGFX->driverBacklightFrequency));
			DBGLOG("igfx", "wrapIvyWriteRegister32: write BLC_PWM_CPU_CTL 0x%x/0x%x, rescaled to 0x%x/0x%x", value,
				   callbackIGFX->driverBacklightFrequency, rescaledValue, callbackIGFX->targetBacklightFrequency);
			value = rescaledValue;
		} else {
			// This should never happen, but in case it does we should log it at the very least.
			SYSLOG("igfx", "wrapIvyWriteRegister32: write BLC_PWM_CPU_CTL has zero frequency driver (%d) target (%d)",
				   callbackIGFX->driverBacklightFrequency, callbackIGFX->targetBacklightFrequency);
		}
	}

	callbackIGFX->orgIvyWriteRegister32(that, reg, value);
}

bool IGFX::wrapGetOSInformation(IOService *that) {
	auto cpuGeneration = BaseDeviceInfo::get().cpuGeneration;
	
#ifdef DEBUG
	if (callbackIGFX->dumpFramebufferToDisk) {
		char name[64];
		snprintf(name, sizeof(name), "/var/log/AppleIntelFramebuffer_%d_%d.%d", cpuGeneration, getKernelVersion(), getKernelMinorVersion());
		FileIO::writeBufferToFile(name, callbackIGFX->framebufferStart, callbackIGFX->framebufferSize);
		SYSLOG("igfx", "dumping framebuffer information to %s", name);
		uint32_t delay = 20000;
		PE_parse_boot_argn("igfxdumpdelay", &delay, sizeof(delay));
		IOSleep(delay);
	}
#endif

#ifdef DEBUG
	if (callbackIGFX->dumpPlatformTable)
		callbackIGFX->writePlatformListData("platform-table-native");
#endif

	if (callbackIGFX->applyFramebufferPatch && cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge)
		callbackIGFX->applyFramebufferPatches();
	else if (callbackIGFX->applyFramebufferPatch && cpuGeneration == CPUInfo::CpuGeneration::Westmere)
		callbackIGFX->applyWestmereFeaturePatches(that);
	else if (callbackIGFX->hdmiAutopatch)
		callbackIGFX->applyHdmiAutopatch();

#ifdef DEBUG
	if (callbackIGFX->dumpPlatformTable)
		callbackIGFX->writePlatformListData("platform-table-patched");
#endif

	return FunctionCast(wrapGetOSInformation, callbackIGFX->orgGetOSInformation)(that);
}

bool IGFX::wrapLoadGuCBinary(void *that, bool flag) {
	bool r = false;
	DBGLOG("igfx", "attempting to load firmware for %d scheduler for cpu gen %d",
		   callbackIGFX->fwLoadMode, BaseDeviceInfo::get().cpuGeneration);

	if (callbackIGFX->firmwareSizePointer)
		callbackIGFX->performingFirmwareLoad = true;

	r = FunctionCast(wrapLoadGuCBinary, callbackIGFX->orgLoadGuCBinary)(that, flag);
	DBGLOG("igfx", "loadGuCBinary returned %d", r);

	callbackIGFX->performingFirmwareLoad = false;

	return r;
}

bool IGFX::wrapLoadFirmware(IOService *that) {
	DBGLOG("igfx", "load firmware setting sleep overrides %d", BaseDeviceInfo::get().cpuGeneration);

	// We have to patch the virtual table, because the original methods are very short.
	// See __ZN12IGScheduler415systemWillSleepEv and __ZN12IGScheduler413systemDidWakeEv
	// Note, that other methods are also not really implemented, so we may have to implement them ourselves sooner or later.
	(*reinterpret_cast<uintptr_t **>(that))[52] = reinterpret_cast<uintptr_t>(wrapSystemWillSleep);
	(*reinterpret_cast<uintptr_t **>(that))[53] = reinterpret_cast<uintptr_t>(wrapSystemDidWake);
	return FunctionCast(wrapLoadFirmware, callbackIGFX->orgLoadFirmware)(that);
}

void IGFX::wrapSystemWillSleep(IOService *that) {
	DBGLOG("igfx", "systemWillSleep GuC callback");
	// Perhaps we want to send a message to GuC firmware like Apple does for its own implementation?
}

void IGFX::wrapSystemDidWake(IOService *that) {
	DBGLOG("igfx", "systemDidWake GuC callback");

	// This is IGHardwareGuC class instance.
	auto &GuC = (reinterpret_cast<OSObject **>(that))[76];
	DBGLOG("igfx", "reloading firmware on wake discovered IGHardwareGuC %d", GuC != nullptr);
	if (GuC) {
		GuC->release();
		GuC = nullptr;
	}

	FunctionCast(wrapLoadFirmware, callbackIGFX->orgLoadFirmware)(that);
}

bool IGFX::wrapInitSchedControl(void *that, void *ctrl) {
	// This function is caled within loadGuCBinary, and it also uses shared buffers.
	// To avoid any issues here we ensure that the filtering is off.
	DBGLOG("igfx", "attempting to init sched control with load %d", callbackIGFX->performingFirmwareLoad);
	bool perfLoad = callbackIGFX->performingFirmwareLoad;
	callbackIGFX->performingFirmwareLoad = false;
	bool r = FunctionCast(wrapInitSchedControl, callbackIGFX->orgInitSchedControl)(that, ctrl);

#ifdef DEBUG
	if (callbackIGFX->fwLoadMode == FW_GENERIC) {
		struct ParamRegs {
			uint32_t bak[35];
			uint32_t params[10];
		};

		auto v = &static_cast<ParamRegs *>(that)->params[0];
		DBGLOG("igfx", "fw params1 %08X %08X %08X %08X %08X", v[0], v[1], v[2], v[3], v[4]);
		DBGLOG("igfx", "fw params2 %08X %08X %08X %08X %08X", v[5], v[6], v[7], v[8], v[9]);
	}
#endif

	callbackIGFX->performingFirmwareLoad = perfLoad;
	return r;
}

void *IGFX::wrapIgBufferWithOptions(void *accelTask, unsigned long size, unsigned int type, unsigned int flags) {
	auto cpuGeneration = BaseDeviceInfo::get().cpuGeneration;
	void *r = nullptr;

	if (callbackIGFX->performingFirmwareLoad) {
		// Allocate a dummy buffer
		callbackIGFX->dummyFirmwareBuffer = Buffer::create<uint8_t>(size);
		// Select the latest firmware to upload
		DBGLOG("igfx", "preparing firmware for cpu gen %d with range 0x%lX", cpuGeneration, size);

		const void *fw = nullptr;
		const void *fwsig = nullptr;
		size_t fwsize = 0;
		size_t fwsigsize = 0;

		if (cpuGeneration == CPUInfo::CpuGeneration::Skylake) {
			fw = GuCFirmwareSKL;
			fwsig = GuCFirmwareSKLSignature;
			fwsize = GuCFirmwareSKLSize;
			fwsigsize = GuCFirmwareSignatureSize;
		} else {
			fw = GuCFirmwareKBL;
			fwsig = GuCFirmwareKBLSignature;
			fwsize = GuCFirmwareKBLSize;
			fwsigsize = GuCFirmwareSignatureSize;
		}

		// Allocate enough memory for the new firmware (should be 64K-aligned)
		unsigned long newsize = fwsize > size ? ((fwsize + 0xFFFF) & (~0xFFFF)) : size;
		r = FunctionCast(wrapIgBufferWithOptions, callbackIGFX->orgIgBufferWithOptions)(accelTask, newsize, type, flags);
		// Replace the real buffer with a dummy buffer
		if (r && callbackIGFX->dummyFirmwareBuffer) {
			// Copy firmware contents, update the sizes and signature
			auto status = MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
			if (status == KERN_SUCCESS) {
				// Upload the firmware ourselves
				callbackIGFX->realFirmwareBuffer = static_cast<uint8_t **>(r)[7];
				static_cast<uint8_t **>(r)[7] = callbackIGFX->dummyFirmwareBuffer;
				lilu_os_memcpy(callbackIGFX->realFirmwareBuffer, fw, fwsize);
				lilu_os_memcpy(callbackIGFX->signaturePointer, fwsig, fwsigsize);
				callbackIGFX->realBinarySize = static_cast<uint32_t>(fwsize);
				// Update the firmware size for IGScheduler4
				*callbackIGFX->firmwareSizePointer = static_cast<uint32_t>(fwsize);
				MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
			} else {
				SYSLOG("igfx", "ig buffer protection upgrade failure %d", status);
			}
		} else if (callbackIGFX->dummyFirmwareBuffer) {
			SYSLOG("igfx", "ig shared buffer allocation failure");
			Buffer::deleter(callbackIGFX->dummyFirmwareBuffer);
			callbackIGFX->dummyFirmwareBuffer = nullptr;
		} else {
			SYSLOG("igfx", "dummy buffer allocation failure");
		}
	} else {
		r = FunctionCast(wrapIgBufferWithOptions, callbackIGFX->orgIgBufferWithOptions)(accelTask, size, type, flags);
	}

	return r;
}

uint64_t IGFX::wrapIgBufferGetGpuVirtualAddress(void *that) {
	if (callbackIGFX->performingFirmwareLoad && callbackIGFX->realFirmwareBuffer) {
		// Restore the original firmware buffer
		static_cast<uint8_t **>(that)[7] = callbackIGFX->realFirmwareBuffer;
		callbackIGFX->realFirmwareBuffer = nullptr;
		// Free the dummy framebuffer which is no longer used
		Buffer::deleter(callbackIGFX->dummyFirmwareBuffer);
		callbackIGFX->dummyFirmwareBuffer = nullptr;
	}

	return FunctionCast(wrapIgBufferGetGpuVirtualAddress, callbackIGFX->orgIgBufferGetGpuVirtualAddress)(that);
}

void IGFX::loadIGScheduler4Patches(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	gKmGen9GuCBinary = patcher.solveSymbol<uint8_t *>(index, "__ZL17__KmGen9GuCBinary", address, size);
	if (gKmGen9GuCBinary) {
		DBGLOG("igfx", "obtained __KmGen9GuCBinary");

		auto loadGuC = patcher.solveSymbol(index, "__ZN13IGHardwareGuC13loadGuCBinaryEv", address, size);
		if (loadGuC) {
			DBGLOG("igfx", "obtained IGHardwareGuC::loadGuCBinary");

			// Lookup the assignment to the size register.
			uint8_t sizeReg[] {0x10, 0xC3, 0x00, 0x00};
			auto pos    = reinterpret_cast<uint8_t *>(loadGuC);
			auto endPos = pos + PAGE_SIZE;
			while (memcmp(pos, sizeReg, sizeof(sizeReg)) != 0 && pos < endPos)
				pos++;

			// Verify and store the size pointer
			if (pos != endPos) {
				pos += sizeof(uint32_t);
				firmwareSizePointer = reinterpret_cast<uint32_t *>(pos);
				DBGLOG("igfx", "discovered firmware size: %u bytes", *firmwareSizePointer);
				// Firmware size must not be bigger than 1 MB
				if ((*firmwareSizePointer & 0xFFFFF) == *firmwareSizePointer)
					// Firmware follows the signature
					signaturePointer = gKmGen9GuCBinary + *firmwareSizePointer;
				else
					firmwareSizePointer = nullptr;
			}

			if (firmwareSizePointer) {
				orgLoadGuCBinary = patcher.routeFunction(loadGuC, reinterpret_cast<mach_vm_address_t>(wrapLoadGuCBinary), true);
				if (patcher.getError() == KernelPatcher::Error::NoError) {
					DBGLOG("igfx", "routed IGHardwareGuC::loadGuCBinary");

					KernelPatcher::RouteRequest requests[] {
						{"__ZN12IGScheduler412loadFirmwareEv", wrapLoadFirmware, orgLoadFirmware},
						{"__ZN13IGHardwareGuC16initSchedControlEv", wrapInitSchedControl, orgInitSchedControl},
						{"__ZN20IGSharedMappedBuffer11withOptionsEP11IGAccelTaskmjj", wrapIgBufferWithOptions, orgIgBufferWithOptions},
						{"__ZNK14IGMappedBuffer20getGPUVirtualAddressEv", wrapIgBufferGetGpuVirtualAddress, orgIgBufferGetGpuVirtualAddress},
					};
					patcher.routeMultiple(index, requests, address, size);
				} else {
					SYSLOG("igfx", "failed to route IGHardwareGuC::loadGuCBinary %d", patcher.getError());
					patcher.clearError();
				}
			} else {
				SYSLOG("igfx", "failed to find GuC firmware size assignment");
			}
		} else {
			SYSLOG("igfx", "failed to resolve IGHardwareGuC::loadGuCBinary %d", patcher.getError());
			patcher.clearError();
		}
	} else {
		SYSLOG("igfx", "failed to resoolve __KmGen9GuCBinary %d", patcher.getError());
		patcher.clearError();
	}
}

bool IGFX::loadPatchesFromDevice(IORegistryEntry *igpu, uint32_t currentFramebufferId) {
	bool hasFramebufferPatch = false;
	
	auto cpuGeneration = BaseDeviceInfo::get().cpuGeneration;

	uint32_t framebufferPatchEnable = 0;
	if (WIOKit::getOSDataValue(igpu, "framebuffer-patch-enable", framebufferPatchEnable) && framebufferPatchEnable) {
		DBGLOG("igfx", "framebuffer-patch-enable %d", framebufferPatchEnable);
		
		if (cpuGeneration == CPUInfo::CpuGeneration::Westmere) {
			hasFramebufferPatch = true;
			
			// First generation only has link mode and width patching.
			framebufferPatchFlags.bitsWestmere.LinkWidth = WIOKit::getOSDataValue(igpu, "framebuffer-linkwidth", framebufferWestmerePatches.LinkWidth);
			framebufferPatchFlags.bitsWestmere.SingleLink = WIOKit::getOSDataValue(igpu, "framebuffer-singlelink", framebufferWestmerePatches.SingleLink);
			
			framebufferPatchFlags.bitsWestmere.FBCControlCompression =
				WIOKit::getOSDataValue(igpu, "framebuffer-fbccontrol-compression", framebufferWestmerePatches.FBCControlCompression);
			framebufferPatchFlags.bitsWestmere.FeatureControlFBC =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-fbc", framebufferWestmerePatches.FeatureControlFBC);
			framebufferPatchFlags.bitsWestmere.FeatureControlGPUInterruptHandling =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-gpuinterrupthandling", framebufferWestmerePatches.FeatureControlGPUInterruptHandling);
			framebufferPatchFlags.bitsWestmere.FeatureControlGamma =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-gamma", framebufferWestmerePatches.FeatureControlGamma);
			framebufferPatchFlags.bitsWestmere.FeatureControlMaximumSelfRefreshLevel =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-maximumselfrefreshlevel", framebufferWestmerePatches.FeatureControlMaximumSelfRefreshLevel);
			framebufferPatchFlags.bitsWestmere.FeatureControlPowerStates =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-powerstates", framebufferWestmerePatches.FeatureControlPowerStates);
			framebufferPatchFlags.bitsWestmere.FeatureControlRSTimerTest =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-rstimertest", framebufferWestmerePatches.FeatureControlRSTimerTest);
			framebufferPatchFlags.bitsWestmere.FeatureControlRenderStandby =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-renderstandby", framebufferWestmerePatches.FeatureControlRenderStandby);
			framebufferPatchFlags.bitsWestmere.FeatureControlWatermarks =
				WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-watermarks", framebufferWestmerePatches.FeatureControlWatermarks);
			
			// Settings above will override all-zero settings.
			uint32_t fbcControlAllZero = 0;
			if (WIOKit::getOSDataValue(igpu, "framebuffer-fbccontrol-allzero", fbcControlAllZero) && fbcControlAllZero) {
				framebufferPatchFlags.bitsWestmere.FBCControlCompression = 1;
			}
			
			// Settings above will override all-zero settings.
			uint32_t featureControlAllZero = 0;
			if (WIOKit::getOSDataValue(igpu, "framebuffer-featurecontrol-allzero", featureControlAllZero) && featureControlAllZero) {
				framebufferPatchFlags.bitsWestmere.FeatureControlFBC = 1;
				framebufferPatchFlags.bitsWestmere.FeatureControlGPUInterruptHandling = 1;
				framebufferPatchFlags.bitsWestmere.FeatureControlGamma = 1;
				framebufferPatchFlags.bitsWestmere.FeatureControlMaximumSelfRefreshLevel = 1;
				framebufferPatchFlags.bitsWestmere.FeatureControlPowerStates = 1;
				framebufferPatchFlags.bitsWestmere.FeatureControlRSTimerTest = 1;
				framebufferPatchFlags.bitsWestmere.FeatureControlRenderStandby = 1;
				framebufferPatchFlags.bitsWestmere.FeatureControlWatermarks = 1;
			}
		} else if (cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge) {
			// Note, the casts to uint32_t here and below are required due to device properties always injecting 32-bit types.
			framebufferPatchFlags.bits.FPFFramebufferId = WIOKit::getOSDataValue(igpu, "framebuffer-framebufferid", framebufferPatch.framebufferId);
			framebufferPatchFlags.bits.FPFFlags = WIOKit::getOSDataValue(igpu, "framebuffer-flags", framebufferPatch.flags.value);
			framebufferPatchFlags.bits.FPFCamelliaVersion = WIOKit::getOSDataValue(igpu, "framebuffer-camellia", framebufferPatch.camelliaVersion);
			framebufferPatchFlags.bits.FPFMobile = WIOKit::getOSDataValue<uint32_t>(igpu, "framebuffer-mobile", framebufferPatch.fMobile);
			framebufferPatchFlags.bits.FPFPipeCount = WIOKit::getOSDataValue<uint32_t>(igpu, "framebuffer-pipecount", framebufferPatch.fPipeCount);
			framebufferPatchFlags.bits.FPFPortCount = WIOKit::getOSDataValue<uint32_t>(igpu, "framebuffer-portcount", framebufferPatch.fPortCount);
			framebufferPatchFlags.bits.FPFFBMemoryCount = WIOKit::getOSDataValue<uint32_t>(igpu, "framebuffer-memorycount", framebufferPatch.fFBMemoryCount);
			framebufferPatchFlags.bits.FPFStolenMemorySize = WIOKit::getOSDataValue(igpu, "framebuffer-stolenmem", framebufferPatch.fStolenMemorySize);
			framebufferPatchFlags.bits.FPFFramebufferMemorySize = WIOKit::getOSDataValue(igpu, "framebuffer-fbmem", framebufferPatch.fFramebufferMemorySize);
			framebufferPatchFlags.bits.FPFUnifiedMemorySize = WIOKit::getOSDataValue(igpu, "framebuffer-unifiedmem", framebufferPatch.fUnifiedMemorySize);
			framebufferPatchFlags.bits.FPFFramebufferCursorSize = WIOKit::getOSDataValue(igpu, "framebuffer-cursormem", fPatchCursorMemorySize);

			if (framebufferPatchFlags.value != 0)
				hasFramebufferPatch = true;

			for (size_t i = 0; i < arrsize(framebufferPatch.connectors); i++) {
				char name[48];
				snprintf(name, sizeof(name), "framebuffer-con%lu-enable", i);
				uint32_t framebufferConnectorPatchEnable = 0;
				if (!WIOKit::getOSDataValue(igpu, name, framebufferConnectorPatchEnable) || !framebufferConnectorPatchEnable)
					continue;

				DBGLOG("igfx", "framebuffer-con%lu-enable %d", i, framebufferConnectorPatchEnable);

				snprintf(name, sizeof(name), "framebuffer-con%lu-%08x-alldata", i, currentFramebufferId);
				auto allData = OSDynamicCast(OSData, igpu->getProperty(name));
				if (!allData) {
					snprintf(name, sizeof(name), "framebuffer-con%lu-alldata", i);
					allData = OSDynamicCast(OSData, igpu->getProperty(name));
				}
				if (allData) {
					auto allDataSize = allData->getLength();
					auto replaceCount = allDataSize / sizeof(ConnectorInfo);
					if (0 == allDataSize % sizeof(ConnectorInfo) && i + replaceCount <= arrsize(framebufferPatch.connectors)) {
						auto replacementConnectors = reinterpret_cast<const ConnectorInfo*>(allData->getBytesNoCopy());
						for (size_t j = 0; j < replaceCount; j++) {
							framebufferPatch.connectors[i+j].index = replacementConnectors[j].index;
							framebufferPatch.connectors[i+j].busId = replacementConnectors[j].busId;
							framebufferPatch.connectors[i+j].pipe = replacementConnectors[j].pipe;
							framebufferPatch.connectors[i+j].pad = replacementConnectors[j].pad;
							framebufferPatch.connectors[i+j].type = replacementConnectors[j].type;
							framebufferPatch.connectors[i+j].flags = replacementConnectors[j].flags;
							connectorPatchFlags[i+j].bits.CPFIndex = true;
							connectorPatchFlags[i+j].bits.CPFBusId = true;
							connectorPatchFlags[i+j].bits.CPFPipe = true;
							connectorPatchFlags[i+j].bits.CPFType = true;
							connectorPatchFlags[i+j].bits.CPFFlags = true;
						}
					}
				}

				snprintf(name, sizeof(name), "framebuffer-con%lu-index", i);
				connectorPatchFlags[i].bits.CPFIndex |= WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].index);
				snprintf(name, sizeof(name), "framebuffer-con%lu-busid", i);
				connectorPatchFlags[i].bits.CPFBusId |= WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].busId);
				snprintf(name, sizeof(name), "framebuffer-con%lu-pipe", i);
				connectorPatchFlags[i].bits.CPFPipe |= WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].pipe);
				snprintf(name, sizeof(name), "framebuffer-con%lu-type", i);
				connectorPatchFlags[i].bits.CPFType |= WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].type);
				snprintf(name, sizeof(name), "framebuffer-con%lu-flags", i);
				connectorPatchFlags[i].bits.CPFFlags |= WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].flags.value);

				if (connectorPatchFlags[i].value != 0)
					hasFramebufferPatch = true;
			}
		}
	}

	if (cpuGeneration >= CPUInfo::CpuGeneration::SandyBridge) {
		size_t patchIndex = 0;
		for (size_t i = 0; i < MaxFramebufferPatchCount; i++) {
			char name[48];
			snprintf(name, sizeof(name), "framebuffer-patch%lu-enable", i);
			// Missing status means no patches at all.
			uint32_t framebufferPatchEnable = 0;
			if (!WIOKit::getOSDataValue(igpu, name, framebufferPatchEnable))
				break;

			// False status means a temporarily disabled patch, skip for next one.
			if (!framebufferPatchEnable)
				continue;

			uint32_t framebufferId = 0;
			size_t framebufferPatchCount = 0;

			snprintf(name, sizeof(name), "framebuffer-patch%lu-framebufferid", i);
			bool passedFramebufferId = WIOKit::getOSDataValue(igpu, name, framebufferId);
			snprintf(name, sizeof(name), "framebuffer-patch%lu-find", i);
			auto framebufferPatchFind = OSDynamicCast(OSData, igpu->getProperty(name));
			snprintf(name, sizeof(name), "framebuffer-patch%lu-replace", i);
			auto framebufferPatchReplace = OSDynamicCast(OSData, igpu->getProperty(name));
			snprintf(name, sizeof(name), "framebuffer-patch%lu-count", i);
			(void)WIOKit::getOSDataValue(igpu, name, framebufferPatchCount);

			if (!framebufferPatchFind || !framebufferPatchReplace)
				continue;

			framebufferPatches[patchIndex].framebufferId = (passedFramebufferId ? framebufferId : currentFramebufferId);
			framebufferPatches[patchIndex].find = framebufferPatchFind;
			framebufferPatches[patchIndex].replace = framebufferPatchReplace;
			framebufferPatches[patchIndex].count = (framebufferPatchCount ? framebufferPatchCount : 1);

			framebufferPatchFind->retain();
			framebufferPatchReplace->retain();

			hasFramebufferPatch = true;

			patchIndex++;
		}
	}

	return hasFramebufferPatch;
}

uint8_t *IGFX::findFramebufferId(uint32_t framebufferId, uint8_t *startingAddress, size_t maxSize) {
	uint32_t *startAddress = reinterpret_cast<uint32_t *>(startingAddress);
	uint32_t *endAddress = reinterpret_cast<uint32_t *>(startingAddress + maxSize);
	while (startAddress < endAddress) {
		if (*startAddress == framebufferId)
			return reinterpret_cast<uint8_t *>(startAddress);
		startAddress++;
	}

	return nullptr;
}

#ifdef DEBUG
size_t IGFX::calculatePlatformListSize(size_t maxSize) {
	// sanity check maxSize
	if (maxSize < sizeof(uint32_t)*2)
		return maxSize;
	// ig-platform-id table ends with 0xFFFFF, but to avoid false positive
	// look for FFFFFFFF 00000000
	// and Sandy Bridge is special, ending in 00000000 000c0c0c
	uint8_t * startingAddress = reinterpret_cast<uint8_t *>(gPlatformInformationList);
	uint32_t *startAddress = reinterpret_cast<uint32_t *>(startingAddress);
	uint32_t *endAddress = reinterpret_cast<uint32_t *>(startingAddress + maxSize - sizeof(uint32_t));
	while (startAddress < endAddress) {
		if ((!gPlatformListIsSNB && 0xffffffff == startAddress[0] && 0 == startAddress[1]) ||
			(gPlatformListIsSNB && 0 == startAddress[0] && 0x0c0c0c00 == startAddress[1]))
			return reinterpret_cast<uint8_t *>(startAddress) - startingAddress + sizeof(uint32_t)*2;
		startAddress++;
	}

	return maxSize; // in case of no termination, just return maxSize
}

void IGFX::writePlatformListData(const char *subKeyName) {
	if (BaseDeviceInfo::get().cpuGeneration < CPUInfo::CpuGeneration::SandyBridge) {
		DBGLOG("igfx", "writePlatformListData unsupported below Sandy bridge");
		return;
	}
	
	auto entry = IORegistryEntry::fromPath("IOService:/IOResources/WhateverGreen");
	if (entry) {
		entry->setProperty(subKeyName, gPlatformInformationList, static_cast<unsigned>(calculatePlatformListSize(PAGE_SIZE)));
		entry->release();
	}
}
#endif

bool IGFX::applyPatch(const KernelPatcher::LookupPatch &patch, uint8_t *startingAddress, size_t maxSize) {
	bool r = false;
	size_t i = 0, patchCount = 0;
	uint8_t *startAddress = startingAddress;
	uint8_t *endAddress = startingAddress + maxSize - patch.size;

	if (startAddress < framebufferStart)
		startAddress = framebufferStart;
	if (endAddress > framebufferStart + framebufferSize)
		endAddress = framebufferStart + framebufferSize;

	while (startAddress < endAddress) {
		for (i = 0; i < patch.size; i++) {
			if (startAddress[i] != patch.find[i])
				break;
		}
		if (i == patch.size) {
			for (i = 0; i < patch.size; i++)
				startAddress[i] = patch.replace[i];

			r = true;

			if (++patchCount >= patch.count)
				break;

			startAddress += patch.size;
			continue;
		}

		startAddress++;
	}

	return r;
}

bool IGFX::setDictUInt32(OSDictionary *dict, const char *key, UInt32 value) {
    auto *num = OSNumber::withNumber(value, sizeof(UInt32));
	if (!num)
		return false;
	
	bool success = dict->setObject(key, num);
	num->release();
	return success;
}

template <>
bool IGFX::applyPlatformInformationListPatch(uint32_t framebufferId, FramebufferSNB *platformInformationList) {
	bool framebufferFound = false;

	for (size_t i = 0; i < SandyPlatformNum; i++) {
		if (sandyPlatformId[i] == framebufferId) {
			if (framebufferPatchFlags.bits.FPFMobile)
				platformInformationList[i].fMobile = framebufferPatch.fMobile;

			if (framebufferPatchFlags.bits.FPFPipeCount)
				platformInformationList[i].fPipeCount = framebufferPatch.fPipeCount;

			if (framebufferPatchFlags.bits.FPFPortCount)
				platformInformationList[i].fPortCount = framebufferPatch.fPortCount;

			if (framebufferPatchFlags.bits.FPFFBMemoryCount)
				platformInformationList[i].fFBMemoryCount = framebufferPatch.fFBMemoryCount;

			for (size_t j = 0; j < arrsize(platformInformationList[i].connectors); j++) {
				if (connectorPatchFlags[j].bits.CPFIndex)
					platformInformationList[i].connectors[j].index = framebufferPatch.connectors[j].index;

				if (connectorPatchFlags[j].bits.CPFBusId)
					platformInformationList[i].connectors[j].busId = framebufferPatch.connectors[j].busId;

				if (connectorPatchFlags[j].bits.CPFPipe)
					platformInformationList[i].connectors[j].pipe = framebufferPatch.connectors[j].pipe;

				if (connectorPatchFlags[j].bits.CPFType)
					platformInformationList[i].connectors[j].type = framebufferPatch.connectors[j].type;

				if (connectorPatchFlags[j].bits.CPFFlags)
					platformInformationList[i].connectors[j].flags = framebufferPatch.connectors[j].flags;

				if (connectorPatchFlags[j].value) {
					DBGLOG("igfx", "patching framebufferId 0x%08X connector [%d] busId: 0x%02X, pipe: %u, type: 0x%08X, flags: 0x%08X", framebufferId, platformInformationList[i].connectors[j].index, platformInformationList[i].connectors[j].busId, platformInformationList[i].connectors[j].pipe, platformInformationList[i].connectors[j].type, platformInformationList[i].connectors[j].flags.value);

					framebufferFound = true;
				}
			}

			if (framebufferPatchFlags.value) {
				DBGLOG("igfx", "patching framebufferId 0x%08X", framebufferId);
				DBGLOG("igfx", "mobile: 0x%08X", platformInformationList[i].fMobile);
				DBGLOG("igfx", "pipeCount: %u", platformInformationList[i].fPipeCount);
				DBGLOG("igfx", "portCount: %u", platformInformationList[i].fPortCount);
				DBGLOG("igfx", "fbMemoryCount: %u", platformInformationList[i].fFBMemoryCount);

				framebufferFound = true;
			}
		}
	}

	return framebufferFound;
}

// Sandy and Ivy have no flags
template <>
void IGFX::applyPlatformInformationPatchEx(FramebufferSNB *frame) {}

template <>
void IGFX::applyPlatformInformationPatchEx(FramebufferIVB *frame) {}

template <>
void IGFX::applyPlatformInformationPatchEx(FramebufferHSW *frame) {
	// fCursorMemorySize is Haswell specific
	if (framebufferPatchFlags.bits.FPFFramebufferCursorSize) {
		frame->fCursorMemorySize = fPatchCursorMemorySize;
		DBGLOG("igfx", "fCursorMemorySize: 0x%08X", frame->fCursorMemorySize);
	}

	if (framebufferPatchFlags.bits.FPFFlags)
		frame->flags.value = framebufferPatch.flags.value;

	if (framebufferPatchFlags.bits.FPFCamelliaVersion)
		frame->camelliaVersion = framebufferPatch.camelliaVersion;
}

template <typename T>
void IGFX::applyPlatformInformationPatchEx(T *frame) {
	if (framebufferPatchFlags.bits.FPFFlags)
		frame->flags.value = framebufferPatch.flags.value;


	if (framebufferPatchFlags.bits.FPFCamelliaVersion)
		frame->camelliaVersion = framebufferPatch.camelliaVersion;
}

template <typename T>
bool IGFX::applyPlatformInformationListPatch(uint32_t framebufferId, T *platformInformationList) {
	auto frame = reinterpret_cast<T *>(findFramebufferId(framebufferId, reinterpret_cast<uint8_t *>(platformInformationList), PAGE_SIZE));
	if (!frame)
		return false;

	bool r = false;

	if (framebufferPatchFlags.bits.FPFMobile)
		frame->fMobile = framebufferPatch.fMobile;

	if (framebufferPatchFlags.bits.FPFPipeCount)
		frame->fPipeCount = framebufferPatch.fPipeCount;

	if (framebufferPatchFlags.bits.FPFPortCount)
		frame->fPortCount = framebufferPatch.fPortCount;

	if (framebufferPatchFlags.bits.FPFFBMemoryCount)
		frame->fFBMemoryCount = framebufferPatch.fFBMemoryCount;

	if (framebufferPatchFlags.bits.FPFStolenMemorySize)
		frame->fStolenMemorySize = framebufferPatch.fStolenMemorySize;

	if (framebufferPatchFlags.bits.FPFFramebufferMemorySize)
		frame->fFramebufferMemorySize = framebufferPatch.fFramebufferMemorySize;

	if (framebufferPatchFlags.bits.FPFUnifiedMemorySize)
		frame->fUnifiedMemorySize = framebufferPatch.fUnifiedMemorySize;

	if (framebufferPatchFlags.value) {
		DBGLOG("igfx", "patching framebufferId 0x%08X", frame->framebufferId);
		DBGLOG("igfx", "mobile: 0x%08X", frame->fMobile);
		DBGLOG("igfx", "pipeCount: %u", frame->fPipeCount);
		DBGLOG("igfx", "portCount: %u", frame->fPortCount);
		DBGLOG("igfx", "fbMemoryCount: %u", frame->fFBMemoryCount);
		DBGLOG("igfx", "stolenMemorySize: 0x%08X", frame->fStolenMemorySize);
		DBGLOG("igfx", "framebufferMemorySize: 0x%08X", frame->fFramebufferMemorySize);
		DBGLOG("igfx", "unifiedMemorySize: 0x%08X", frame->fUnifiedMemorySize);

		r = true;
	}

	applyPlatformInformationPatchEx(frame);

	for (size_t j = 0; j < arrsize(frame->connectors); j++) {
		if (connectorPatchFlags[j].bits.CPFIndex)
			frame->connectors[j].index = framebufferPatch.connectors[j].index;

		if (connectorPatchFlags[j].bits.CPFBusId)
			frame->connectors[j].busId = framebufferPatch.connectors[j].busId;

		if (connectorPatchFlags[j].bits.CPFPipe)
			frame->connectors[j].pipe = framebufferPatch.connectors[j].pipe;

		if (connectorPatchFlags[j].bits.CPFType)
			frame->connectors[j].type = framebufferPatch.connectors[j].type;

		if (connectorPatchFlags[j].bits.CPFFlags)
			frame->connectors[j].flags = framebufferPatch.connectors[j].flags;

		if (connectorPatchFlags[j].value) {
			DBGLOG("igfx", "patching framebufferId 0x%08X connector [%d] busId: 0x%02X, pipe: %u, type: 0x%08X, flags: 0x%08X", frame->framebufferId, frame->connectors[j].index, frame->connectors[j].busId, frame->connectors[j].pipe, frame->connectors[j].type, frame->connectors[j].flags.value);

			r = true;
		}
	}

	return r;
}

template <>
bool IGFX::applyDPtoHDMIPatch(uint32_t framebufferId, FramebufferSNB *platformInformationList) {
	bool found = false;

	for (size_t i = 0; i < SandyPlatformNum; i++) {
		if (sandyPlatformId[i] == framebufferId) {
			for (size_t j = 0; j < arrsize(platformInformationList[i].connectors); j++) {
				DBGLOG("igfx", "snb connector [%lu] busId: 0x%02X, pipe: %d, type: 0x%08X, flags: 0x%08X", j, platformInformationList[i].connectors[j].busId, platformInformationList[i].connectors[j].pipe,
					   platformInformationList[i].connectors[j].type, platformInformationList[i].connectors[j].flags);

				if (platformInformationList[i].connectors[j].type == ConnectorDP) {
					platformInformationList[i].connectors[j].type = ConnectorHDMI;
					DBGLOG("igfx", "replaced snb connector %lu type from DP to HDMI", j);
					found = true;
				}
			}
		}
	}

	return found;
}

template <typename T>
bool IGFX::applyDPtoHDMIPatch(uint32_t framebufferId, T *platformInformationList) {
	auto frame = reinterpret_cast<T *>(findFramebufferId(framebufferId, reinterpret_cast<uint8_t *>(platformInformationList), PAGE_SIZE));
	if (!frame)
		return false;

	bool found = false;
	for (size_t i = 0; i < arrsize(frame->connectors); i++) {
		DBGLOG("igfx", "connector [%lu] busId: 0x%02X, pipe: %d, type: 0x%08X, flags: 0x%08X", i, platformInformationList[i].connectors[i].busId, platformInformationList[i].connectors[i].pipe,
			   platformInformationList[i].connectors[i].type, platformInformationList[i].connectors[i].flags);

		if (frame->connectors[i].type == ConnectorDP) {
			frame->connectors[i].type = ConnectorHDMI;
			DBGLOG("igfx", "replaced connector %lu type from DP to HDMI", i);
			found = true;
		}
	}

	return found;
}

void IGFX::applyFramebufferPatches() {
	auto cpuGeneration = BaseDeviceInfo::get().cpuGeneration;
	uint32_t framebufferId = framebufferPatch.framebufferId;

	// Not tested prior to 10.10.5, and definitely different on 10.9.5 at least.
	if (getKernelVersion() >= KernelVersion::Yosemite) {
		bool success = false;
		if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferSNB *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::IvyBridge)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferIVB *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::Haswell)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferHSW *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::Broadwell)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferBDW *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::Skylake || cpuGeneration == CPUInfo::CpuGeneration::KabyLake ||
				 (cpuGeneration == CPUInfo::CpuGeneration::CoffeeLake && static_cast<FramebufferSKL *>(gPlatformInformationList)->framebufferId == 0x591E0000))
			//FIXME: write this in a nicer way (coffee pretending to be Kaby, detecting via first kaby frame)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferSKL *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::CoffeeLake)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferCFL *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::CometLake)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferCFL *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::CannonLake)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferCNL *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::IceLake) {
			// FIXME: Need to address possible circumstance of both ICL kexts loaded at the same time
			if (callbackIGFX->currentFramebuffer->loadIndex != KernelPatcher::KextInfo::Unloaded)
				success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferICLLP *>(gPlatformInformationList));
			else if (callbackIGFX->currentFramebufferOpt->loadIndex != KernelPatcher::KextInfo::Unloaded)
				success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferICLHP *>(gPlatformInformationList));
		}

		if (success)
			DBGLOG("igfx", "patching framebufferId 0x%08X successful", framebufferId);
		else
			DBGLOG("igfx", "patching framebufferId 0x%08X failed", framebufferId);
	}

	uint8_t *platformInformationAddress = findFramebufferId(framebufferId, static_cast<uint8_t *>(gPlatformInformationList), PAGE_SIZE);
	if (platformInformationAddress) {
		for (size_t i = 0; i < MaxFramebufferPatchCount; i++) {
			if (!framebufferPatches[i].find || !framebufferPatches[i].replace)
				continue;

			if (framebufferPatches[i].framebufferId != framebufferId)    {
				framebufferId = framebufferPatches[i].framebufferId;
				platformInformationAddress = findFramebufferId(framebufferId, static_cast<uint8_t *>(gPlatformInformationList), PAGE_SIZE);
			}

			if (!platformInformationAddress) {
				DBGLOG("igfx", "patch %lu framebufferId 0x%08X not found", i, framebufferId);
				continue;
			}

			if (framebufferPatches[i].find->getLength() != framebufferPatches[i].replace->getLength()) {
				DBGLOG("igfx", "patch %lu framebufferId 0x%08X length mistmatch", i, framebufferId);
				continue;
			}

			KernelPatcher::LookupPatch patch {};
			patch.kext = currentFramebuffer;
			patch.find = static_cast<const uint8_t *>(framebufferPatches[i].find->getBytesNoCopy());
			patch.replace = static_cast<const uint8_t *>(framebufferPatches[i].replace->getBytesNoCopy());
			patch.size = framebufferPatches[i].find->getLength();
			patch.count = framebufferPatches[i].count;

			if (applyPatch(patch, platformInformationAddress, PAGE_SIZE))
				DBGLOG("igfx", "patch %lu framebufferId 0x%08X successful", i, framebufferId);
			else
				DBGLOG("igfx", "patch %lu framebufferId 0x%08X failed", i, framebufferId);

			framebufferPatches[i].find->release();
			framebufferPatches[i].find = nullptr;
			framebufferPatches[i].replace->release();
			framebufferPatches[i].replace = nullptr;
		}
	}
}

void IGFX::applyHdmiAutopatch() {
	auto cpuGeneration = BaseDeviceInfo::get().cpuGeneration;
	uint32_t framebufferId = framebufferPatch.framebufferId;

	DBGLOG("igfx", "applyHdmiAutopatch framebufferId %X cpugen %X", framebufferId, cpuGeneration);

	bool success = false;
	if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferSNB *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::IvyBridge)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferIVB *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::Haswell)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferHSW *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::Broadwell)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferBDW *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::Skylake || cpuGeneration == CPUInfo::CpuGeneration::KabyLake ||
			 (cpuGeneration == CPUInfo::CpuGeneration::CoffeeLake && static_cast<FramebufferSKL *>(gPlatformInformationList)->framebufferId == 0x591E0000))
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferSKL *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::CoffeeLake)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferCFL *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::CometLake)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferCFL *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::CannonLake)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferCNL *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::IceLake) {
		// FIXME: Need to address possible circumstance of both ICL kexts loaded at the same time
		if (callbackIGFX->currentFramebuffer->loadIndex != KernelPatcher::KextInfo::Unloaded)
			success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferICLLP *>(gPlatformInformationList));
		else if (callbackIGFX->currentFramebufferOpt->loadIndex != KernelPatcher::KextInfo::Unloaded)
			success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferICLHP *>(gPlatformInformationList));
	}

	if (success)
		DBGLOG("igfx", "hdmi patching framebufferId 0x%08X successful", framebufferId);
	else
		DBGLOG("igfx", "hdmi patching framebufferId 0x%08X failed", framebufferId);
}

void IGFX::applyWestmerePatches(KernelPatcher &patcher) {
	auto kernelVersion = getKernelVersion();
	DBGLOG("igfx", "applyWestmerePatches kernel version %X", kernelVersion);
	
	//
	// Reference: https://github.com/Goldfish64/ArrandaleGraphicsHackintosh/blob/master/Patches.md
	//
	
	// Use 0x2000 for stride (fixes artifacts on resolutions > 1024x768). Located in AppleIntelHDGraphicsFB::hwSetCRTC.
	const uint8_t findStride[] = { 0x0F, 0x45, 0xC8, 0x42, 0x89, 0x8C };
	const uint8_t replaceStride[] = { 0x90, 0x90, 0x90, 0x42, 0x89, 0x8C };
	KernelPatcher::LookupPatch stridePatch { currentFramebuffer, findStride, replaceStride, sizeof(findStride), 1 };
	patcher.applyLookupPatch(&stridePatch);
	DBGLOG("igfx", "applyWestmerePatches applied stride patch");
	
	if (framebufferPatchFlags.bitsWestmere.SingleLink && framebufferWestmerePatches.SingleLink) {
		// AAPL00,DualLink is set to <00000000> instead of patch 1.
		
		// Single link patch 2. Changes to divisor 14. Located in AppleIntelHDGraphicsFB::hwRegsNeedUpdate.
		if (kernelVersion == KernelVersion::MountainLion) {
			const uint8_t findSingleWidth2Ml[] = { 0xB9, 0x00, 0x00, 0x00, 0x09 };
			const uint8_t replaceSingleWidth2Ml[] = { 0xB9, 0x00, 0x00, 0x00, 0x08 };
			KernelPatcher::LookupPatch singleWidth2MlPatch { currentFramebuffer, findSingleWidth2Ml, replaceSingleWidth2Ml, sizeof(findSingleWidth2Ml), 1 };
			patcher.applyLookupPatch(&singleWidth2MlPatch);
			DBGLOG("igfx", "applyWestmerePatches applied single width patch 2 for Mountain Lion");
		}
		else if (kernelVersion == KernelVersion::Mavericks) {
			const uint8_t findSingleWidth2Mav[] = { 0x41, 0xB9, 0x00, 0x60, 0x00, 0x09 };
			const uint8_t replaceSingleWidth2Mav[] = { 0x41, 0xB9, 0x00, 0x60, 0x00, 0x08 };
			KernelPatcher::LookupPatch singleWidth2MavPatch { currentFramebuffer, findSingleWidth2Mav, replaceSingleWidth2Mav, sizeof(findSingleWidth2Mav), 1 };
			patcher.applyLookupPatch(&singleWidth2MavPatch);
			DBGLOG("igfx", "applyWestmerePatches applied single width patch 2 for Mavericks");
		}
		else if (kernelVersion >= KernelVersion::Yosemite && kernelVersion <= KernelVersion::Sierra) {
			const uint8_t findSingleWidth2Yos[] = { 0xB8, 0x00, 0x60, 0x00, 0x09 };
			const uint8_t replaceSingleWidth2Yos[] = { 0xB8, 0x00, 0x60, 0x00, 0x08 };
			KernelPatcher::LookupPatch singleWidth2YosPatch { currentFramebuffer, findSingleWidth2Yos, replaceSingleWidth2Yos, sizeof(findSingleWidth2Yos), 1 };
			patcher.applyLookupPatch(&singleWidth2YosPatch);
			DBGLOG("igfx", "applyWestmerePatches applied single width patch 2 for Yosemite to Sierra");
		} else if (kernelVersion >= KernelVersion::HighSierra) {
			const uint8_t findSingleWidth2Hs[] = { 0xBA, 0x00, 0x60, 0x00, 0x09 };
			const uint8_t replaceSingleWidth2Hs[] = { 0xBA, 0x00, 0x60, 0x00, 0x08 };
			KernelPatcher::LookupPatch singleWidth2HsPatch { currentFramebuffer, findSingleWidth2Hs, replaceSingleWidth2Hs, sizeof(findSingleWidth2Hs), 1 };
			patcher.applyLookupPatch(&singleWidth2HsPatch);
			DBGLOG("igfx", "applyWestmerePatches applied single width patch 2 for High Sierra+");
		}
		
		// Single link patch 3. Powers down CLKB (fixes pixelated image). Located in AppleIntelHDGraphicsFB::hwRegsNeedUpdate.
		const uint8_t findSingleWidth3[] = { 0x3C, 0x03, 0x30, 0x80 };
		const uint8_t replaceSingleWidth3[] = { 0x00, 0x03, 0x30, 0x80 };
		KernelPatcher::LookupPatch singleWidth3Patch { currentFramebuffer, findSingleWidth3, replaceSingleWidth3, sizeof(findSingleWidth3), 1 };
		patcher.applyLookupPatch(&singleWidth3Patch);
		DBGLOG("igfx", "applyWestmerePatches applied single width patch 3");
	}
	
	// Cap link width.
	if (framebufferWestmerePatches.LinkWidth == 0 || framebufferWestmerePatches.LinkWidth > 4) {
		SYSLOG("igfx", "applyWestmerePatches link width of %u is invalid; using 1", framebufferWestmerePatches.LinkWidth);
		framebufferWestmerePatches.LinkWidth = 1;
	}

	// Link width patch. Sets the link width. Located in AppleIntelHDGraphicsFB::TrainFDI.
	// Formula =  ((link_width - 1) & 7) << 19.
	uint32_t linkWidth = ((framebufferWestmerePatches.LinkWidth - 1) & 7) << 19;
	uint8_t *linkWidthPtr = (uint8_t*)&linkWidth;
	if (kernelVersion >= KernelVersion::MountainLion && kernelVersion <= KernelVersion::ElCapitan) {
		const uint8_t findLinkWidthMl[] = { 0x49, 0x8B, 0x84, 0x24, 0x98, 0x06, 0x00, 0x00, 0x0F, 0xB6, 0x40, 0x18, 0xC1, 0xE0, 0x13, 0x41, 0x0B, 0x46, 0x6C, 0x41, 0x89, 0x46, 0x6C, 0x49, 0x8B, 0x8C, 0x24, 0x98, 0x00, 0x00, 0x00, 0x89, 0x81, 0x0C, 0x00, 0x0F, 0x00, 0x49, 0x8B, 0x84, 0x24, 0x98, 0x06, 0x00, 0x00, 0x0F, 0xB6, 0x40, 0x18, 0xC1, 0xE0, 0x13, 0x41, 0x0B, 0x46, 0x68 };
		const uint8_t replaceLinkWidthMl[] = { 0x41, 0x8B, 0x46, 0x6C, 0x25, 0xFF, 0xFF, 0xC7, 0xFF, 0x0D, linkWidthPtr[3], linkWidthPtr[2], linkWidthPtr[1], linkWidthPtr[0], 0x90, 0x90, 0x90, 0x90, 0x90, 0x41, 0x89, 0x46, 0x6C, 0x49, 0x8B, 0x8C, 0x24, 0x98, 0x00, 0x00, 0x00, 0x89, 0x81, 0x0C, 0x00, 0x0F, 0x00, 0x41, 0x8B, 0x46, 0x68, 0x25, 0xFF, 0xFF, 0xC7, 0xFF, 0x0D, linkWidthPtr[3], linkWidthPtr[2], linkWidthPtr[1], linkWidthPtr[0], 0x90, 0x90, 0x90, 0x90, 0x90 };
		KernelPatcher::LookupPatch linkWidthPatchMl { currentFramebuffer, findLinkWidthMl, replaceLinkWidthMl, sizeof(findLinkWidthMl), 1 };
		patcher.applyLookupPatch(&linkWidthPatchMl);
		DBGLOG("igfx", "applyWestmerePatches applied link width patch for Mountain Lion to El Capitan");
	} else if (kernelVersion >= KernelVersion::Sierra) {
		const uint8_t findLinkWidthSi[] = { 0x41, 0x89, 0x4E, 0x6C, 0x49, 0x8B, 0x84, 0x24, 0x98, 0x00, 0x00, 0x00, 0x89, 0x88, 0x0C, 0x00, 0x0F, 0x00, 0x49, 0x8B, 0x8C, 0x24, 0x98, 0x06, 0x00, 0x00, 0x0F, 0xB6, 0x51, 0x18, 0xC1, 0xE2, 0x13, 0x41, 0x8B, 0x76, 0x6C, 0x09, 0xD6, 0x41, 0x89, 0x76, 0x6C, 0x89, 0xB0, 0x0C, 0x00, 0x0F, 0x00, 0x41, 0x0B, 0x56, 0x68 };
		const uint8_t replaceLinkWidthSi[] = { 0xBB, 0xFF, 0xFF, 0xC7, 0xFF, 0xBA, linkWidthPtr[3], linkWidthPtr[2], linkWidthPtr[1], linkWidthPtr[0], 0x21, 0xD9, 0x09, 0xD1, 0x41, 0x89, 0x4E, 0x6C, 0x49, 0x8B, 0x84, 0x24, 0x98, 0x00, 0x00, 0x00, 0x89, 0x88, 0x0C, 0x00, 0x0F, 0x00, 0x41, 0x8B, 0x4E, 0x68, 0x21, 0xD9, 0x09, 0xD1, 0x89, 0xCA, 0x90, 0x90, 0x90, 0x49, 0x8B, 0x8C, 0x24, 0x98, 0x06, 0x00, 0x00 };
		KernelPatcher::LookupPatch linkWidthSiPatch { currentFramebuffer, findLinkWidthSi, replaceLinkWidthSi, sizeof(findLinkWidthSi), 1 };
		patcher.applyLookupPatch(&linkWidthSiPatch);
		DBGLOG("igfx", "applyWestmerePatches applied link width patch for Sierra+");
	}
}

void IGFX::applyWestmereFeaturePatches(IOService *framebuffer) {
	bool success = true;
	
	uint32_t patchFeatureControl = 0;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlFBC;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlGPUInterruptHandling;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlGamma;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlMaximumSelfRefreshLevel;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlPowerStates;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlRSTimerTest;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlRenderStandby;
	patchFeatureControl |= callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlWatermarks;
	
	if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FBCControlCompression) {
		// Entire dictionary will always be replaced as there is only a single property.
		auto dictFBCControl = OSDictionary::withCapacity(1);
		if (dictFBCControl) {
			success &= setDictUInt32(dictFBCControl, "Compression", framebufferWestmerePatches.FBCControlCompression);
			
			// Replace FBCControl dictionary.
			success &= framebuffer->setProperty("FBCControl", dictFBCControl);
			dictFBCControl->release();
		} else {
			success = false;
		}
	}
	
	if (patchFeatureControl) {
		// Try to get existing dictionary, replace if not found.
		auto dictFeatureControlOld = OSDynamicCast(OSDictionary, framebuffer->getProperty("FeatureControl"));
		OSDictionary *dictFeatureControlNew;
		if (dictFeatureControlOld)
			dictFeatureControlNew = OSDictionary::withDictionary(dictFeatureControlOld);
		else
			dictFeatureControlNew = OSDictionary::withCapacity(8);
		
		// Replace any specified properties.
		if (dictFeatureControlNew) {
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlFBC)
				success &= setDictUInt32(dictFeatureControlNew, "FBC", framebufferWestmerePatches.FeatureControlFBC);
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlGPUInterruptHandling)
				success &= setDictUInt32(dictFeatureControlNew, "GPUInterruptHandling", framebufferWestmerePatches.FeatureControlGPUInterruptHandling);
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlGamma)
				success &= setDictUInt32(dictFeatureControlNew, "Gamma", framebufferWestmerePatches.FeatureControlGamma);
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlMaximumSelfRefreshLevel)
				success &= setDictUInt32(dictFeatureControlNew, "MaximumSelfRefreshLevel", framebufferWestmerePatches.FeatureControlMaximumSelfRefreshLevel);
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlPowerStates)
				success &= setDictUInt32(dictFeatureControlNew, "PowerStates", framebufferWestmerePatches.FeatureControlPowerStates);
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlRSTimerTest)
				success &= setDictUInt32(dictFeatureControlNew, "RSTimerTest", framebufferWestmerePatches.FeatureControlRSTimerTest);
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlRenderStandby)
				success &= setDictUInt32(dictFeatureControlNew, "RenderStandby", framebufferWestmerePatches.FeatureControlRenderStandby);
			if (callbackIGFX->framebufferPatchFlags.bitsWestmere.FeatureControlWatermarks)
				success &= setDictUInt32(dictFeatureControlNew, "Watermarks", framebufferWestmerePatches.FeatureControlWatermarks);
			
			// Replace FBCControl dictionary.
			success &= framebuffer->setProperty("FeatureControl", dictFeatureControlNew);
			dictFeatureControlNew->release();
		} else {
			success = false;
		}
	}
	
	if (success)
		DBGLOG("igfx", "applyWestmereFeaturePatches successful %X", callbackIGFX->framebufferPatchFlags.value);
	else
		DBGLOG("igfx", "applyWestmereFeaturePatches failed %X", callbackIGFX->framebufferPatchFlags.value);
}
