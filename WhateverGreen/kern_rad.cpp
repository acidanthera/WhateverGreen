//
//  kern_rad.cpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IOService.h>

#include <Availability.h>
#include <IOKit/IOPlatformExpert.h>

#include "kern_rad.hpp"

static const char *pathFramebuffer[]		{ "/System/Library/Extensions/AMDFramebuffer.kext/Contents/MacOS/AMDFramebuffer" };
static const char *pathRedeonX6000Framebuffer[]	{ "/System/Library/Extensions/AMDRadeonX6000Framebuffer.kext/Contents/MacOS/AMDRadeonX6000Framebuffer" };
static const char *pathLegacyFramebuffer[]	{ "/System/Library/Extensions/AMDLegacyFramebuffer.kext/Contents/MacOS/AMDLegacyFramebuffer" };
static const char *pathSupport[]			{ "/System/Library/Extensions/AMDSupport.kext/Contents/MacOS/AMDSupport" };
static const char *pathLegacySupport[]		{ "/System/Library/Extensions/AMDLegacySupport.kext/Contents/MacOS/AMDLegacySupport" };
static const char *pathRadeonX3000[]        { "/System/Library/Extensions/AMDRadeonX3000.kext/Contents/MacOS/AMDRadeonX3000" };
static const char *pathRadeonX4000[]        { "/System/Library/Extensions/AMDRadeonX4000.kext/Contents/MacOS/AMDRadeonX4000" };
static const char *pathRadeonX4100[]        { "/System/Library/Extensions/AMDRadeonX4100.kext/Contents/MacOS/AMDRadeonX4100" };
static const char *pathRadeonX4150[]        { "/System/Library/Extensions/AMDRadeonX4150.kext/Contents/MacOS/AMDRadeonX4150" };
static const char *pathRadeonX4200[]        { "/System/Library/Extensions/AMDRadeonX4200.kext/Contents/MacOS/AMDRadeonX4200" };
static const char *pathRadeonX4250[]        { "/System/Library/Extensions/AMDRadeonX4250.kext/Contents/MacOS/AMDRadeonX4250" };
static const char *pathRadeonX5000[]        { "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000" };
static const char *pathRadeonX6000[]        { "/System/Library/Extensions/AMDRadeonX6000.kext/Contents/MacOS/AMDRadeonX6000" };
static const char *patchPolarisController[] { "/System/Library/Extensions/AMD9500Controller.kext/Contents/MacOS/AMD9500Controller" };

static const char *idRadeonX3000New {"com.apple.kext.AMDRadeonX3000"};
static const char *idRadeonX4000New {"com.apple.kext.AMDRadeonX4000"};
static const char *idRadeonX4100New {"com.apple.kext.AMDRadeonX4100"};
static const char *idRadeonX4150New {"com.apple.kext.AMDRadeonX4150"};
static const char *idRadeonX4200New {"com.apple.kext.AMDRadeonX4200"};
static const char *idRadeonX4250New {"com.apple.kext.AMDRadeonX4250"};
static const char *idRadeonX5000New {"com.apple.kext.AMDRadeonX5000"};
static const char *idRadeonX6000New {"com.apple.kext.AMDRadeonX6000"};
static const char *idRadeonX3000Old {"com.apple.AMDRadeonX3000"};
static const char *idRadeonX4000Old {"com.apple.AMDRadeonX4000"};

