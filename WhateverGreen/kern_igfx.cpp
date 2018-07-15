//
//  kern_igfx.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"
#include "kern_fb.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>

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
	framebufferPatch.framebufferId = info->reportedFramebufferId;

	if (info->videoBuiltin) {
		applyFramebufferPatch = loadPatchesFromDevice(info->videoBuiltin, info->reportedFramebufferId);

		bool connectorLessFrame = info->reportedFramebufferIsConnectorLess;

		// Black screen (ComputeLaneCount) happened from 10.12.4
		// It only affects SKL and KBL drivers with a frame with connectors.
		if (!connectorLessFrame && ((getKernelVersion() == KernelVersion::Sierra && getKernelMinorVersion() >= 5) ||
									getKernelVersion() >= KernelVersion::HighSierra) &&
			(cpuGeneration == CPUInfo::CpuGeneration::Skylake || cpuGeneration == CPUInfo::CpuGeneration::KabyLake)) {
			blackScreenPatch = info->firmwareVendor != DeviceInfo::FirmwareVendor::Apple;
		}

		// GuC firmware is just fine on Apple hardware
		if (info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple)
			avoidFirmwareLoading = false;

		// PAVP patch is only necessary when we have no discrete GPU
		pavpDisablePatch = !connectorLessFrame && info->firmwareVendor != DeviceInfo::FirmwareVendor::Apple;

		int gl = info->videoBuiltin->getProperty("disable-metal") != nullptr;
		PE_parse_boot_argn("igfxgl", &gl, sizeof(gl));
		forceOpenGL = gl == 1;

		// Disable kext patching if we have nothing to do.
		switchOffFramebuffer = !blackScreenPatch && !applyFramebufferPatch;
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

		if (applyFramebufferPatch) {
			gPlatformInformationList = patcher.solveSymbol<void *>(index, "_gPlatformInformationList", address, size);
			if (gPlatformInformationList) {
				auto fbGetOSInformation = "__ZN31AppleIntelFramebufferController16getOSInformationEv";
				if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge)
					fbGetOSInformation = "__ZN23AppleIntelSNBGraphicsFB16getOSInformationEv";
				else if (cpuGeneration == CPUInfo::CpuGeneration::IvyBridge)
					fbGetOSInformation = "__ZN25AppleIntelCapriController16getOSInformationEv";
				else if (cpuGeneration == CPUInfo::CpuGeneration::Haswell)
					fbGetOSInformation = "__ZN24AppleIntelAzulController16getOSInformationEv";
				else if (cpuGeneration == CPUInfo::CpuGeneration::Broadwell)
					fbGetOSInformation = "__ZN22AppleIntelFBController16getOSInformationEv";

				KernelPatcher::RouteRequest request(fbGetOSInformation, wrapGetOSInformation, orgGetOSInformation);
				patcher.routeMultiple(index, &request, 1, address, size);
			} else {
				SYSLOG("igfx", "failed to obtain gPlatformInformationList pointer with code %d", patcher.getError());
				patcher.clearError();
			}
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
	// guardded by a HDMI 2.0 enable boot-arg, which resulted in one bug fixing the other, and 10.13.5
	// broke it again.
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

uint64_t IGFX::wrapGetOSInformation(void *that) {
	if (!callbackIGFX->orgGetOSInformation || !callbackIGFX->gPlatformInformationList)
		return 0;

	uint32_t framebufferId = callbackIGFX->framebufferPatch.framebufferId;
	size_t platformInformationCount = 0, platformInformationSize = 0;

	if (callbackIGFX->cpuGeneration == CPUInfo::CpuGeneration::SandyBridge) {
		FramebufferSNB *platformInformationList = static_cast<FramebufferSNB *>(callbackIGFX->gPlatformInformationList);
		uint32_t framebufferPlatformId[] = { 0x00010000, 0x00020000, 0x00030010, 0x00030030, 0x00040000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00030020, 0x00050000 };
		platformInformationCount = sizeof(framebufferPlatformId) / sizeof(uint32_t);
		platformInformationSize = sizeof(FramebufferSNB) * platformInformationCount;
		bool framebufferFound = false;

		for (size_t i = 0; i < platformInformationCount; i++) {
			if (framebufferPlatformId[i] == framebufferId) {
				if (callbackIGFX->framebufferPatchFlags.bits.FPFMobile)
					platformInformationList[i].fMobile = callbackIGFX->framebufferPatch.fMobile;

				if (callbackIGFX->framebufferPatchFlags.bits.FPFPipeCount)
					platformInformationList[i].fPipeCount = callbackIGFX->framebufferPatch.fPipeCount;

				if (callbackIGFX->framebufferPatchFlags.bits.FPFPortCount)
					platformInformationList[i].fPortCount = callbackIGFX->framebufferPatch.fPortCount;

				if (callbackIGFX->framebufferPatchFlags.bits.FPFFBMemoryCount)
					platformInformationList[i].fFBMemoryCount = callbackIGFX->framebufferPatch.fFBMemoryCount;

				if (callbackIGFX->framebufferPatchFlags.bits.FPFBacklightFrequency)
					platformInformationList[i].fBacklightFrequency = callbackIGFX->framebufferPatch.fBacklightFrequency;

				if (callbackIGFX->framebufferPatchFlags.bits.FPFBacklightMax)
					platformInformationList[i].fBacklightMax = callbackIGFX->framebufferPatch.fBacklightMax;

				for (size_t j = 0; j < MaxFramebufferConnectorCount; j++) {
					if (callbackIGFX->connectorPatchFlags[j].bits.CPFIndex)
						platformInformationList[i].connectors[j].index = callbackIGFX->framebufferPatch.connectors[j].index;

					if (callbackIGFX->connectorPatchFlags[j].bits.CPFBusId)
						platformInformationList[i].connectors[j].busId = callbackIGFX->framebufferPatch.connectors[j].busId;

					if (callbackIGFX->connectorPatchFlags[j].bits.CPFPipe)
						platformInformationList[i].connectors[j].pipe = callbackIGFX->framebufferPatch.connectors[j].pipe;

					if (callbackIGFX->connectorPatchFlags[j].bits.CPFType)
						platformInformationList[i].connectors[j].type = callbackIGFX->framebufferPatch.connectors[j].type;

					if (callbackIGFX->connectorPatchFlags[j].bits.CPFFlags)
						platformInformationList[i].connectors[j].flags = callbackIGFX->framebufferPatch.connectors[j].flags;

					if (callbackIGFX->connectorPatchFlags[j].value) {
						DBGLOG("igfx", "Patching framebufferId 0x%08X connector [%d] busId: 0x%02X, pipe: %d, type: 0x%08X, flags: 0x%08X", framebufferId, platformInformationList[i].connectors[j].index, platformInformationList[i].connectors[j].busId, platformInformationList[i].connectors[j].pipe, platformInformationList[i].connectors[j].type, platformInformationList[i].connectors[j].flags.value);

						framebufferFound = true;
					}
				}

				if (callbackIGFX->framebufferPatchFlags.value) {
					DBGLOG("igfx", "Patching framebufferId 0x%08X", framebufferId);
					DBGLOG("igfx", "Mobile: 0x%08X", platformInformationList[i].fMobile);
					DBGLOG("igfx", "PipeCount: %d", platformInformationList[i].fPipeCount);
					DBGLOG("igfx", "PortCount: %d", platformInformationList[i].fPortCount);
					DBGLOG("igfx", "FBMemoryCount: %d", platformInformationList[i].fFBMemoryCount);
					DBGLOG("igfx", "BacklightFrequency: %d", platformInformationList[i].fBacklightFrequency);
					DBGLOG("igfx", "BacklightMax: %d", platformInformationList[i].fBacklightMax);

					framebufferFound = true;
				}
			}
		}

		if (framebufferFound)
			DBGLOG("igfx", "Patching framebufferId 0x%08X successful", framebufferId);
		else
			DBGLOG("igfx", "Patching framebufferId 0x%08X failed", framebufferId);
	} else if (callbackIGFX->cpuGeneration == CPUInfo::CpuGeneration::IvyBridge) {
		FramebufferIVB *platformInformationList = static_cast<FramebufferIVB *>(callbackIGFX->gPlatformInformationList);
		platformInformationCount = PAGE_SIZE / sizeof(FramebufferIVB);
		platformInformationSize = sizeof(FramebufferIVB) * platformInformationCount;

		if (callbackIGFX->applyPlatformInformationListPatch(framebufferId, platformInformationList, platformInformationCount))
			DBGLOG("igfx", "Patching framebufferId 0x%08X successful", framebufferId);
		else
			DBGLOG("igfx", "Patching framebufferId 0x%08X failed", framebufferId);
	} else if (callbackIGFX->cpuGeneration == CPUInfo::CpuGeneration::Haswell) {
		FramebufferHSW *platformInformationList = static_cast<FramebufferHSW *>(callbackIGFX->gPlatformInformationList);
		platformInformationCount = PAGE_SIZE / sizeof(FramebufferHSW);
		platformInformationSize = sizeof(FramebufferHSW) * platformInformationCount;

		if (callbackIGFX->applyPlatformInformationListPatch(framebufferId, platformInformationList, platformInformationCount))
			DBGLOG("igfx", "Patching framebufferId 0x%08X successful", framebufferId);
		else
			DBGLOG("igfx", "Patching framebufferId 0x%08X failed", framebufferId);
	} else if (callbackIGFX->cpuGeneration == CPUInfo::CpuGeneration::Broadwell) {
		FramebufferBDW *platformInformationList = static_cast<FramebufferBDW *>(callbackIGFX->gPlatformInformationList);
		platformInformationCount = PAGE_SIZE / sizeof(FramebufferBDW);
		platformInformationSize = sizeof(FramebufferBDW) * platformInformationCount;

		if (callbackIGFX->applyPlatformInformationListPatch(framebufferId, platformInformationList, platformInformationCount))
			DBGLOG("igfx", "Patching framebufferId 0x%08X successful", framebufferId);
		else
			DBGLOG("igfx", "Patching framebufferId 0x%08X failed", framebufferId);
	} else if (callbackIGFX->cpuGeneration == CPUInfo::CpuGeneration::Skylake || callbackIGFX->cpuGeneration == CPUInfo::CpuGeneration::KabyLake) {
		FramebufferSKL *platformInformationList = static_cast<FramebufferSKL *>(callbackIGFX->gPlatformInformationList);
		platformInformationCount = PAGE_SIZE / sizeof(FramebufferSKL);
		platformInformationSize = sizeof(FramebufferSKL) * platformInformationCount;

		if (callbackIGFX->applyPlatformInformationListPatch(framebufferId, platformInformationList, platformInformationCount))
			DBGLOG("igfx", "Patching framebufferId 0x%08X successful", framebufferId);
		else
			DBGLOG("igfx", "Patching framebufferId 0x%08X failed", framebufferId);
	}

	uint8_t *platformInformationAddress = callbackIGFX->findFramebufferId(framebufferId, static_cast<uint8_t *>(callbackIGFX->gPlatformInformationList), platformInformationSize);

	if (platformInformationAddress) {
		for (size_t i = 0; i < MaxFramebufferPatchCount; i++) {
			if (!callbackIGFX->framebufferPatches[i].find || !callbackIGFX->framebufferPatches[i].replace)
				continue;

			if (callbackIGFX->framebufferPatches[i].framebufferId != framebufferId)    {
				framebufferId = callbackIGFX->framebufferPatches[i].framebufferId;
				platformInformationAddress = callbackIGFX->findFramebufferId(framebufferId, static_cast<uint8_t *>(callbackIGFX->gPlatformInformationList), platformInformationSize);
			}

			if (!platformInformationAddress) {
				DBGLOG("igfx", "Patch %lu framebufferId 0x%08X not found", i, framebufferId);
				continue;
			}

			if (callbackIGFX->framebufferPatches[i].find->getLength() != callbackIGFX->framebufferPatches[i].replace->getLength()) {
				DBGLOG("igfx", "Patch %lu framebufferId 0x%08X length mistmatch", i, framebufferId);
				continue;
			}

			KernelPatcher::LookupPatch patch {};
			patch.kext = callbackIGFX->currentFramebuffer;
			patch.find = static_cast<const uint8_t *>(callbackIGFX->framebufferPatches[i].find->getBytesNoCopy());
			patch.replace = static_cast<const uint8_t *>(callbackIGFX->framebufferPatches[i].replace->getBytesNoCopy());
			patch.size = callbackIGFX->framebufferPatches[i].find->getLength();
			patch.count = callbackIGFX->framebufferPatches[i].count;

			if (callbackIGFX->applyPatch(patch, platformInformationAddress, platformInformationSize))
				DBGLOG("igfx", "Patch %lu framebufferId 0x%08X successful", i, framebufferId);
			else
				DBGLOG("igfx", "Patch %lu framebufferId 0x%08X failed", i, framebufferId);

			callbackIGFX->framebufferPatches[i].find->release();
			callbackIGFX->framebufferPatches[i].find = nullptr;
			callbackIGFX->framebufferPatches[i].replace->release();
			callbackIGFX->framebufferPatches[i].replace = nullptr;
		}
	}

	return FunctionCast(wrapGetOSInformation, callbackIGFX->orgGetOSInformation)(that);
}

bool IGFX::loadPatchesFromDevice(IORegistryEntry *igpu, uint32_t currentFramebufferId) {
	bool hasFramebufferPatch = false;

	uint32_t framebufferPatchEnable = 0;
	if (WIOKit::getOSDataValue(igpu, "framebuffer-patch-enable", framebufferPatchEnable) && framebufferPatchEnable) {
		DBGLOG("igfx", "framebuffer-patch-enable %d", framebufferPatchEnable);

		framebufferPatchFlags.bits.FPFFramebufferId = WIOKit::getOSDataValue(igpu, "framebuffer-framebufferid", framebufferPatch.framebufferId);
		framebufferPatchFlags.bits.FPFMobile = WIOKit::getOSDataValue(igpu, "framebuffer-mobile", framebufferPatch.fMobile);
		framebufferPatchFlags.bits.FPFPipeCount = WIOKit::getOSDataValue(igpu, "framebuffer-pipecount", framebufferPatch.fPipeCount);
		framebufferPatchFlags.bits.FPFPortCount = WIOKit::getOSDataValue(igpu, "framebuffer-portcount", framebufferPatch.fPortCount);
		framebufferPatchFlags.bits.FPFFBMemoryCount = WIOKit::getOSDataValue(igpu, "framebuffer-memorycount", framebufferPatch.fFBMemoryCount);
		framebufferPatchFlags.bits.FPFStolenMemorySize = WIOKit::getOSDataValue(igpu, "framebuffer-stolenmem", framebufferPatch.fStolenMemorySize);
		framebufferPatchFlags.bits.FPFFramebufferMemorySize = WIOKit::getOSDataValue(igpu, "framebuffer-fbmem", framebufferPatch.fFramebufferMemorySize);
		framebufferPatchFlags.bits.FPFUnifiedMemorySize = WIOKit::getOSDataValue(igpu, "framebuffer-unifiedmem", framebufferPatch.fUnifiedMemorySize);
		framebufferPatchFlags.bits.FPFBacklightFrequency = WIOKit::getOSDataValue(igpu, "framebuffer-backlightfreq", framebufferPatch.fBacklightFrequency);
		framebufferPatchFlags.bits.FPFBacklightMax = WIOKit::getOSDataValue(igpu, "framebuffer-backlightmax", framebufferPatch.fBacklightMax);

		if (framebufferPatchFlags.value != 0)
			hasFramebufferPatch = true;

		for (size_t i = 0; i < MaxFramebufferConnectorCount; i++) {
			char name[48];
			snprintf(name, sizeof(name), "framebuffer-con%ld-enable", i);
			uint32_t framebufferConnectorPatchEnable = 0;
			if (!WIOKit::getOSDataValue(igpu, name, framebufferConnectorPatchEnable) || !framebufferConnectorPatchEnable)
				continue;

			DBGLOG("igfx", "framebuffer-con%ld-enable %d", i, framebufferConnectorPatchEnable);

			snprintf(name, sizeof(name), "framebuffer-con%ld-index", i);
			connectorPatchFlags[i].bits.CPFIndex = WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].index);
			snprintf(name, sizeof(name), "framebuffer-con%ld-busid", i);
			connectorPatchFlags[i].bits.CPFBusId = WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].busId);
			snprintf(name, sizeof(name), "framebuffer-con%ld-pipe", i);
			connectorPatchFlags[i].bits.CPFPipe = WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].pipe);
			snprintf(name, sizeof(name), "framebuffer-con%ld-type", i);
			connectorPatchFlags[i].bits.CPFType = WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].type);
			snprintf(name, sizeof(name), "framebuffer-con%ld-flags", i);
			connectorPatchFlags[i].bits.CPFFlags = WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].flags.value);

			if (connectorPatchFlags[i].value != 0)
				hasFramebufferPatch = true;
		}
	}

	size_t patchIndex = 0;
	for (size_t i = 0; i < MaxFramebufferPatchCount; i++) {
		char name[48];
		snprintf(name, sizeof(name), "framebuffer-patch%ld-enable", i);
		// Missing status means no patches at all.
		uint32_t framebufferPatchEnable = 0;
		if (!WIOKit::getOSDataValue(igpu, name, framebufferPatchEnable))
			break;

		// False status means a temporarily disabled patch, skip for next one.
		if (!framebufferPatchEnable)
			continue;

		uint32_t framebufferId = 0;
		size_t framebufferPatchCount = 0;

		snprintf(name, sizeof(name), "framebuffer-patch%ld-framebufferid", i);
		WIOKit::getOSDataValue(igpu, name, framebufferId);
		snprintf(name, sizeof(name), "framebuffer-patch%ld-find", i);
		auto framebufferPatchFind = OSDynamicCast(OSData, igpu->getProperty(name));
		snprintf(name, sizeof(name), "framebuffer-patch%ld-replace", i);
		auto framebufferPatchReplace = OSDynamicCast(OSData, igpu->getProperty(name));
		snprintf(name, sizeof(name), "framebuffer-patch%ld-count", i);
		WIOKit::getOSDataValue(igpu, name, framebufferPatchCount);

		if (!framebufferPatchFind || !framebufferPatchReplace)
			continue;

		framebufferPatches[patchIndex].framebufferId = (framebufferId ? framebufferId : currentFramebufferId);
		framebufferPatches[patchIndex].find = framebufferPatchFind;
		framebufferPatches[patchIndex].replace = framebufferPatchReplace;
		framebufferPatches[patchIndex].count = (framebufferPatchCount ? framebufferPatchCount : 1);

		framebufferPatchFind->retain();
		framebufferPatchReplace->retain();

		hasFramebufferPatch = true;

		patchIndex++;
	}

	return hasFramebufferPatch;
}

