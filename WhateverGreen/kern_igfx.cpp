//
//  kern_igfx.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"
#include "kern_fb.hpp"
#include "kern_guc.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_file.hpp>
#include <Headers/kern_iokit.hpp>

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

	int canLoadGuC = 0;
	if (getKernelVersion() >= KernelVersion::HighSierra)
		PE_parse_boot_argn("igfxfw", &canLoadGuC, sizeof(canLoadGuC));

	uint32_t family = 0, model = 0;
	cpuGeneration = CPUInfo::getGeneration(&family, &model);
	switch (cpuGeneration) {
		case CPUInfo::CpuGeneration::Penryn:
		case CPUInfo::CpuGeneration::Nehalem:
		case CPUInfo::CpuGeneration::Westmere:
			// Do not warn about legacy processors (e.g. Xeon).
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
			avoidFirmwareLoading = getKernelVersion() >= KernelVersion::HighSierra;
			loadGuCFirmware = canLoadGuC > 0;
			currentGraphics = &kextIntelSKL;
			currentFramebuffer = &kextIntelSKLFb;
			break;
		case CPUInfo::CpuGeneration::KabyLake:
			avoidFirmwareLoading = getKernelVersion() >= KernelVersion::HighSierra;
			loadGuCFirmware = canLoadGuC > 0;
			currentGraphics = &kextIntelKBL;
			currentFramebuffer = &kextIntelKBLFb;
			break;
		case CPUInfo::CpuGeneration::CoffeeLake:
			avoidFirmwareLoading = getKernelVersion() >= KernelVersion::HighSierra;
			loadGuCFirmware = canLoadGuC > 0;
			currentGraphics = &kextIntelKBL;
			currentFramebuffer = &kextIntelCFLFb;
			// Allow faking ask KBL
			currentFramebufferOpt = &kextIntelKBLFb;
			// Note, several CFL GPUs are completely broken. They freeze in IGMemoryManager::initCache due to incompatible
			// configuration, supposedly due to Apple not supporting new MOCS table and forcing Skylake-based format.
			// See: https://github.com/torvalds/linux/blob/135c5504a600ff9b06e321694fbcac78a9530cd4/drivers/gpu/drm/i915/intel_mocs.c#L181
			break;
		case CPUInfo::CpuGeneration::CannonLake:
			avoidFirmwareLoading = getKernelVersion() >= KernelVersion::HighSierra;
			loadGuCFirmware = canLoadGuC > 0;
			currentGraphics = &kextIntelCNL;
			currentFramebuffer = &kextIntelCNLFb;
			break;
		case CPUInfo::CpuGeneration::IceLake:
			avoidFirmwareLoading = getKernelVersion() >= KernelVersion::HighSierra;
			loadGuCFirmware = canLoadGuC > 0;
			currentGraphics = &kextIntelICL;
			currentFramebuffer = &kextIntelICLLPFb;
			currentFramebufferOpt = &kextIntelICLHPFb;
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

}