static KernelPatcher::KextInfo kextRadeonFramebuffer
{ "com.apple.kext.AMDFramebuffer", pathFramebuffer, arrsize(pathFramebuffer), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextRadeonLegacyFramebuffer
{ "com.apple.kext.AMDLegacyFramebuffer", pathLegacyFramebuffer, arrsize(pathLegacyFramebuffer), {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextRadeonSupport
{ "com.apple.kext.AMDSupport", pathSupport, 1, {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextRadeonLegacySupport
{ "com.apple.kext.AMDLegacySupport", pathLegacySupport, 1, {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextPolarisController
{ "com.apple.kext.AMD9500Controller", patchPolarisController, 1, {}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextRadeonX6000Framebuffer
{ "com.apple.kext.AMDRadeonX6000Framebuffer", pathRedeonX6000Framebuffer, arrsize(pathRedeonX6000Framebuffer), {}, {}, KernelPatcher::KextInfo::Unloaded };

static KernelPatcher::KextInfo kextRadeonHardware[RAD::MaxRadeonHardware] {
	[RAD::IndexRadeonHardwareX3000] = { idRadeonX3000New, pathRadeonX3000, arrsize(pathRadeonX3000), {}, {}, KernelPatcher::KextInfo::Unloaded },
	[RAD::IndexRadeonHardwareX4100] = { idRadeonX4100New, pathRadeonX4100, arrsize(pathRadeonX4100), {}, {}, KernelPatcher::KextInfo::Unloaded },
	[RAD::IndexRadeonHardwareX4150] = { idRadeonX4150New, pathRadeonX4150, arrsize(pathRadeonX4150), {}, {}, KernelPatcher::KextInfo::Unloaded },
	[RAD::IndexRadeonHardwareX4200] = { idRadeonX4200New, pathRadeonX4200, arrsize(pathRadeonX4200), {}, {}, KernelPatcher::KextInfo::Unloaded },
	[RAD::IndexRadeonHardwareX4250] = { idRadeonX4250New, pathRadeonX4250, arrsize(pathRadeonX4250), {}, {}, KernelPatcher::KextInfo::Unloaded },
	[RAD::IndexRadeonHardwareX4000] = { idRadeonX4000New, pathRadeonX4000, arrsize(pathRadeonX4000), {}, {}, KernelPatcher::KextInfo::Unloaded },
	[RAD::IndexRadeonHardwareX5000] = { idRadeonX5000New, pathRadeonX5000, arrsize(pathRadeonX5000), {}, {}, KernelPatcher::KextInfo::Unloaded },
	[RAD::IndexRadeonHardwareX6000] = { idRadeonX6000New, pathRadeonX6000, arrsize(pathRadeonX6000), {}, {}, KernelPatcher::KextInfo::Unloaded }
};

/**
 *  Power-gating flags
 *  Each symbol corresponds to a bit provided in a radpg argument mask
 */
static const char *powerGatingFlags[] {
	"CAIL_DisableDrmdmaPowerGating",
	"CAIL_DisableGfxCGPowerGating",
	"CAIL_DisableUVDPowerGating",
	"CAIL_DisableVCEPowerGating",
	"CAIL_DisableDynamicGfxMGPowerGating",
	"CAIL_DisableGmcPowerGating",
	"CAIL_DisableAcpPowerGating",
	"CAIL_DisableSAMUPowerGating"
};

RAD *RAD::callbackRAD;

void RAD::init(bool enableNavi10Bkl) {
	callbackRAD = this;

	currentPropProvider.init();
	currentLegacyPropProvider.init();

	force24BppMode = checkKernelArgument("-rad24");
	useCustomAgdpDecision = getKernelVersion() >= KernelVersion::Catalina;

	// Certain displays do not support 32-bit colour output, so we have to force 24-bit.
	if (getKernelVersion() >= KernelVersion::Sierra && force24BppMode) {
		lilu.onKextLoadForce(&kextRadeonFramebuffer);
		// Mojave dropped legacy GPU support (5xxx and 6xxx).
		if (getKernelVersion() < KernelVersion::Mojave)
			lilu.onKextLoadForce(&kextRadeonLegacyFramebuffer);
	}
	
	if (enableNavi10Bkl) {
		lilu.onKextLoadForce(&kextRadeonX6000Framebuffer);
	}

	// Certain GPUs cannot output to DVI at full resolution.
	dviSingleLink = checkKernelArgument("-raddvi");

	// Disabling Metal may be useful for testing
	forceOpenGL = checkKernelArgument("-radgl");

	// Fix accelerator name if requested
	fixConfigName = checkKernelArgument("-radcfg");

	// Broken drivers can still let us boot in vesa mode
	forceVesaMode = checkKernelArgument("-radvesa");
	
	// Fix codec PID to be spoofed PID if requested
	forceCodecInfo = checkKernelArgument("-radcodec");

	// To support overriding connectors and -radvesa mode we need to patch AMDSupport.
	lilu.onKextLoadForce(&kextRadeonSupport);
	// Mojave dropped legacy GPU support (5xxx and 6xxx).
	if (getKernelVersion() < KernelVersion::Mojave)
		lilu.onKextLoadForce(&kextRadeonLegacySupport);
	else
		lilu.onKextLoadForce(&kextPolarisController);

	initHardwareKextMods();

	//FIXME: autodetect?
	uint32_t powerGatingMask = 0;
	PE_parse_boot_argn("radpg", &powerGatingMask, sizeof(powerGatingMask));
	for (size_t i = 0; i < arrsize(powerGatingFlags); i++) {
		if (!(powerGatingMask & (1 << i))) {
			DBGLOG("rad", "not enabling %s", powerGatingFlags[i]);
			powerGatingFlags[i] = nullptr;
		} else {
			DBGLOG("rad", "enabling %s", powerGatingFlags[i]);
		}
	}
}

void RAD::deinit() {

}

void RAD::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	bool hasAMD = false;
	for (size_t i = 0; i < info->videoExternal.size(); i++) {
		if (info->videoExternal[i].vendor == WIOKit::VendorID::ATIAMD) {
			if (!hasAMD) {
				enableGvaSupport = getKernelVersion() >= KernelVersion::Mojave;
				hasAMD = true;
			}

			if (info->videoExternal[i].video->getProperty("disable-gva-support"))
				enableGvaSupport = false;

			// When injecting values into device properties one cannot specify boolean types.
			// Provide special support for Force_Load_FalconSMUFW.
			auto smufw = OSDynamicCast(OSData, info->videoExternal[i].video->getProperty("Force_Load_FalconSMUFW"));
			if (smufw && smufw->getLength() == 1)
				info->videoExternal[i].video->setProperty("Force_Load_FalconSMUFW",
					*static_cast<const uint8_t *>(smufw->getBytesNoCopy()) ? kOSBooleanTrue : kOSBooleanFalse);
		}
	}

	if (hasAMD) {
		int gva;
		if (PE_parse_boot_argn("radgva", &gva, sizeof(gva)))
			enableGvaSupport = gva != 0;

		KernelPatcher::RouteRequest requests[] {
			KernelPatcher::RouteRequest("__ZN15IORegistryEntry11setPropertyEPKcPvj", wrapSetProperty, orgSetProperty),
			KernelPatcher::RouteRequest("__ZNK15IORegistryEntry11getPropertyEPKc", wrapGetProperty, orgGetProperty),
		};
		patcher.routeMultiple(KernelPatcher::KernelID, requests);

		if (useCustomAgdpDecision && info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple)
			useCustomAgdpDecision = false;
	} else {
		kextRadeonFramebuffer.switchOff();
		kextRadeonLegacyFramebuffer.switchOff();
		kextRadeonSupport.switchOff();
		kextRadeonLegacySupport.switchOff();

		for (size_t i = 0; i < maxHardwareKexts; i++)
			kextRadeonHardware[i].switchOff();
	}
}

void RAD::updatePwmMaxBrightnessFromInternalDisplay() {
	OSDictionary * matching = IOService::serviceMatching("AppleBacklightDisplay");
	if (matching == nullptr) {
		DBGLOG("igfx", "isRadeonX6000WiredToInternalDisplay null AppleBacklightDisplay");
		return;
	}
	
	OSIterator *iter = IOService::getMatchingServices(matching);
	if (iter == nullptr) {
		DBGLOG("igfx", "isRadeonX6000WiredToInternalDisplay null matching");
		matching->release();
		return;
	}
	
	IORegistryEntry* display = OSDynamicCast(IORegistryEntry, iter->getNextObject());
	if (display == nullptr) {
		DBGLOG("igfx", "isRadeonX6000WiredToInternalDisplay null display");
		iter->release();
		matching->release();
		return;
	}
	
	OSDictionary* iodispparm = OSDynamicCast(OSDictionary, display->getProperty("IODisplayParameters"));
	if (iodispparm == nullptr) {
		DBGLOG("igfx", "isRadeonX6000WiredToInternalDisplay null IODisplayParameters");
		iter->release();
		matching->release();
		return;
	}
	
	OSDictionary* linearbri = OSDynamicCast(OSDictionary, iodispparm->getObject("linear-brightness"));
	if (linearbri == nullptr) {
		DBGLOG("igfx", "isRadeonX6000WiredToInternalDisplay null linear-brightness");
		iter->release();
		matching->release();
		return;
	}
	
	OSNumber* maxbri = OSDynamicCast(OSNumber, linearbri->getObject("max"));
	if (maxbri == nullptr) {
		DBGLOG("igfx", "isRadeonX6000WiredToInternalDisplay null max");
		iter->release();
		matching->release();
		return;
	}

	callbackRAD->maxPwmBacklightLvl = maxbri->unsigned32BitValue();
	DBGLOG("igfx", "updatePwmMaxBrightnessFromInternalDisplay get max brightness: 0x%x", callbackRAD->maxPwmBacklightLvl);

	iter->release();
	matching->release();
}

uint32_t RAD::wrapDcePanelCntlHwInit(void *panel_cntl) {
	callbackRAD->panelCntlPtr = panel_cntl;
	callbackRAD->updatePwmMaxBrightnessFromInternalDisplay(); // read max brightness value from IOReg
	uint32_t ret = FunctionCast(wrapDcePanelCntlHwInit, callbackRAD->orgDcePanelCntlHwInit)(panel_cntl);
	return ret;
}

IOReturn RAD::wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t value) {
	IOReturn ret = FunctionCast(wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute, callbackRAD->orgAMDRadeonX6000AmdRadeonFramebufferSetAttribute)(framebuffer, connectIndex, attribute, value);
	if (attribute != (UInt32)'bklt') {
		return ret;
	}
	
	if (callbackRAD->maxPwmBacklightLvl == 0) {
		DBGLOG("igfx", "wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute zero maxPwmBacklightLvl");
		return 0;
	}
	
	if (callbackRAD->panelCntlPtr == nullptr) {
		DBGLOG("igfx", "wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute null panel cntl");
		return 0;
	}
	
	if (callbackRAD->orgDceDriverSetBacklight == nullptr) {
		DBGLOG("igfx", "wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute null orgDcLinkSetBacklightLevel");
		return 0;
	}
	
	// set the backlight of AMD navi10 driver
	callbackRAD->curPwmBacklightLvl = (uint32_t)value;
	uint32_t btlper = callbackRAD->curPwmBacklightLvl * 100 / callbackRAD->maxPwmBacklightLvl;
	uint32_t pwmval = 0;
	if (btlper >= 100) {
		// This is from the dmcu_set_backlight_level function of Linux source
		// ...
		// if (backlight_pwm_u16_16 & 0x10000)
		// 	   backlight_8_bit = 0xFF;
		// else
		// 	   backlight_8_bit = (backlight_pwm_u16_16 >> 8) & 0xFF;
		// ...
		// The max brightness should have 0x10000 bit set
		pwmval = 0x1FF00;
	} else {
		pwmval = ((btlper * 0xFF) / 100) << 8U;
	}

	callbackRAD->orgDceDriverSetBacklight(callbackRAD->panelCntlPtr, pwmval);
	return 0;
}

IOReturn RAD::wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t * value) {
	IOReturn ret = FunctionCast(wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute, callbackRAD->orgAMDRadeonX6000AmdRadeonFramebufferGetAttribute)(framebuffer, connectIndex, attribute, value);
	if (attribute == (UInt32)'bklt') {
		// enable the backlight feature of AMD navi10 driver
		*value = callbackRAD->curPwmBacklightLvl;
		ret = 0;
	}
	return ret;
}

bool RAD::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (kextRadeonX6000Framebuffer.loadIndex == index) {
		KernelPatcher::RouteRequest requests[] = {
			{"_dce_panel_cntl_hw_init", wrapDcePanelCntlHwInit, orgDcePanelCntlHwInit},
			{"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25setAttributeForConnectionEijm", wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute, orgAMDRadeonX6000AmdRadeonFramebufferSetAttribute},
			{"__ZN35AMDRadeonX6000_AmdRadeonFramebuffer25getAttributeForConnectionEijPm", wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute, orgAMDRadeonX6000AmdRadeonFramebufferGetAttribute},
		};

		if (!patcher.routeMultiple(index, requests, address, size, true, true))
			SYSLOG("igfx", "Failed to route redeon x6000 gpu tracing.");
		
		orgDceDriverSetBacklight = reinterpret_cast<t_DceDriverSetBacklight>(patcher.solveSymbol(index, "_dce_driver_set_backlight"));
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("igfx", "failed to resolve _dce_driver_set_backlight");
			patcher.clearError();
		}
	}
	
	if (kextRadeonFramebuffer.loadIndex == index) {
		if (force24BppMode)
			process24BitOutput(patcher, kextRadeonFramebuffer, address, size);
		return true;
	}

	if (kextRadeonLegacyFramebuffer.loadIndex == index) {
		if (force24BppMode)
			process24BitOutput(patcher, kextRadeonLegacyFramebuffer, address, size);
		return true;
	}

	if (kextRadeonSupport.loadIndex == index) {
		processConnectorOverrides(patcher, address, size, true);

		if (getKernelVersion() > KernelVersion::Mojave ||
			(getKernelVersion() == KernelVersion::Mojave && getKernelMinorVersion() >= 5)) {
			KernelPatcher::RouteRequest request("__ZN13ATIController8TestVRAME13PCI_REG_INDEXb", doNotTestVram);
			patcher.routeMultiple(index, &request, 1, address, size);
		}

		if (useCustomAgdpDecision) {
			KernelPatcher::RouteRequest request("__ZN16AtiDeviceControl16notifyLinkChangeE31kAGDCRegisterLinkControlEvent_tmj", wrapNotifyLinkChange, orgNotifyLinkChange);
			patcher.routeMultiple(index, &request, 1, address, size);
		}

		return true;
	}

	if (kextRadeonLegacySupport.loadIndex == index) {
		processConnectorOverrides(patcher, address, size, false);
		return true;
	}

	if (kextPolarisController.loadIndex == index) {
		KernelPatcher::RouteRequest request("__ZN17AMD9500Controller23findProjectByPartNumberEP20ControllerProperties", findProjectByPartNumber);
		patcher.routeMultiple(index, &request, 1, address, size);
	}

	for (size_t i = 0; i < maxHardwareKexts; i++) {
		if (kextRadeonHardware[i].loadIndex == index) {
			processHardwareKext(patcher, i, address, size);
			return true;
		}
	}

	return false;
}

void RAD::initHardwareKextMods() {
	// Decide on kext amount present for optimal performance.
	// 10.15+   has X4000, X5000, and X6000
	// 10.14+   has X4000 and X5000
	// 10.13.4+ has X3000, X4000, and X5000
	if (getKernelVersion() >= KernelVersion::Catalina)
		maxHardwareKexts = MaxRadeonHardwareCatalina;
	else if (getKernelVersion() >= KernelVersion::Mojave)
		maxHardwareKexts = MaxRadeonHardwareMojave;
	else if (getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() >= 5)
		maxHardwareKexts = MaxRadeonHardwareModernHighSierra;

	// 10.13.4 fixed black screen issues
	if (maxHardwareKexts != MaxRadeonHardware) {
		for (size_t i = 0; i < MaxGetFrameBufferProcs; i++)
			getFrameBufferProcNames[IndexRadeonHardwareX4000][i] = nullptr;

		// We have nothing to do for these kexts on recent systems
		if (!fixConfigName && !forceOpenGL && !forceCodecInfo) {
			// X4000 kext is not included in this list as we need to fix GVA properties for most of its GPUs
			kextRadeonHardware[IndexRadeonHardwareX5000].switchOff();
			kextRadeonHardware[IndexRadeonHardwareX6000].switchOff();
		}
	}

	if (getKernelVersion() < KernelVersion::Catalina) {
		kextRadeonHardware[IndexRadeonHardwareX6000].switchOff();
	}

	if (getKernelVersion() < KernelVersion::HighSierra) {
		// Versions before 10.13 do not support X4250 and X5000
		kextRadeonHardware[IndexRadeonHardwareX4250].switchOff();
		kextRadeonHardware[IndexRadeonHardwareX5000].switchOff();

		// Versions before 10.13 have legacy X3000 and X4000 IDs
		kextRadeonHardware[IndexRadeonHardwareX3000].id = idRadeonX3000Old;
		kextRadeonHardware[IndexRadeonHardwareX4000].id = idRadeonX4000Old;

		bool preSierra = getKernelVersion() < KernelVersion::Sierra;

		if (preSierra) {
			// Versions before 10.12 do not support X4100
			kextRadeonHardware[IndexRadeonHardwareX4100].switchOff();
		}

		if (preSierra || (getKernelVersion() == KernelVersion::Sierra && getKernelMinorVersion() < 7)) {
			// Versions before 10.12.6 do not support X4150, X4200
			kextRadeonHardware[IndexRadeonHardwareX4150].switchOff();
			kextRadeonHardware[IndexRadeonHardwareX4200].switchOff();
		}
	}

	lilu.onKextLoadForce(kextRadeonHardware, maxHardwareKexts);
}

void RAD::process24BitOutput(KernelPatcher &patcher, KernelPatcher::KextInfo &info, mach_vm_address_t address, size_t size) {
	auto bitsPerComponent = patcher.solveSymbol<int *>(info.loadIndex, "__ZL18BITS_PER_COMPONENT", address, size);
	if (bitsPerComponent) {
		while (bitsPerComponent && *bitsPerComponent) {
			if (*bitsPerComponent == 10) {
				auto ret = MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
				if (ret == KERN_SUCCESS) {
					DBGLOG("rad", "fixing BITS_PER_COMPONENT");
					*bitsPerComponent = 8;
					MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
				} else {
					SYSLOG("rad", "failed to disable write protection for BITS_PER_COMPONENT");
				}
			}
			bitsPerComponent++;
		}
	} else {
		SYSLOG("rad", "failed to find BITS_PER_COMPONENT");
		patcher.clearError();
	}

	DBGLOG("rad", "fixing pixel types");

	KernelPatcher::LookupPatch pixelPatch {
		&info,
		reinterpret_cast<const uint8_t *>("--RRRRRRRRRRGGGGGGGGGGBBBBBBBBBB"),
		reinterpret_cast<const uint8_t *>("--------RRRRRRRRGGGGGGGGBBBBBBBB"),
		32, 2
	};

	patcher.applyLookupPatch(&pixelPatch);
	if (patcher.getError() != KernelPatcher::Error::NoError) {
		SYSLOG("rad", "failed to patch RGB mask for 24-bit output");
		patcher.clearError();
	}
}

void RAD::processConnectorOverrides(KernelPatcher &patcher, mach_vm_address_t address, size_t size, bool modern) {
	if (modern) {
		if (getKernelVersion() >= KernelVersion::HighSierra) {
			KernelPatcher::RouteRequest requests[] {
				KernelPatcher::RouteRequest("__ZN14AtiBiosParser116getConnectorInfoEP13ConnectorInfoRh", wrapGetConnectorsInfoV1, orgGetConnectorsInfoV1),
				KernelPatcher::RouteRequest("__ZN14AtiBiosParser216getConnectorInfoEP13ConnectorInfoRh", wrapGetConnectorsInfoV2, orgGetConnectorsInfoV2),
				KernelPatcher::RouteRequest("__ZN14AtiBiosParser126translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V117AtomConnectorInfoER13ConnectorInfo",
											wrapTranslateAtomConnectorInfoV1, orgTranslateAtomConnectorInfoV1),
				KernelPatcher::RouteRequest("__ZN14AtiBiosParser226translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V217AtomConnectorInfoER13ConnectorInfo",
											wrapTranslateAtomConnectorInfoV2, orgTranslateAtomConnectorInfoV2),
				KernelPatcher::RouteRequest("__ZN13ATIController5startEP9IOService", wrapATIControllerStart, orgATIControllerStart)
			};
			patcher.routeMultiple(kextRadeonSupport.loadIndex, requests, address, size);
		} else {
			KernelPatcher::RouteRequest requests[] {
				KernelPatcher::RouteRequest("__ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh", wrapGetConnectorsInfoV1, orgGetConnectorsInfoV1),
				KernelPatcher::RouteRequest("__ZN13ATIController5startEP9IOService", wrapATIControllerStart, orgATIControllerStart),
			};
			patcher.routeMultiple(kextRadeonSupport.loadIndex, requests, address, size);

			orgGetAtomObjectTableForType = reinterpret_cast<t_getAtomObjectTableForType>(patcher.solveSymbol(kextRadeonSupport.loadIndex,
																											 "__ZN20AtiAtomBiosUtilities25getAtomObjectTableForTypeEhRh", address, size));
			if (!orgGetAtomObjectTableForType) {
				SYSLOG("rad", "failed to find AtiAtomBiosUtilities::getAtomObjectTableForType");
				patcher.clearError();
			}
		}
	} else {
		KernelPatcher::RouteRequest requests[] {
			KernelPatcher::RouteRequest("__ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh", wrapLegacyGetConnectorsInfo, orgLegacyGetConnectorsInfo),
			KernelPatcher::RouteRequest("__ZN19AMDLegacyController5startEP9IOService", wrapLegacyATIControllerStart, orgLegacyATIControllerStart),
		};
		patcher.routeMultiple(kextRadeonLegacySupport.loadIndex, requests, address, size);

		orgLegacyGetAtomObjectTableForType = patcher.solveSymbol<t_getAtomObjectTableForType>(kextRadeonLegacySupport.loadIndex,
																							  "__ZN20AtiAtomBiosUtilities25getAtomObjectTableForTypeEhRh", address, size);
		if (!orgLegacyGetAtomObjectTableForType) {
			SYSLOG("rad", "failed to find AtiAtomBiosUtilities::getAtomObjectTableForType");
			patcher.clearError();
		}
	}
}

void RAD::processHardwareKext(KernelPatcher &patcher, size_t hwIndex, mach_vm_address_t address, size_t size) {
	auto getFrame = getFrameBufferProcNames[hwIndex];
	auto &hardware = kextRadeonHardware[hwIndex];

	// Fix boot and wake to black screen
	for (size_t j = 0; j < MaxGetFrameBufferProcs && getFrame[j] != nullptr; j++) {
		auto getFB = patcher.solveSymbol(hardware.loadIndex, getFrame[j], address, size);
		if (getFB) {
			// Initially it was discovered that the only problematic register is PRIMARY_SURFACE_ADDRESS_HIGH (0x1A07).
			// This register must be nulled to solve most of the issues.
			// Depending on the amount of connected screens PRIMARY_SURFACE_ADDRESS (0x1A04) may not be null.
			// However, as of AMD Vega drivers in 10.13 DP1 both of these registers are now ignored.
			// Furthermore, there are no (extra) issues from just returning 0 in framebuffer base address.

			// xor rax, rax
			// ret
			uint8_t ret[] {0x48, 0x31, 0xC0, 0xC3};
			patcher.routeBlock(getFB, ret, sizeof(ret));
			if (patcher.getError() == KernelPatcher::Error::NoError) {
				DBGLOG("rad", "patched %s", getFrame[j]);
			} else {
				SYSLOG("rad", "failed to patch %s code %d", getFrame[j], patcher.getError());
				patcher.clearError();
			}
		} else {
			SYSLOG("rad", "failed to find %s code %d", getFrame[j], patcher.getError());
			patcher.clearError();
		}
	}

	// Fix reported Accelerator name to support WhateverName.app
	// Also fix GVA properties for X4000.
	if (fixConfigName || hwIndex == IndexRadeonHardwareX4000) {
		KernelPatcher::RouteRequest request(populateAccelConfigProcNames[hwIndex], wrapPopulateAccelConfig[hwIndex], orgPopulateAccelConfig[hwIndex]);
		patcher.routeMultiple(hardware.loadIndex, &request, 1, address, size);
	}

	// Enforce OpenGL support if requested
	if (forceOpenGL) {
		DBGLOG("rad", "disabling Metal support");
		uint8_t find1[] {0x4D, 0x65, 0x74, 0x61, 0x6C, 0x53, 0x74, 0x61};
		uint8_t find2[] {0x4D, 0x65, 0x74, 0x61, 0x6C, 0x50, 0x6C, 0x75};
		uint8_t repl1[] {0x50, 0x65, 0x74, 0x61, 0x6C, 0x53, 0x74, 0x61};
		uint8_t repl2[] {0x50, 0x65, 0x74, 0x61, 0x6C, 0x50, 0x6C, 0x75};

		KernelPatcher::LookupPatch antimetal[] {
			{&hardware, find1, repl1, sizeof(find1), 2},
			{&hardware, find2, repl2, sizeof(find1), 2}
		};

		for (auto &p : antimetal) {
			patcher.applyLookupPatch(&p);
			patcher.clearError();
		}
	}

	// Patch AppleGVA support for non-supported models
	if (forceCodecInfo && getHWInfoProcNames[hwIndex] != nullptr) {
		KernelPatcher::RouteRequest request(getHWInfoProcNames[hwIndex], wrapGetHWInfo[hwIndex], orgGetHWInfo[hwIndex]);
		patcher.routeMultiple(hardware.loadIndex, &request, 1, address, size);
	}
}

void RAD::mergeProperty(OSDictionary *props, const char *name, OSObject *value) {
	// The only type we could make from device properties is data.
	// To be able to override other types we do a conversion here.
	auto data = OSDynamicCast(OSData, value);
	if (data) {
		// It is hard to make a boolean even from ACPI, so we make a hack here:
		// 1-byte OSData with 0x01 / 0x00 values becomes boolean.
		auto val = static_cast<const uint8_t *>(data->getBytesNoCopy());
		auto len = data->getLength();
		if (val && len == sizeof(uint8_t)) {
			if (val[0] == 1) {
				props->setObject(name, kOSBooleanTrue);
				DBGLOG("rad", "prop %s was merged as kOSBooleanTrue", name);
				return;
			} else if (val[0] == 0) {
				props->setObject(name, kOSBooleanFalse);
				DBGLOG("rad", "prop %s was merged as kOSBooleanFalse", name);
				return;
			}
		}

		// Consult the original value to make a decision
		auto orgValue = props->getObject(name);
		if (val && orgValue) {
			DBGLOG("rad", "prop %s has original value", name);
			if (len == sizeof(uint32_t) && OSDynamicCast(OSNumber, orgValue)) {
				auto num = *reinterpret_cast<const uint32_t *>(val);
				auto osnum = OSNumber::withNumber(num, 32);
				if (osnum) {
					DBGLOG("rad", "prop %s was merged as number %u", name, num);
					props->setObject(name, osnum);
					osnum->release();
				}
				return;
			} else if (len > 0 && val[len-1] == '\0' && OSDynamicCast(OSString, orgValue)) {
				auto str = reinterpret_cast<const char *>(val);
				auto osstr = OSString::withCString(str);
				if (osstr) {
					DBGLOG("rad", "prop %s was merged as string %s", name, str);
					props->setObject(name, osstr);
					osstr->release();
				}
				return;
			}
		} else {
			DBGLOG("rad", "prop %s has no original value", name);
		}
	}

	// Default merge as is
	props->setObject(name, value);
	DBGLOG("rad", "prop %s was merged", name);
}

void RAD::mergeProperties(OSDictionary *props, const char *prefix, IOService *provider) {
	// Should be ok, but in case there are issues switch to dictionaryWithProperties();
	auto dict = provider->getPropertyTable();
	if (dict) {
		auto iterator = OSCollectionIterator::withCollection(dict);
		if (iterator) {
			OSSymbol *propname;
			size_t prefixlen = strlen(prefix);
			while ((propname = OSDynamicCast(OSSymbol, iterator->getNextObject())) != nullptr) {
				auto name = propname->getCStringNoCopy();
				if (name && propname->getLength() > prefixlen && !strncmp(name, prefix, prefixlen)) {
					auto prop = dict->getObject(propname);
					if (prop)
						mergeProperty(props, name + prefixlen, prop);
					else
						DBGLOG("rad", "prop %s was not merged due to no value", name);
				} else {
					//DBGLOG("rad", "prop %s does not match %s prefix", safeString(name), prefix);
				}
			}

			iterator->release();
		} else {
			SYSLOG("rad", "prop merge failed to iterate over properties");
		}
	} else {
		SYSLOG("rad", "prop merge failed to get properties");
	}

	if (!strcmp(prefix, "CAIL,")) {
		for (size_t i = 0; i < arrsize(powerGatingFlags); i++) {
			if (powerGatingFlags[i] && props->getObject(powerGatingFlags[i])) {
				DBGLOG("rad", "cail prop merge found %s, replacing", powerGatingFlags[i]);
				auto num = OSNumber::withNumber(1, 32);
				if (num) {
					props->setObject(powerGatingFlags[i], num);
					num->release();
				}
			}
		}
	}
}

void RAD::applyPropertyFixes(IOService *service, uint32_t connectorNum) {
	if (service && getKernelVersion() >= KernelVersion::HighSierra) {
		// Starting with 10.13.2 this is important to fix sleep issues due to enforced 6 screens
		if (!service->getProperty("CFG,CFG_FB_LIMIT")) {
			DBGLOG("rad", "setting fb limit to %u", connectorNum);
			service->setProperty("CFG_FB_LIMIT", connectorNum, 32);
		}

		// In the past we set CFG_USE_AGDC to false, which caused visual glitches and broken multimonitor support.
		// A better workaround is to disable AGDP just like we do globally.
	}
}

void RAD::updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl, RADConnectors::Connector *connectors, uint8_t *sz) {
	if (atomutils) {
		DBGLOG("rad", "getConnectorsInfo found %u connectors", *sz);
		RADConnectors::print(connectors, *sz);
	}

	// Check if the user wants to override automatically detected connectors
	auto cons = ctrl->getProperty("connectors");
	if (cons) {
		auto consData = OSDynamicCast(OSData, cons);
		if (consData) {
			auto consPtr = consData->getBytesNoCopy();
			auto consSize = consData->getLength();

			uint32_t consCount;
			if (WIOKit::getOSDataValue(ctrl, "connector-count", consCount)) {
				*sz = consCount;
				DBGLOG("rad", "getConnectorsInfo got size override to %u", *sz);
			}

			if (consPtr && consSize > 0 && *sz > 0 && RADConnectors::valid(consSize, *sz)) {
				RADConnectors::copy(connectors, *sz, static_cast<const RADConnectors::Connector *>(consPtr), consSize);
				DBGLOG("rad", "getConnectorsInfo installed %u connectors", *sz);
				applyPropertyFixes(ctrl, *sz);
			} else {
				DBGLOG("rad", "getConnectorsInfo conoverrides have invalid size %u for %u num", consSize, *sz);
			}
		} else {
			DBGLOG("rad", "getConnectorsInfo conoverrides have invalid type");
		}
	} else {
		if (atomutils) {
			DBGLOG("rad", "getConnectorsInfo attempting to autofix connectors");
			uint8_t sHeader = 0, displayPathNum = 0, connectorObjectNum = 0;
			auto baseAddr = static_cast<uint8_t *>(gettable(atomutils, AtomObjectTableType::Common, &sHeader)) - sizeof(uint32_t);
			auto displayPaths = static_cast<AtomDisplayObjectPath *>(gettable(atomutils, AtomObjectTableType::DisplayPath, &displayPathNum));
			auto connectorObjects = static_cast<AtomConnectorObject *>(gettable(atomutils, AtomObjectTableType::ConnectorObject, &connectorObjectNum));
			if (displayPathNum == connectorObjectNum)
				autocorrectConnectors(baseAddr, displayPaths, displayPathNum, connectorObjects, connectorObjectNum, connectors, *sz);
			else
				DBGLOG("rad", "getConnectorsInfo found different displaypaths %u and connectors %u", displayPathNum, connectorObjectNum);
		}

		applyPropertyFixes(ctrl, *sz);

		// Prioritise connectors, since it may cause black screen on e.g. R9 370
		const uint8_t *senseList = nullptr;
		uint8_t senseNum = 0;
		auto priData = OSDynamicCast(OSData, ctrl->getProperty("connector-priority"));
		if (priData) {
			senseList = static_cast<const uint8_t *>(priData->getBytesNoCopy());
			senseNum = static_cast<uint8_t>(priData->getLength());
			DBGLOG("rad", "getConnectorInfo found %u senses in connector-priority", senseNum);
			reprioritiseConnectors(senseList, senseNum, connectors, *sz);
		} else {
			DBGLOG("rad", "getConnectorInfo leaving unchaged priority");
		}
	}

	DBGLOG("rad", "getConnectorsInfo resulting %u connectors follow", *sz);
	RADConnectors::print(connectors, *sz);
}

void RAD::autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum, AtomConnectorObject *connectorObjects,
								uint8_t connectorObjectNum, RADConnectors::Connector *connectors, uint8_t sz) {
	for (uint8_t i = 0; i < displayPathNum; i++) {
		if (!isEncoder(displayPaths[i].usGraphicObjIds)) {
			DBGLOG("rad", "autocorrectConnectors not encoder %X at %u", displayPaths[i].usGraphicObjIds, i);
			continue;
		}

		uint8_t txmit = 0, enc = 0;
		if (!getTxEnc(displayPaths[i].usGraphicObjIds, txmit, enc))
			continue;

		uint8_t sense = getSenseID(baseAddr + connectorObjects[i].usRecordOffset);
		if (!sense) {
			DBGLOG("rad", "autocorrectConnectors failed to detect sense for %u connector", i);
			continue;
		}

		DBGLOG("rad", "autocorrectConnectors found txmit %02X enc %02X sense %02X for %u connector", txmit, enc, sense, i);

		autocorrectConnector(getConnectorID(displayPaths[i].usConnObjectId), sense, txmit, enc, connectors, sz);
	}
}

void RAD::autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit, uint8_t enc, RADConnectors::Connector *connectors, uint8_t sz) {
	// This function attempts to fix the following issues:
	//
	// 1. Incompatible DVI transmitter on 290X, 370 and probably some other models
	// In this case a correct transmitter is detected by AtiAtomBiosDce60::getPropertiesForEncoderObject, however, later
	// in AtiAtomBiosDce60::getPropertiesForConnectorObject for DVI DL and TITFP513 this value is conjuncted with 0xCF,
	// which makes it wrong: 0x10 -> 0, 0x11 -> 1. As a result one gets black screen when connecting multiple displays.
	// getPropertiesForEncoderObject takes usGraphicObjIds and getPropertiesForConnectorObject takes usConnObjectId

	if (callbackRAD->dviSingleLink) {
		if (connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I &&
			connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D &&
			connector != CONNECTOR_OBJECT_ID_LVDS) {
			DBGLOG("rad", "autocorrectConnector found unsupported connector type %02X", connector);
			return;
		}

		auto fixTransmit = [](auto &con, uint8_t idx, uint8_t sense, uint8_t txmit) {
			if (con.sense == sense) {
				if (con.transmitter != txmit && (con.transmitter & 0xCF) == con.transmitter) {
					DBGLOG("rad", "autocorrectConnector replacing txmit %02X with %02X for %u connector sense %02X",
						   con.transmitter, txmit, idx, sense);
					con.transmitter = txmit;
				}
				return true;
			}
			return false;
		};

		bool isModern = RADConnectors::modern();
		for (uint8_t j = 0; j < sz; j++) {
			if (isModern) {
				auto &con = (&connectors->modern)[j];
				if (fixTransmit(con, j, sense, txmit))
					break;
			} else {
				auto &con = (&connectors->legacy)[j];
				if (fixTransmit(con, j, sense, txmit))
					break;
			}
		}
	} else {
		DBGLOG("rad", "autocorrectConnector use -raddvi to enable dvi autocorrection");
	}
}