uint8_t *IGFX::findFramebufferId(uint32_t framebufferId, uint8_t *startingAddress, size_t maxSize) {
	uint8_t *startAddress = startingAddress;
	uint8_t *endAddress = startingAddress + maxSize - sizeof(uint32_t);
	while (startAddress < endAddress) {
		if (*(reinterpret_cast<uint32_t *>(startAddress)) == framebufferId)
			return startAddress;
		startAddress++;
	}

	return nullptr;
}

bool IGFX::applyPatch(const KernelPatcher::LookupPatch &patch, uint8_t *startingAddress, size_t maxSize) {
	bool r = false;
	size_t i = 0, patchCount = 0;
	uint8_t *startAddress = startingAddress;
	uint8_t *endAddress = startingAddress + maxSize - patch.size;
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
		}

		startAddress++;
	}

	return r;
}

template <typename T>
bool IGFX::applyPlatformInformationListPatch(uint32_t framebufferId, T *platformInformationList, size_t platformInformationCount) {
	bool r = false;
	for (size_t i = 0; i < platformInformationCount; i++) {
		if (platformInformationList[i].framebufferId == 0xFFFFFFFF)
			break;

		if (platformInformationList[i].framebufferId == framebufferId) {
			if (framebufferPatchFlags.bits.FPFMobile)
				platformInformationList[i].fMobile = framebufferPatch.fMobile;

			if (framebufferPatchFlags.bits.FPFPipeCount)
				platformInformationList[i].fPipeCount = framebufferPatch.fPipeCount;

			if (framebufferPatchFlags.bits.FPFPortCount)
				platformInformationList[i].fPortCount = framebufferPatch.fPortCount;

			if (framebufferPatchFlags.bits.FPFFBMemoryCount)
				platformInformationList[i].fFBMemoryCount = framebufferPatch.fFBMemoryCount;

			if (framebufferPatchFlags.bits.FPFStolenMemorySize)
				platformInformationList[i].fStolenMemorySize = framebufferPatch.fStolenMemorySize;

			if (framebufferPatchFlags.bits.FPFFramebufferMemorySize)
				platformInformationList[i].fFramebufferMemorySize = framebufferPatch.fFramebufferMemorySize;

			if (framebufferPatchFlags.bits.FPFUnifiedMemorySize)
				platformInformationList[i].fUnifiedMemorySize = framebufferPatch.fUnifiedMemorySize;

			if (framebufferPatchFlags.bits.FPFBacklightFrequency)
				platformInformationList[i].fBacklightFrequency = framebufferPatch.fBacklightFrequency;

			if (framebufferPatchFlags.bits.FPFBacklightMax)
				platformInformationList[i].fBacklightMax = framebufferPatch.fBacklightMax;

			if (framebufferPatchFlags.value) {
				DBGLOG("igfx", "Patching framebufferId 0x%08X", platformInformationList[i].framebufferId);
				DBGLOG("igfx", "Mobile: 0x%08X", platformInformationList[i].fMobile);
				DBGLOG("igfx", "PipeCount: %d", platformInformationList[i].fPipeCount);
				DBGLOG("igfx", "PortCount: %d", platformInformationList[i].fPortCount);
				DBGLOG("igfx", "FBMemoryCount: %d", platformInformationList[i].fFBMemoryCount);
				DBGLOG("igfx", "StolenMemorySize: 0x%08X", platformInformationList[i].fStolenMemorySize);
				DBGLOG("igfx", "FramebufferMemorySize: 0x%08X", platformInformationList[i].fFramebufferMemorySize);
				DBGLOG("igfx", "UnifiedMemorySize: 0x%08X", platformInformationList[i].fUnifiedMemorySize);
				DBGLOG("igfx", "BacklightFrequency: %d", platformInformationList[i].fBacklightFrequency);
				DBGLOG("igfx", "BacklightMax: %d", platformInformationList[i].fBacklightMax);

				r = true;
			}

			for (size_t j=0; j<MaxFramebufferConnectorCount; j++) {
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
					DBGLOG("igfx", "Patching framebufferId 0x%08X connector [%d] busId: 0x%02X, pipe: %d, type: 0x%08X, flags: 0x%08X", platformInformationList[i].framebufferId, platformInformationList[i].connectors[j].index, platformInformationList[i].connectors[j].busId, platformInformationList[i].connectors[j].pipe, platformInformationList[i].connectors[j].type, platformInformationList[i].connectors[j].flags.value);

					r = true;
				}
			}

			break;
		}
	}

	return r;
}
