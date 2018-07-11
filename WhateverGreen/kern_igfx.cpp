//
//  kern_igfx.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>

static const char *pathIntelHD3000[] { "/System/Library/Extensions/AppleIntelHD3000Graphics.kext/Contents/MacOS/AppleIntelHD3000Graphics" };
static const char *pathIntelHD4000[] { "/System/Library/Extensions/AppleIntelHD4000Graphics.kext/Contents/MacOS/AppleIntelHD4000Graphics" };
static const char *pathIntelHD5000[] { "/System/Library/Extensions/AppleIntelHD5000Graphics.kext/Contents/MacOS/AppleIntelHD5000Graphics" };
static const char *pathIntelBDW[]    { "/System/Library/Extensions/AppleIntelBDWGraphics.kext/Contents/MacOS/AppleIntelBDWGraphics" };
static const char *pathIntelSKL[]    { "/System/Library/Extensions/AppleIntelSKLGraphics.kext/Contents/MacOS/AppleIntelSKLGraphics" };
static const char *pathIntelSKLFb[]  { "/System/Library/Extensions/AppleIntelSKLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelSKLGraphicsFramebuffer" };
static const char *pathIntelKBL[]    { "/System/Library/Extensions/AppleIntelKBLGraphics.kext/Contents/MacOS/AppleIntelKBLGraphics" };
static const char *pathIntelKBLFb[]  { "/System/Library/Extensions/AppleIntelKBLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelKBLGraphicsFramebuffer" };

static KernelPatcher::KextInfo kextIntelHD3000 { "com.apple.driver.AppleIntelHD3000Graphics", pathIntelHD3000, arrsize(pathIntelHD3000), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelHD4000 { "com.apple.driver.AppleIntelHD4000Graphics", pathIntelHD4000, arrsize(pathIntelHD4000), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelHD5000 { "com.apple.driver.AppleIntelHD5000Graphics", pathIntelHD5000, arrsize(pathIntelHD5000), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelBDW    { "com.apple.driver.AppleIntelBDWGraphics", pathIntelBDW, arrsize(pathIntelBDW), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelSKL    { "com.apple.driver.AppleIntelSKLGraphics", pathIntelSKL, arrsize(pathIntelSKL), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelSKLFb  { "com.apple.driver.AppleIntelSKLGraphicsFramebuffer", pathIntelSKLFb, arrsize(pathIntelSKLFb), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelKBL    { "com.apple.driver.AppleIntelKBLGraphics", pathIntelKBL, arrsize(pathIntelKBL), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextIntelKBLFb  { "com.apple.driver.AppleIntelKBLGraphicsFramebuffer", pathIntelKBLFb, arrsize(pathIntelKBLFb), {}, {}, KernelPatcher::KextInfo::Unloaded };

IGFX *IGFX::callbackIGFX;

void IGFX::init() {
	callbackIGFX = this;

	forceVesaMode = checkKernelArgument("-igfxvesa");

	uint32_t family = 0, model = 0;
	cpuGeneration = CPUInfo::getGeneration(&family, &model);
	switch (cpuGeneration) {
		case CPUInfo::CpuGeneration::SandyBridge: {
			int tmp = 1;
			PE_parse_boot_argn("igfxsnb", &tmp, sizeof(tmp));
			moderniseAccelerator = tmp == 1;
			currentGraphics = &kextIntelHD3000;
			break;
		}
		case CPUInfo::CpuGeneration::IvyBridge:
			currentGraphics = &kextIntelHD4000;
			break;
		case CPUInfo::CpuGeneration::Haswell:
			currentGraphics = &kextIntelHD5000;
			break;
		case CPUInfo::CpuGeneration::Broadwell:
			currentGraphics = &kextIntelBDW;
			break;
		case CPUInfo::CpuGeneration::Skylake:
			avoidFirmwareLoading = getKernelVersion() >= KernelVersion::HighSierra;
			currentGraphics = &kextIntelSKL;
			currentFramebuffer = &kextIntelSKLFb;
			break;
		case CPUInfo::CpuGeneration::KabyLake:
			avoidFirmwareLoading = getKernelVersion() >= KernelVersion::HighSierra;
			currentGraphics = &kextIntelKBL;
			currentFramebuffer = &kextIntelKBLFb;
			break;
		default:
			SYSLOG("igfx", "found an unsupported processor 0x%X:0x%X, please report this!", family, model);
			break;
	}

	if (currentGraphics)
		lilu.onKextLoadForce(currentGraphics);

	if (currentFramebuffer)
		lilu.onKextLoadForce(currentFramebuffer);
}

void IGFX::deinit() {

}

void IGFX::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	bool switchOffGraphics = false;
	bool switchOffFramebuffer = false;

	if (info->videoBuiltin) {
		bool connectorLessFrame = info->reportedFramebufferIsConnectorLess;

		// Black screen (ComputeLaneCount) happened from 10.12.4 till 10.13.4 inclusive
		// It only affects SKL and KBL drivers with a frame with connectors.
		if (!connectorLessFrame && ((getKernelVersion() == KernelVersion::Sierra && getKernelMinorVersion() >= 5) ||
									(getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() < 5)) &&
			(cpuGeneration == CPUInfo::CpuGeneration::Skylake || cpuGeneration == CPUInfo::CpuGeneration::KabyLake)) {
			blackScreenPatch = true;
		}

		// PAVP patch is only necessary when we have no discrete GPU
		pavpDisablePatch = !connectorLessFrame;

		int gl = info->videoBuiltin->getProperty("disable-metal") != nullptr;
		PE_parse_boot_argn("igfxgl", &gl, sizeof(gl));
		forceOpenGL = gl == 1;

		// Disable kext patching if we have nothing to do.
		switchOffFramebuffer = !blackScreenPatch;
		switchOffGraphics = !pavpDisablePatch && !forceOpenGL && !forceVesaMode && !moderniseAccelerator && !avoidFirmwareLoading;
	} else {
		switchOffGraphics = switchOffFramebuffer = true;
	}

	if (switchOffGraphics && currentGraphics)
		currentGraphics->switchOff();

	if (switchOffFramebuffer && currentFramebuffer)
		currentFramebuffer->switchOff();

	if (moderniseAccelerator) {
		KernelPatcher::RouteRequest request("__ZN9IOService20copyExistingServicesEP12OSDictionaryjj", wrapCopyExistingServices, orgCopyExistingServices);
		patcher.routeMultiple(KernelPatcher::KernelID, &request, 1);
	}
}

bool IGFX::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
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

		if (forceOpenGL || forceVesaMode || moderniseAccelerator || avoidFirmwareLoading) {
			auto startSym = "__ZN16IntelAccelerator5startEP9IOService";
			if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge)
				startSym = "__ZN16IntelAccelerator5startEP9IOService";

			KernelPatcher::RouteRequest request(startSym, wrapAcceleratorStart, orgAcceleratorStart);
			patcher.routeMultiple(index, &request, 1, address, size);
		}

		return true;
	}

	if (currentFramebuffer && currentFramebuffer->loadIndex == index) {
		if (blackScreenPatch) {
			KernelPatcher::RouteRequest request("__ZN31AppleIntelFramebufferController16ComputeLaneCountEPK29IODetailedTimingInformationV2jjPj", wrapComputeLaneCount, orgComputeLaneCount);
			patcher.routeMultiple(index, &request, 1, address, size);
		}

		return true;
	}


	return false;
}

IOReturn IGFX::wrapPavpSessionCallback(void *intelAccelerator, int32_t sessionCommand, uint32_t sessionAppId, uint32_t *a4, bool flag) {
	//DBGLOG("igfx, "pavpCallback: cmd = %d, flag = %d, app = %d, a4 = %s", sessionCommand, flag, sessionAppId, a4 == nullptr ? "null" : "not null");

	if (sessionCommand == 4) {
		DBGLOG("igfx", "pavpSessionCallback: enforcing error on cmd 4 (send to ring?)!");
		return kIOReturnTimeout; // or kIOReturnSuccess
	}

	return FunctionCast(wrapPavpSessionCallback, callbackIGFX->orgPavpSessionCallback)(intelAccelerator, sessionCommand, sessionAppId, a4, flag);
}

bool IGFX::wrapComputeLaneCount(void *that, void *timing, uint32_t bpp, int32_t availableLanes, int32_t *laneCount) {
	DBGLOG("igfx", "computeLaneCount: bpp = %d, available = %d", bpp, availableLanes);

	// It seems that AGDP fails to properly detect external boot monitors. As a result computeLaneCount
	// is mistakengly called for any boot monitor (e.g. HDMI/DVI), while it is only meant to be used for
	// DP (eDP) displays. More details could be found at:
	// https://github.com/vit9696/Lilu/issues/27#issuecomment-372103559
	// Since the only problematic function is AppleIntelFramebuffer::validateDetailedTiming, there are
	// multiple ways to workaround it.
	// 1. In 10.13.4 Apple added an additional extended timing validation call, which happened to be
	// guardded by a HDMI 2.0 enable boot-arg, which resulted in one bug fixing the other.
	// 2. Another good way is to intercept AppleIntelFramebufferController::RegisterAGDCCallback and
	// make sure AppleGraphicsDevicePolicy::_VendorEventHandler returns mode 2 (not 0) for event 10.
	// 3. Disabling AGDC by nopping AppleIntelFramebufferController::RegisterAGDCCallback is also fine.
	// Simply returning true from computeLaneCount and letting 0 to be compared against zero so far was
	// least destructive and most reliable. Let's stick with it until we could solve more problems.
	bool r = FunctionCast(wrapComputeLaneCount, callbackIGFX->orgComputeLaneCount)(that, timing, bpp, availableLanes, laneCount);
	if (!r && *laneCount == 0) {
		DBGLOG("igfx", "reporting worked lane count");
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
				matching->setObject(gIONameMatchKey, OSString::withCString("IntelAccelerator"));
			}
		}
	}

	return FunctionCast(wrapCopyExistingServices, callbackIGFX->orgCopyExistingServices)(matching, inState, options);
}

bool IGFX::wrapAcceleratorStart(IOService *that, IOService *provider) {
	if (callbackIGFX->forceVesaMode) {
		DBGLOG("igfx", "prevent starting controller");
		return false;
	}

	// By default Apple drivers load Apple-specific firmware, which is incompatible.
	// On KBL they do it unconditionally, which causes infinite loop.
	// On 10.13 there is an option to ignore/load a generic firmware, which we set here.
	// On 10.12 it is not necessary.
	if (callbackIGFX->avoidFirmwareLoading) {
		auto dev = OSDynamicCast(OSDictionary, that->getProperty("Development"));
		if (dev && dev->getObject("GraphicsSchedulerSelect")) {
			auto newDev = OSDynamicCast(OSDictionary, dev->copyCollection());
			if (newDev) {
				// force disable via plist
				newDev->setObject("GraphicsSchedulerSelect", OSNumber::withNumber(2, 32));
				that->setProperty("Development", newDev);
			}
		}
	}

	if (callbackIGFX->forceOpenGL) {
		DBGLOG("igfx", "disabling metal support");
		that->removeProperty("MetalPluginClassName");
		that->removeProperty("MetalPluginName");
		that->removeProperty("MetalStatisticsName");
	}

	if (callbackIGFX->moderniseAccelerator)
		that->setName("IntelAccelerator");

	return FunctionCast(wrapAcceleratorStart, callbackIGFX->orgAcceleratorStart)(that, provider);
}