void RAD::reprioritiseConnectors(const uint8_t *senseList, uint8_t senseNum, RADConnectors::Connector *connectors, uint8_t sz) {
	static constexpr uint32_t typeList[] {
		RADConnectors::ConnectorLVDS,
		RADConnectors::ConnectorDigitalDVI,
		RADConnectors::ConnectorHDMI,
		RADConnectors::ConnectorDP,
		RADConnectors::ConnectorVGA
	};
	static constexpr uint8_t typeNum {static_cast<uint8_t>(arrsize(typeList))};

	bool isModern = RADConnectors::modern();
	uint16_t priCount = 1;
	// Automatically detected connectors have equal priority (0), which often results in black screen
	// This allows to change this firstly by user-defined list, then by type list.
	//TODO: priority is ignored for 5xxx and 6xxx GPUs, should we manually reorder items?
	for (uint8_t i = 0; i < senseNum + typeNum + 1; i++) {
		for (uint8_t j = 0; j < sz; j++) {
			auto reorder = [&](auto &con) {
				if (i == senseNum + typeNum) {
					if (con.priority == 0)
						con.priority = priCount++;
				} else if (i < senseNum) {
					if (con.sense == senseList[i]) {
						DBGLOG("rad", "reprioritiseConnectors setting priority of sense %02X to %u by sense", con.sense, priCount);
						con.priority = priCount++;
						return true;
					}
				} else {
					if (con.priority == 0 && con.type == typeList[i-senseNum]) {
						DBGLOG("rad", "reprioritiseConnectors setting priority of sense %02X to %u by type", con.sense, priCount);
						con.priority = priCount++;
					}
				}
				return false;
			};

			if ((isModern && reorder((&connectors->modern)[j])) ||
				(!isModern && reorder((&connectors->legacy)[j])))
				break;
		}
	}
}