void IGFX::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	bool switchOffGraphics = false;
	bool switchOffFramebuffer = false;
	framebufferPatch.framebufferId = info->reportedFramebufferId;

	if (info->videoBuiltin) {
		applyFramebufferPatch = loadPatchesFromDevice(info->videoBuiltin, info->reportedFramebufferId);

#ifdef DEBUG
		if (checkKernelArgument("-igfxdump"))
			dumpFramebufferToDisk = true;

		if (checkKernelArgument("-igfxfbdump"))
			dumpPlatformTable = true;
#endif

		// Enable CFL backlight patch on mobile CFL or if IGPU propery enable-cfl-backlight-fix is set
		int bkl = 0;
		if (PE_parse_boot_argn("igfxcflbklt", &bkl, sizeof(bkl)))
			cflBacklightPatch = bkl == 1 ? CoffeeBacklightPatch::On : CoffeeBacklightPatch::Off;
		else if (info->videoBuiltin->getProperty("enable-cfl-backlight-fix"))
			cflBacklightPatch = CoffeeBacklightPatch::On;
		else if (currentFramebuffer == &kextIntelCFLFb && WIOKit::getComputerModel() == WIOKit::ComputerModel::ComputerLaptop)
			cflBacklightPatch = CoffeeBacklightPatch::Auto;

		if (WIOKit::getOSDataValue(info->videoBuiltin, "max-backlight-freq", targetBacklightFrequency))
			DBGLOG("igfx", "read custom backlight frequency %u", targetBacklightFrequency);

		bool connectorLessFrame = info->reportedFramebufferIsConnectorLess;

		// Black screen (ComputeLaneCount) happened from 10.12.4
		// It only affects SKL, KBL, and CFL drivers with a frame with connectors.
		if (!connectorLessFrame && cpuGeneration >= CPUInfo::CpuGeneration::Skylake &&
			((getKernelVersion() == KernelVersion::Sierra && getKernelMinorVersion() >= 5) || getKernelVersion() >= KernelVersion::HighSierra)) {
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

		// Starting from 10.14.4b1 KabyLake graphics randomly kernel panics on GPU usage
		readDescriptorPatch = cpuGeneration == CPUInfo::CpuGeneration::KabyLake && getKernelVersion() >= KernelVersion::Mojave;

		// Automatically enable HDMI -> DP patches
		hdmiAutopatch = !applyFramebufferPatch && !connectorLessFrame && getKernelVersion() >= Yosemite && !checkKernelArgument("-igfxnohdmi");

		// Disable kext patching if we have nothing to do.
		switchOffFramebuffer = !blackScreenPatch && !applyFramebufferPatch && !dumpFramebufferToDisk && !dumpPlatformTable && !hdmiAutopatch && cflBacklightPatch == CoffeeBacklightPatch::Off;
		switchOffGraphics = !pavpDisablePatch && !forceOpenGL && !moderniseAccelerator && !avoidFirmwareLoading && !readDescriptorPatch;
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

		if (forceOpenGL || moderniseAccelerator || avoidFirmwareLoading) {
			auto startSym = "__ZN16IntelAccelerator5startEP9IOService";
			if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge)
				startSym = "__ZN16IntelAccelerator5startEP9IOService";

			KernelPatcher::RouteRequest request(startSym, wrapAcceleratorStart, orgAcceleratorStart);
			patcher.routeMultiple(index, &request, 1, address, size);

			if (loadGuCFirmware)
				loadIGScheduler4Patches(patcher, index, address, size);
		}

		if (readDescriptorPatch) {
			KernelPatcher::RouteRequest request("__ZNK25IGHardwareGlobalPageTable4readEyRyS0_", globalPageTableRead);
			patcher.routeMultiple(index, &request, 1, address, size);
		}

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

			auto regRead = patcher.solveSymbol<decltype(orgCflReadRegister32)>
				(index, "__ZN31AppleIntelFramebufferController14ReadRegister32Em", address, size);
			auto regWrite = patcher.solveSymbol(index, "__ZN31AppleIntelFramebufferController15WriteRegister32Emj", address, size);
			if (regWrite) {
				(bklCoffeeFb ? orgCflReadRegister32 : orgKblReadRegister32) = regRead;

				patcher.eraseCoverageInstPrefix(regWrite);
				auto orgRegWrite = reinterpret_cast<decltype(orgCflWriteRegister32)>
					(patcher.routeFunction(regWrite, reinterpret_cast<mach_vm_address_t>(bklCoffeeFb ? wrapCflWriteRegister32 : wrapKblWriteRegister32), true));

				if (orgRegWrite) {
					(bklCoffeeFb ? orgCflWriteRegister32 : orgKblWriteRegister32) = orgRegWrite;
				} else {
					SYSLOG("igfx", "failed to route WriteRegister32 for cfl %d", bklCoffeeFb);
					patcher.clearError();
				}
			} else {
				SYSLOG("igfx", "failed to find ReadRegister32 for cfl %d", bklCoffeeFb);
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
			if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge) {
				gPlatformListIsSNB = true;
				gPlatformInformationList = patcher.solveSymbol<void *>(index, "_PlatformInformationList", address, size);
			} else {
				gPlatformListIsSNB = false;
				gPlatformInformationList = patcher.solveSymbol<void *>(index, "_gPlatformInformationList", address, size);
			}

			DBGLOG("igfx", "platform is snb %d and list " PRIKADDR, gPlatformListIsSNB, CASTKADDR(gPlatformInformationList));

			if (gPlatformInformationList) {
				framebufferStart = reinterpret_cast<uint8_t *>(address);
				framebufferSize = size;

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
	// By default Apple drivers load Apple-specific firmware, which is incompatible.
	// On KBL they do it unconditionally, which causes infinite loop.
	// On 10.13 there is an option to ignore/load a generic firmware, which we set here.
	// On 10.12 it is not necessary.
	if (callbackIGFX->avoidFirmwareLoading) {
		auto dev = OSDynamicCast(OSDictionary, that->getProperty("Development"));
		if (dev && dev->getObject("GraphicsSchedulerSelect")) {
			auto newDev = OSDynamicCast(OSDictionary, dev->copyCollection());
			if (newDev) {
				// 1 - Automatic scheduler (Apple -> fallback to disabled)
				// 2 - Force disable via plist
				// 3 - Apple Scheduler
				// 4 - Reference Scheduler
				auto num = OSNumber::withNumber(callbackIGFX->loadGuCFirmware ? 4 : 2, 32);
				if (num) {
					newDev->setObject("GraphicsSchedulerSelect", num);
					num->release();
					that->setProperty("Development", newDev);
					newDev->release();
				}
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
			uint32_t rescaledValue = static_cast<uint32_t>((value * static_cast<uint64_t>(callbackIGFX->targetBacklightFrequency)) /
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

		uint32_t rescaledValue = frequency == 0 ? 0 : static_cast<uint32_t>((dutyCycle * static_cast<uint64_t>(callbackIGFX->targetBacklightFrequency)) /
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

bool IGFX::wrapGetOSInformation(void *that) {
#ifdef DEBUG
	if (callbackIGFX->dumpFramebufferToDisk) {
		char name[64];
		snprintf(name, sizeof(name), "/AppleIntelFramebuffer_%d_%d.%d", callbackIGFX->cpuGeneration, getKernelVersion(), getKernelMinorVersion());
		FileIO::writeBufferToFile(name, callbackIGFX->framebufferStart, callbackIGFX->framebufferSize);
		SYSLOG("igfx", "dumping framebuffer information to %s", name);
	}
#endif

#ifdef DEBUG
	if (callbackIGFX->dumpPlatformTable)
		callbackIGFX->writePlatformListData("platform-table-native");
#endif

	if (callbackIGFX->applyFramebufferPatch)
		callbackIGFX->applyFramebufferPatches();
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
		   callbackIGFX->loadGuCFirmware, callbackIGFX->cpuGeneration);

	if (callbackIGFX->firmwareSizePointer)
		callbackIGFX->performingFirmwareLoad = true;

	r = FunctionCast(wrapLoadGuCBinary, callbackIGFX->orgLoadGuCBinary)(that, flag);
	DBGLOG("igfx", "loadGuCBinary returned %d", r);

	callbackIGFX->performingFirmwareLoad = false;

	return r;
}

bool IGFX::wrapLoadFirmware(IOService *that) {
	DBGLOG("igfx", "load firmware setting sleep overrides %d", callbackIGFX->cpuGeneration);

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
	if (callbackIGFX->loadGuCFirmware) {
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
	void *r = nullptr;

	if (callbackIGFX->performingFirmwareLoad) {
		// Allocate a dummy buffer
		callbackIGFX->dummyFirmwareBuffer = Buffer::create<uint8_t>(size);
		// Select the latest firmware to upload
		DBGLOG("igfx", "preparing firmware for cpu gen %d with range 0x%lX", callbackIGFX->cpuGeneration, size);

		const void *fw = nullptr;
		const void *fwsig = nullptr;
		size_t fwsize = 0;
		size_t fwsigsize = 0;

		if (callbackIGFX->cpuGeneration == CPUInfo::CpuGeneration::Skylake) {
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

	uint32_t framebufferPatchEnable = 0;
	if (WIOKit::getOSDataValue(igpu, "framebuffer-patch-enable", framebufferPatchEnable) && framebufferPatchEnable) {
		DBGLOG("igfx", "framebuffer-patch-enable %d", framebufferPatchEnable);

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
				auto replaceCount = allDataSize / sizeof(framebufferPatch.connectors[0]);
				if (0 == allDataSize % sizeof(framebufferPatch.connectors[0]) && i + replaceCount <= arrsize(framebufferPatch.connectors)) {
					auto replacementConnectors = reinterpret_cast<const ConnectorInfo*>(allData->getBytesNoCopy());
					for (size_t j = 0; j < replaceCount; j++) {
						framebufferPatch.connectors[i+j] = replacementConnectors[j];
						connectorPatchFlags[i+j].bits.CPFIndex = true;
						connectorPatchFlags[i+j].bits.CPFBusId = true;
						connectorPatchFlags[i+j].bits.CPFPipe = true;
						connectorPatchFlags[i+j].bits.CPFType = true;
						connectorPatchFlags[i+j].bits.CPFFlags = true;
					}
				}
			}

			snprintf(name, sizeof(name), "framebuffer-con%lu-index", i);
			connectorPatchFlags[i].bits.CPFIndex |= WIOKit::getOSDataValue<uint32_t>(igpu, name, framebufferPatch.connectors[i].index);
			snprintf(name, sizeof(name), "framebuffer-con%lu-busid", i);
			connectorPatchFlags[i].bits.CPFBusId |= WIOKit::getOSDataValue<uint32_t>(igpu, name, framebufferPatch.connectors[i].busId);
			snprintf(name, sizeof(name), "framebuffer-con%lu-pipe", i);
			connectorPatchFlags[i].bits.CPFPipe |= WIOKit::getOSDataValue<uint32_t>(igpu, name, framebufferPatch.connectors[i].pipe);
			snprintf(name, sizeof(name), "framebuffer-con%lu-type", i);
			connectorPatchFlags[i].bits.CPFType |= WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].type);
			snprintf(name, sizeof(name), "framebuffer-con%lu-flags", i);
			connectorPatchFlags[i].bits.CPFFlags |= WIOKit::getOSDataValue(igpu, name, framebufferPatch.connectors[i].flags.value);

			if (connectorPatchFlags[i].value != 0)
				hasFramebufferPatch = true;
		}
	}

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
		WIOKit::getOSDataValue(igpu, name, framebufferPatchCount);

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
		else if (cpuGeneration == CPUInfo::CpuGeneration::CannonLake)
			success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferCNL *>(gPlatformInformationList));
		else if (cpuGeneration == CPUInfo::CpuGeneration::IceLake) {
			// FIXME: Need to address possible circumstance of both ICL kexts loaded at the same time
			if (callbackIGFX->currentFramebuffer->loadIndex == KernelPatcher::KextInfo::SysFlags::Loaded)
				success = applyPlatformInformationListPatch(framebufferId, static_cast<FramebufferICLLP *>(gPlatformInformationList));
			else if (callbackIGFX->currentFramebufferOpt->loadIndex == KernelPatcher::KextInfo::SysFlags::Loaded)
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
	else if (cpuGeneration == CPUInfo::CpuGeneration::CannonLake)
		success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferCNL *>(gPlatformInformationList));
	else if (cpuGeneration == CPUInfo::CpuGeneration::IceLake) {
		// FIXME: Need to address possible circumstance of both ICL kexts loaded at the same time
		if (callbackIGFX->currentFramebuffer->loadIndex == KernelPatcher::KextInfo::SysFlags::Loaded)
			success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferICLLP *>(gPlatformInformationList));
		else if (callbackIGFX->currentFramebufferOpt->loadIndex == KernelPatcher::KextInfo::SysFlags::Loaded)
			success = applyDPtoHDMIPatch(framebufferId, static_cast<FramebufferICLHP *>(gPlatformInformationList));
	}
	
	if (success)
		DBGLOG("igfx", "hdmi patching framebufferId 0x%08X successful", framebufferId);
	else
		DBGLOG("igfx", "hdmi patching framebufferId 0x%08X failed", framebufferId);
}