void RAD::setGvaProperties(IOService *accelService) {
	auto codecStr = OSDynamicCast(OSString, accelService->getProperty("IOGVACodec"));
	if (codecStr == nullptr) {
		DBGLOG("rad", "updating X4000 accelerator IOGVACodec to VCE");
		accelService->setProperty("IOGVACodec", "VCE");
	} else {
		auto codec = codecStr->getCStringNoCopy();
		DBGLOG("rad", "X4000 accelerator IOGVACodec is already set to %s", safeString(codec));
		if (codec != nullptr && strncmp(codec, "AMD", strlen("AMD")) == 0) {
			bool needsDecode = accelService->getProperty("IOGVAHEVCDecode") == nullptr;
			bool needsEncode = accelService->getProperty("IOGVAHEVCEncode") == nullptr;
			if (needsDecode) {
				OSObject *VTMaxDecodeLevel = OSNumber::withNumber(153, 32);
				OSString *VTMaxDecodeLevelKey  = OSString::withCString("VTMaxDecodeLevel");
				OSDictionary *VTPerProfileDetailsInner = OSDictionary::withCapacity(1);
				OSDictionary *VTPerProfileDetails = OSDictionary::withCapacity(3);
				OSString *VTPerProfileDetailsKey1 = OSString::withCString("1");
				OSString *VTPerProfileDetailsKey2 = OSString::withCString("2");
				OSString *VTPerProfileDetailsKey3 = OSString::withCString("3");

				OSArray *VTSupportedProfileArray = OSArray::withCapacity(3);
				OSNumber *VTSupportedProfileArray1 = OSNumber::withNumber(1, 32);
				OSNumber *VTSupportedProfileArray2 = OSNumber::withNumber(2, 32);
				OSNumber *VTSupportedProfileArray3 = OSNumber::withNumber(3, 32);

				OSDictionary *IOGVAHEVCDecodeCapabilities = OSDictionary::withCapacity(2);
				OSString *VTPerProfileDetailsKey = OSString::withCString("VTPerProfileDetails");
				OSString *VTSupportedProfileArrayKey = OSString::withCString("VTSupportedProfileArray");

				if (VTMaxDecodeLevel != nullptr && VTMaxDecodeLevelKey != nullptr && VTPerProfileDetailsInner != nullptr &&
					VTPerProfileDetails != nullptr && VTPerProfileDetailsKey1 != nullptr && VTPerProfileDetailsKey2 != nullptr &&
					VTPerProfileDetailsKey3 != nullptr && VTSupportedProfileArrayKey != nullptr && VTSupportedProfileArray1 != nullptr &&
					VTSupportedProfileArray2 != nullptr && VTSupportedProfileArray3 != nullptr && VTSupportedProfileArray != nullptr &&
					VTPerProfileDetailsKey != nullptr && IOGVAHEVCDecodeCapabilities != nullptr) {
					VTPerProfileDetailsInner->setObject(VTMaxDecodeLevelKey, VTMaxDecodeLevel);
					VTPerProfileDetails->setObject(VTPerProfileDetailsKey1, VTPerProfileDetailsInner);
					VTPerProfileDetails->setObject(VTPerProfileDetailsKey2, VTPerProfileDetailsInner);
					VTPerProfileDetails->setObject(VTPerProfileDetailsKey3, VTPerProfileDetailsInner);

					VTSupportedProfileArray->setObject(VTSupportedProfileArray1);
					VTSupportedProfileArray->setObject(VTSupportedProfileArray2);
					VTSupportedProfileArray->setObject(VTSupportedProfileArray3);

					IOGVAHEVCDecodeCapabilities->setObject(VTPerProfileDetailsKey, VTPerProfileDetails);
					IOGVAHEVCDecodeCapabilities->setObject(VTSupportedProfileArrayKey, VTSupportedProfileArray);

					accelService->setProperty("IOGVAHEVCDecode", "1");
					accelService->setProperty("IOGVAHEVCDecodeCapabilities", IOGVAHEVCDecodeCapabilities);

					DBGLOG("rad", "recovering IOGVAHEVCDecode");
				} else {
					SYSLOG("rad", "allocation failure in IOGVAHEVCDecode");
				}

				OSSafeReleaseNULL(VTMaxDecodeLevel);
				OSSafeReleaseNULL(VTMaxDecodeLevelKey);
				OSSafeReleaseNULL(VTPerProfileDetailsInner);
				OSSafeReleaseNULL(VTPerProfileDetails);
				OSSafeReleaseNULL(VTPerProfileDetailsKey1);
				OSSafeReleaseNULL(VTPerProfileDetailsKey2);
				OSSafeReleaseNULL(VTPerProfileDetailsKey3);
				OSSafeReleaseNULL(VTSupportedProfileArrayKey);
				OSSafeReleaseNULL(VTSupportedProfileArray1);
				OSSafeReleaseNULL(VTSupportedProfileArray2);
				OSSafeReleaseNULL(VTSupportedProfileArray3);
				OSSafeReleaseNULL(VTSupportedProfileArray);
				OSSafeReleaseNULL(VTPerProfileDetailsKey);
				OSSafeReleaseNULL(IOGVAHEVCDecodeCapabilities);
			}

			if (needsEncode) {
				OSObject *VTMaxEncodeLevel = OSNumber::withNumber(153, 32);
				OSString *VTMaxEncodeLevelKey  = OSString::withCString("VTMaxEncodeLevel");

				OSDictionary *VTPerProfileDetailsInner = OSDictionary::withCapacity(1);
				OSDictionary *VTPerProfileDetails = OSDictionary::withCapacity(1);
				OSString *VTPerProfileDetailsKey1 = OSString::withCString("1");

				OSArray *VTSupportedProfileArray = OSArray::withCapacity(1);
				OSNumber *VTSupportedProfileArray1 = OSNumber::withNumber(1, 32);

				OSDictionary *IOGVAHEVCEncodeCapabilities = OSDictionary::withCapacity(4);
				OSString *VTPerProfileDetailsKey = OSString::withCString("VTPerProfileDetails");
				OSString *VTQualityRatingKey = OSString::withCString("VTQualityRating");
				OSNumber *VTQualityRating = OSNumber::withNumber(50, 32);
				OSString *VTRatingKey = OSString::withCString("VTRating");
				OSNumber *VTRating = OSNumber::withNumber(350, 32);
				OSString *VTSupportedProfileArrayKey = OSString::withCString("VTSupportedProfileArray");

				if (VTMaxEncodeLevel != nullptr && VTMaxEncodeLevelKey != nullptr && VTPerProfileDetailsInner != nullptr &&
					VTPerProfileDetails != nullptr && VTPerProfileDetailsKey1 != nullptr && VTSupportedProfileArrayKey != nullptr &&
					VTSupportedProfileArray1 != nullptr && VTSupportedProfileArray != nullptr && VTPerProfileDetailsKey != nullptr &&
					VTQualityRatingKey != nullptr && VTQualityRating != nullptr && VTRatingKey != nullptr && VTRating != nullptr &&
					IOGVAHEVCEncodeCapabilities != nullptr) {

					VTPerProfileDetailsInner->setObject(VTMaxEncodeLevelKey, VTMaxEncodeLevel);
					VTPerProfileDetails->setObject(VTPerProfileDetailsKey1, VTPerProfileDetailsInner);
					VTSupportedProfileArray->setObject(VTSupportedProfileArray1);

					IOGVAHEVCEncodeCapabilities->setObject(VTPerProfileDetailsKey, VTPerProfileDetails);
					IOGVAHEVCEncodeCapabilities->setObject(VTQualityRatingKey, VTQualityRating);
					IOGVAHEVCEncodeCapabilities->setObject(VTRatingKey, VTRating);
					IOGVAHEVCEncodeCapabilities->setObject(VTSupportedProfileArrayKey, VTSupportedProfileArray);

					accelService->setProperty("IOGVAHEVCEncode", "1");
					accelService->setProperty("IOGVAHEVCEncodeCapabilities", IOGVAHEVCEncodeCapabilities);

					DBGLOG("rad", "recovering IOGVAHEVCEncode");
				} else {
					SYSLOG("rad", "allocation failure in IOGVAHEVCEncode");
				}

				OSSafeReleaseNULL(VTMaxEncodeLevel);
				OSSafeReleaseNULL(VTMaxEncodeLevelKey);
				OSSafeReleaseNULL(VTPerProfileDetailsInner);
				OSSafeReleaseNULL(VTPerProfileDetails);
				OSSafeReleaseNULL(VTPerProfileDetailsKey1);
				OSSafeReleaseNULL(VTSupportedProfileArrayKey);
				OSSafeReleaseNULL(VTSupportedProfileArray1);
				OSSafeReleaseNULL(VTSupportedProfileArray);
				OSSafeReleaseNULL(VTPerProfileDetailsKey);
				OSSafeReleaseNULL(VTQualityRatingKey);
				OSSafeReleaseNULL(VTQualityRating);
				OSSafeReleaseNULL(VTRatingKey);
				OSSafeReleaseNULL(VTRating);
				OSSafeReleaseNULL(IOGVAHEVCEncodeCapabilities);
			}
		}
	}
}

void RAD::updateAccelConfig(size_t hwIndex, IOService *accelService, const char **accelConfig) {
	if (accelService && accelConfig) {
		if (fixConfigName) {
			auto gpuService = accelService->getParentEntry(gIOServicePlane);

			if (gpuService) {
				auto model = OSDynamicCast(OSData, gpuService->getProperty("model"));
				if (model) {
					auto modelStr = static_cast<const char *>(model->getBytesNoCopy());
					if (modelStr) {
						if (modelStr[0] == 'A' && ((modelStr[1] == 'M' && modelStr[2] == 'D') ||
												   (modelStr[1] == 'T' && modelStr[2] == 'I')) && modelStr[3] == ' ') {
							modelStr += 4;
						}

						DBGLOG("rad", "updateAccelConfig found gpu model %s", modelStr);
						*accelConfig = modelStr;
					} else {
						DBGLOG("rad", "updateAccelConfig found null gpu model");
					}
				} else {
					DBGLOG("rad", "updateAccelConfig failed to find gpu model");
				}

			} else {
				DBGLOG("rad", "updateAccelConfig failed to find accelerator parent");
			}
		}

		if (enableGvaSupport && hwIndex == IndexRadeonHardwareX4000) {
			setGvaProperties(accelService);
		}
	}
}

bool RAD::wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned length) {
	if (length > 10 && aKey && reinterpret_cast<const uint32_t *>(aKey)[0] == 'edom' && reinterpret_cast<const uint16_t *>(aKey)[2] == 'l') {
		DBGLOG("rad", "SetProperty caught model %u (%.*s)", length, length, static_cast<char *>(bytes));
		if (*static_cast<uint32_t *>(bytes) == ' DMA' || *static_cast<uint32_t *>(bytes) == ' ITA' || *static_cast<uint32_t *>(bytes) == 'edaR') {
			if (FunctionCast(wrapGetProperty, callbackRAD->orgGetProperty)(that, aKey)) {
				DBGLOG("rad", "SetProperty ignored setting %s to %s", aKey, static_cast<char *>(bytes));
				return true;
			}
			DBGLOG("rad", "SetProperty missing %s, fallback to %s", aKey, static_cast<char *>(bytes));
		}
	}

	return FunctionCast(wrapSetProperty, callbackRAD->orgSetProperty)(that, aKey, bytes, length);
}

OSObject *RAD::wrapGetProperty(IORegistryEntry *that, const char *aKey) {
	auto obj = FunctionCast(wrapGetProperty, callbackRAD->orgGetProperty)(that, aKey);
	auto props = OSDynamicCast(OSDictionary, obj);

	if (props && aKey) {
		const char *prefix {nullptr};
		auto provider = OSDynamicCast(IOService, that->getParentEntry(gIOServicePlane));
		if (provider) {
			if (aKey[0] == 'a') {
				if (!strcmp(aKey, "aty_config"))
					prefix = "CFG,";
				else if (!strcmp(aKey, "aty_properties"))
					prefix = "PP,";
			} else if (aKey[0] == 'c' && !strcmp(aKey, "cail_properties")) {
				prefix = "CAIL,";
			}

			if (prefix) {
				DBGLOG("rad", "GetProperty discovered property merge request for %s", aKey);
				auto rawProps = props->copyCollection();
				if (rawProps) {
					auto newProps = OSDynamicCast(OSDictionary, rawProps);
					if (newProps) {
						callbackRAD->mergeProperties(newProps, prefix, provider);
						that->setProperty(aKey, newProps);
						obj = newProps;
					}
					rawProps->release();
				}

			}
		}
	}

	return obj;
}

uint32_t RAD::wrapGetConnectorsInfoV1(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
	uint32_t code = FunctionCast(wrapGetConnectorsInfoV1, callbackRAD->orgGetConnectorsInfoV1)(that, connectors, sz);
	auto props = callbackRAD->currentPropProvider.get();

	if (code == 0 && sz && props && *props) {
		if (getKernelVersion() >= KernelVersion::HighSierra)
			callbackRAD->updateConnectorsInfo(nullptr, nullptr, *props, connectors, sz);
		else
			callbackRAD->updateConnectorsInfo(static_cast<void **>(that)[1], callbackRAD->orgGetAtomObjectTableForType, *props, connectors, sz);
	} else {
		DBGLOG("rad", "getConnectorsInfoV1 failed %X or undefined %d", code, props == nullptr);
	}

	return code;
}

uint32_t RAD::wrapGetConnectorsInfoV2(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
	uint32_t code = FunctionCast(wrapGetConnectorsInfoV2, callbackRAD->orgGetConnectorsInfoV2)(that, connectors, sz);
	auto props = callbackRAD->currentPropProvider.get();

	if (code == 0 && sz && props && *props)
		callbackRAD->updateConnectorsInfo(nullptr, nullptr, *props, connectors, sz);
	else
		DBGLOG("rad", "getConnectorsInfoV2 failed %X or undefined %d", code, props == nullptr);

	return code;
}

uint32_t RAD::wrapLegacyGetConnectorsInfo(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
	uint32_t code = FunctionCast(wrapLegacyGetConnectorsInfo, callbackRAD->orgLegacyGetConnectorsInfo)(that, connectors, sz);
	auto props = callbackRAD->currentLegacyPropProvider.get();

	if (code == 0 && sz && props && *props)
		callbackRAD->updateConnectorsInfo(static_cast<void **>(that)[1], callbackRAD->orgLegacyGetAtomObjectTableForType, *props, connectors, sz);
	else
		DBGLOG("rad", "legacy getConnectorsInfo failed %X or undefined %d", code, props == nullptr);

	return code;
}

uint32_t RAD::wrapTranslateAtomConnectorInfoV1(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector) {
	uint32_t code = FunctionCast(wrapTranslateAtomConnectorInfoV1, callbackRAD->orgTranslateAtomConnectorInfoV1)(that, info, connector);

	if (code == 0 && info && connector) {
		RADConnectors::print(connector, 1);

		uint8_t sense = getSenseID(info->i2cRecord);
		if (sense) {
			DBGLOG("rad", "translateAtomConnectorInfoV1 got sense id %02X", sense);

			// We need to extract usGraphicObjIds from info->hpdRecord, which is of type ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT:
			// struct ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT {
			//   uint8_t ucNumberOfSrc;
			//   uint16_t usSrcObjectID[ucNumberOfSrc];
			//   uint8_t ucNumberOfDst;
			//   uint16_t usDstObjectID[ucNumberOfDst];
			// };
			// The value we need is in usSrcObjectID. The structure is byte-packed.

			uint8_t ucNumberOfSrc = info->hpdRecord[0];
			for (uint8_t i = 0; i < ucNumberOfSrc; i++) {
				auto usSrcObjectID = *reinterpret_cast<uint16_t *>(info->hpdRecord + sizeof(uint8_t) + i * sizeof(uint16_t));
				DBGLOG("rad", "translateAtomConnectorInfoV1 checking %04X object id", usSrcObjectID);
				if (((usSrcObjectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT) == GRAPH_OBJECT_TYPE_ENCODER) {
					uint8_t txmit = 0, enc = 0;
					if (getTxEnc(usSrcObjectID, txmit, enc))
						callbackRAD->autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord), txmit, enc, connector, 1);
					break;
				}
			}


		} else {
			DBGLOG("rad", "translateAtomConnectorInfoV1 failed to detect sense for translated connector");
		}
	}

	return code;
}

uint32_t RAD::wrapTranslateAtomConnectorInfoV2(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector) {
	uint32_t code = FunctionCast(wrapTranslateAtomConnectorInfoV2, callbackRAD->orgTranslateAtomConnectorInfoV2)(that, info, connector);

	if (code == 0 && info && connector) {
		RADConnectors::print(connector, 1);

		uint8_t sense = getSenseID(info->i2cRecord);
		if (sense) {
			DBGLOG("rad", "translateAtomConnectorInfoV2 got sense id %02X", sense);
			uint8_t txmit = 0, enc = 0;
			if (getTxEnc(info->usGraphicObjIds, txmit, enc))
				callbackRAD->autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord), txmit, enc, connector, 1);
		} else {
			DBGLOG("rad", "translateAtomConnectorInfoV2 failed to detect sense for translated connector");
		}
	}

	return code;
}

bool RAD::wrapATIControllerStart(IOService *ctrl, IOService *provider) {
	DBGLOG("rad", "starting controller " PRIKADDR, CASTKADDR(current_thread()));
	if (callbackRAD->forceVesaMode) {
		DBGLOG("rad", "disabling video acceleration on request");
		return false;
	}

	callbackRAD->currentPropProvider.set(provider);
	bool r = FunctionCast(wrapATIControllerStart, callbackRAD->orgATIControllerStart)(ctrl, provider);
	DBGLOG("rad", "starting controller done %d " PRIKADDR, r, CASTKADDR(current_thread()));
	callbackRAD->currentPropProvider.erase();

	return r;
}

bool RAD::wrapLegacyATIControllerStart(IOService *ctrl, IOService *provider) {
	DBGLOG("rad", "starting legacy controller " PRIKADDR, CASTKADDR(current_thread()));
	if (callbackRAD->forceVesaMode) {
		DBGLOG("rad", "disabling legacy video acceleration on request");
		return false;
	}

	callbackRAD->currentLegacyPropProvider.set(provider);
	bool r = FunctionCast(wrapLegacyATIControllerStart, callbackRAD->orgLegacyATIControllerStart)(ctrl, provider);
	DBGLOG("rad", "starting legacy legacy controller done %d " PRIKADDR, r, CASTKADDR(current_thread()));
	callbackRAD->currentLegacyPropProvider.erase();

	return r;
}

IOReturn RAD::findProjectByPartNumber(IOService *ctrl, void *properties) {
	// Drivers have predefined framebuffers for the following models:
	// 113-4E353BU, 113-4E3531U, 113-C94002A1XTA
	// Despite this looking sane, at least with Sapphire 113-4E353BU-O50 (RX 580) these framebuffers break connectors.
	return kIOReturnNotFound;
}

bool RAD::doNotTestVram(IOService *ctrl, uint32_t reg, bool retryOnFail) {
	// Based on vladie's patch description:
	// TestVRAM fills memory with 0xaa55aa55 bytes (it's magenta pixels visible onscreen),
	// and it tries to test too much of address space, writing this bytes to framebuffer memory.
	// If you have verbose mode enabled (as i have), there is a possibility that framebuffer
	// will scroll during this test, and TestVRAM will write 0xaa55aa55, but read 0x00000000
	// (because magenta-colored pixels are scrolled up) causing kernel panic.
	//
	// Here we just do not do video memory testing for simplicity.
	return true;
}

bool RAD::wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData, uint32_t eventFlags) {
	auto ret = FunctionCast(wrapNotifyLinkChange, callbackRAD->orgNotifyLinkChange)(atiDeviceControl, event, eventData, eventFlags);

	if (event == kAGDCValidateDetailedTiming) {
		auto cmd = static_cast<AGDCValidateDetailedTiming_t *>(eventData);
		DBGLOG("rad", "AGDCValidateDetailedTiming %u -> %d (%u)", cmd->framebufferIndex, ret, cmd->modeStatus);
		// While we have this condition below, the only actual value we get is ret = true, cmd->modeStatus = 0.
		// This is because AGDP is disabled, and starting from 10.15.1b2 AMDFramebuffer no longer accepts 0 in
		// __ZN14AMDFramebuffer22validateDetailedTimingEPvy
		if (ret == false || cmd->modeStatus < 1 || cmd->modeStatus > 3) {
			cmd->modeStatus = 2;
			ret = true;
		}
	}

	return ret;
}

void RAD::updateGetHWInfo(IOService *accelVideoCtx, void *hwInfo) {
	IOService *accel, *pciDev;
	accel = OSDynamicCast(IOService, accelVideoCtx->getParentEntry(gIOServicePlane));
	if (accel == NULL) {
		SYSLOG("rad", "getHWInfo: no parent found for accelVideoCtx!");
		return;
	}
	pciDev = OSDynamicCast(IOService, accel->getParentEntry(gIOServicePlane));
	if (pciDev == NULL) {
		SYSLOG("rad", "getHWInfo: no parent found for accel!");
		return;
	}
	uint16_t &org = getMember<uint16_t>(hwInfo, 0x4);
	uint32_t dev = org;
	if (!WIOKit::getOSDataValue(pciDev, "codec-device-id", dev)) {
		// fallback to device-id only if we do not have codec-device-id
		WIOKit::getOSDataValue(pciDev, "device-id", dev);
	}
	DBGLOG("rad", "getHWInfo: original PID: 0x%04X, replaced PID: 0x%04X", org, dev);
	org = static_cast<uint16_t>(dev);
}
