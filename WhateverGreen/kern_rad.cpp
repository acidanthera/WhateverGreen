//
//  kern_rad.cpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>
#include <Library/LegacyIOService.h>

#include <Availability.h>
#include <IOKit/IOPlatformExpert.h>
#define protected public
#include <IOKit/graphics/IOFramebuffer.h>
#undef protected

#ifndef __MAC_10_13
#define fVramMap vramMap
#endif

#include "kern_rad.hpp"

static const char *kextRadeonX3000[]		{ "/System/Library/Extensions/AMDRadeonX3000.kext/Contents/MacOS/AMDRadeonX3000" };
static const char *kextRadeonX4000[]		{ "/System/Library/Extensions/AMDRadeonX4000.kext/Contents/MacOS/AMDRadeonX4000" };
static const char *kextRadeonX4100[]		{ "/System/Library/Extensions/AMDRadeonX4100.kext/Contents/MacOS/AMDRadeonX4100" };
static const char *kextRadeonX4150[]		{ "/System/Library/Extensions/AMDRadeonX4150.kext/Contents/MacOS/AMDRadeonX4150" };
static const char *kextRadeonX4200[]		{ "/System/Library/Extensions/AMDRadeonX4200.kext/Contents/MacOS/AMDRadeonX4200" };
static const char *kextRadeonX4250[]		{ "/System/Library/Extensions/AMDRadeonX4250.kext/Contents/MacOS/AMDRadeonX4250" };
static const char *kextRadeonX5000[]		{ "/System/Library/Extensions/AMDRadeonX5000.kext/Contents/MacOS/AMDRadeonX5000" };
static const char *kextIOGraphicsPath[]		{ "/System/Library/Extensions/IOGraphicsFamily.kext/IOGraphicsFamily" };
static const char *kextFramebuffer[]		{ "/System/Library/Extensions/AMDFramebuffer.kext/Contents/MacOS/AMDFramebuffer" };
static const char *kextLegacyFramebuffer[]	{ "/System/Library/Extensions/AMDLegacyFramebuffer.kext/Contents/MacOS/AMDLegacyFramebuffer" };
static const char *kextSupport[]			{ "/System/Library/Extensions/AMDSupport.kext/Contents/MacOS/AMDSupport" };
static const char *kextLegacySupport[]		{ "/System/Library/Extensions/AMDLegacySupport.kext/Contents/MacOS/AMDLegacySupport" };

static const char *kextRadeonX3000NewId {"com.apple.kext.AMDRadeonX3000"};
static const char *kextRadeonX4000NewId {"com.apple.kext.AMDRadeonX4000"};
static const char *kextRadeonX4100NewId {"com.apple.kext.AMDRadeonX4100"};
static const char *kextRadeonX4150NewId {"com.apple.kext.AMDRadeonX4150"};
static const char *kextRadeonX4200NewId {"com.apple.kext.AMDRadeonX4200"};
static const char *kextRadeonX4250NewId {"com.apple.kext.AMDRadeonX4250"};
static const char *kextRadeonX5000NewId {"com.apple.kext.AMDRadeonX5000"};

static const char *kextRadeonX3000OldId {"com.apple.AMDRadeonX3000"};
static const char *kextRadeonX4000OldId {"com.apple.AMDRadeonX4000"};

static KernelPatcher::KextInfo kextList[] {
	{ kextRadeonX3000NewId, kextRadeonX3000, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ kextRadeonX4000NewId, kextRadeonX4000, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ kextRadeonX4100NewId, kextRadeonX4100, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ kextRadeonX4150NewId, kextRadeonX4150, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ kextRadeonX4200NewId, kextRadeonX4200, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ kextRadeonX4250NewId, kextRadeonX4250, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ kextRadeonX5000NewId, kextRadeonX5000, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.iokit.IOGraphicsFamily", kextIOGraphicsPath, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.kext.AMDFramebuffer", kextFramebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.kext.AMDLegacyFramebuffer", kextLegacyFramebuffer, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.kext.AMDSupport", kextSupport, 1, {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.apple.kext.AMDLegacySupport", kextLegacySupport, 1, {}, {}, KernelPatcher::KextInfo::Unloaded }
};

enum KextIndex {
	KextRadeonX3000Index,
	KextRadeonX4000Index,
	KextRadeonX4100Index,
	KextRadeonX4150Index,
	KextRadeonX4200Index,
	KextRadeonX4250Index,
	KextRadeonX5000Index,
	KextIOGraphicsIndex,
	KextAMDFramebufferIndex,
	KextAMDLegacyFramebufferIndex,
	KextAMDSupportIndex,
	KextAMDLegacySupportIndex,
	KextMaxIndex
};

static_assert(KextMaxIndex == arrsize(kextList), "Make sure kextList indexes are synchronised!");

static size_t kextListSize {arrsize(kextList)};

/**
 *  Max framebuffer base functions per kext
 */
static constexpr size_t MaxGetFrameBufferProcs = 3;

/**
 *  Framebuffer base function names
 */
static const char *getFrameBufferProcNames[RAD::HardwareIndex::Total][MaxGetFrameBufferProcs] {
	{
		"__ZN15AMDR8xxHardware25getFrameBufferBaseAddressEv"
	},
	{
		"__ZN13AMDSIHardware25getFrameBufferBaseAddressEv",
		"__ZN13AMDCIHardware25getFrameBufferBaseAddressEv",
		"__ZN28AMDRadeonX4000_AMDVIHardware25getFrameBufferBaseAddressEv"
	},
	{
		"__ZN28AMDRadeonX4100_AMDVIHardware25getFrameBufferBaseAddressEv"
	},
	{
		"__ZN28AMDRadeonX4150_AMDVIHardware25getFrameBufferBaseAddressEv"
	},
	{
		"__ZN28AMDRadeonX4200_AMDVIHardware25getFrameBufferBaseAddressEv"
	},
	{
		"__ZN28AMDRadeonX4250_AMDVIHardware25getFrameBufferBaseAddressEv"
	},
	{
	}
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

/**
 * NOTE: Most of the variables are static for speed reasons,
 * since some of them are on hotpath.
 */

/**
 *  Original set property function
 */
static RAD::t_setProperty orgSetProperty;

/**
 *  Original get property function
 */
static RAD::t_getProperty orgGetProperty;

/**
 *  Original connector info functions
 */
static RAD::t_getConnectorsInfo orgGetConnectorsInfoV1;
static RAD::t_getConnectorsInfo orgGetConnectorsInfoV2;
static RAD::t_getConnectorsInfo orgLegacyGetConnectorsInfo;

/**
 *  Original get atom object table functions
 */
static RAD::t_getAtomObjectTableForType orgGetAtomObjectTableForType;
static RAD::t_getAtomObjectTableForType orgLegacyGetAtomObjectTableForType;

/**
 *  Original translate atom connector into modern connector function
 */
static RAD::t_translateAtomConnectorInfo orgTranslateAtomConnectorInfoV1;
static RAD::t_translateAtomConnectorInfo orgTranslateAtomConnectorInfoV2;

/**
 *  Original controller start functions
 */
static RAD::t_controllerStart orgATIControllerStart;
static RAD::t_controllerStart orgLegacyATIControllerStart;

/**
 *  Original get device type functions
 */
static RAD::t_getDeviceType orgGetDeviceTypeBaffin;
static RAD::t_getDeviceType orgGetDeviceTypeEllesmere;

/**
 *  Verbose boot global variable pointer
 */
static uint8_t *gIOFBVerboseBootPtr {nullptr};

/**
 *  Current controller property provider
 */
static IOService *currentPropProvider {nullptr};
static IOService *currentLegacyPropProvider {nullptr};

/**
 *  Original framebuffer initialisation function
 */
static RAD::t_framebufferInit orgFramebufferInit {nullptr};

/**
 *  Autocorrect DVI
 */
static bool autocorrectDVI {false};

/**
 *  Original populateAccelConfig functions
 */
static RAD::t_populateAccelConfig orgPopulateAccelConfig[RAD::HardwareIndex::Total] {};

/**
 *  Signals vinfo availability
 */
static bool gotVideoInfo {false};

/**
 *  Loaded vinfo with consone parameters
 */
static RAD::vc_info consoleVinfo {};

/**
 *  populateAccelConfig wrapping functions
 */
template <size_t Index>
void populdateAccelConfig(IOService *accelService, const char **accelConfig) {
	if (orgPopulateAccelConfig[Index]) {
		orgPopulateAccelConfig[Index](accelService, accelConfig);
		RAD::updateAccelConfig(accelService, accelConfig);
	} else {
		SYSLOG("rad", "populdateAccelConfig invalid use for %lu", Index);
	}
}

/**
 *  Wrapped populateAccelConfig functions
 */
static RAD::t_populateAccelConfig wrapPopulateAccelConfig[RAD::HardwareIndex::Total] {
	populdateAccelConfig<RAD::HardwareIndex::X3000>,
	populdateAccelConfig<RAD::HardwareIndex::X4000>,
	populdateAccelConfig<RAD::HardwareIndex::X4100>,
	populdateAccelConfig<RAD::HardwareIndex::X4150>,
	populdateAccelConfig<RAD::HardwareIndex::X4200>,
	populdateAccelConfig<RAD::HardwareIndex::X4250>,
	populdateAccelConfig<RAD::HardwareIndex::X5000>
};

/**
 *  Register read function names
 */
static const char *populateAccelConfigProcNames[RAD::HardwareIndex::Total] {
	"__ZN37AMDRadeonX3000_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
	"__ZN37AMDRadeonX4000_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
	"__ZN37AMDRadeonX4100_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
	"__ZN37AMDRadeonX4150_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
	"__ZN37AMDRadeonX4200_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
	"__ZN37AMDRadeonX4250_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
	"__ZN37AMDRadeonX5000_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig"
};

bool RAD::init() {
	osCompat();
	
	char tmp[16];
	bool force24Bits = false;
	if (getKernelVersion() < KernelVersion::Sierra ||
		(static_cast<void>(force24Bits = PE_parse_boot_argn("-rad24", tmp, sizeof(tmp))), !force24Bits)) {
		progressState |= ProcessingState::BlinkingHell;
		kextList[HardwareIndex::Total+1].pathNum = kextList[HardwareIndex::Total+2].pathNum = 0;
	}

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
	
	autocorrectDVI = PE_parse_boot_argn("-raddvi", tmp, sizeof(tmp));
	
	DBGLOG("rad", "init 24-bits %d pg mask %d dvi %d progress %d", force24Bits, powerGatingMask, autocorrectDVI, progressState);
	
	LiluAPI::Error error = lilu.onKextLoad(kextList, kextListSize,
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		static_cast<RAD *>(user)->processKext(patcher, index, address, size);
	}, this);
	
	if (error != LiluAPI::Error::NoError) {
		SYSLOG("rad", "failed to register onKextLoad method %d", error);
		return false;
	}
	
	error = lilu.onPatcherLoad([](void *user, KernelPatcher &patcher) {
		static_cast<RAD *>(user)->processScreenFlicker(patcher);
		static_cast<RAD *>(user)->processProperties(patcher);
	}, this);
	
	if (error != LiluAPI::Error::NoError) {
		SYSLOG("rad", "failed to register onPatcherLoad method %d", error);
		return false;
	}
	
	return true;
}

void RAD::osCompat() {
	if (getKernelVersion() < KernelVersion::HighSierra) {
		// Versions before 10.13 do not support X4250 and X5000
		progressState |= ProcessingState::X4250Hardware | ProcessingState::X5000Hardware;
		kextList[HardwareIndex::X4250].pathNum = kextList[HardwareIndex::X5000].pathNum = 0;
	
		// Versions before 10.13 have legacy X3000 and X4000 IDs
		kextList[HardwareIndex::X3000].id = kextRadeonX3000OldId;
		kextList[HardwareIndex::X4000].id = kextRadeonX4000OldId;
		
		if (getKernelVersion() == KernelVersion::Sierra && getKernelMinorVersion() < 7) {
			// Versions before 10.12.6 do not support X4150, X4200
			progressState |= ProcessingState::X4150Hardware | ProcessingState::X4200Hardware;
			kextList[HardwareIndex::X4150].pathNum = kextList[HardwareIndex::X4200].pathNum = 0;
		} else if (getKernelVersion() < KernelVersion::Sierra) {
			// Versions before 10.12 do not support X4100, X4150, X4200
			progressState |= ProcessingState::X4100Hardware | ProcessingState::X4150Hardware | ProcessingState::X4200Hardware;
			kextList[HardwareIndex::X4100].pathNum = kextList[HardwareIndex::X4150].pathNum = kextList[HardwareIndex::X4200].pathNum = 0;
		}
	} else if (getKernelVersion() > KernelVersion::HighSierra ||
			   (getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() >= 5)) {
		// Versions after 10.13.4 do not support X4100~X4250
		progressState |= ProcessingState::X4100Hardware | ProcessingState::X4150Hardware | ProcessingState::X4200Hardware | ProcessingState::X4250Hardware;
		kextList[HardwareIndex::X4100].pathNum = kextList[HardwareIndex::X4150].pathNum = kextList[HardwareIndex::X4200].pathNum = kextList[HardwareIndex::X4250].pathNum = 0;
		// They also do not need black screen patches
		for (size_t i = 0; i < MaxGetFrameBufferProcs; i++)
			getFrameBufferProcNames[RAD::HardwareIndex::X4200][i] = nullptr;
	}
}

void RAD::wrapFramebufferInit(void *that) {
	if (gIOFBVerboseBootPtr && orgFramebufferInit) {
		uint8_t verboseBoot = *gIOFBVerboseBootPtr;
		// For back copy we need a console buffer and no verbose
		bool zeroFill = gotVideoInfo && !verboseBoot;

		// Now check if the resolution and parameters match
		auto fb = static_cast<IOFramebuffer *>(that);
		if (zeroFill) {
			IODisplayModeID mode;
			IOIndex depth;
			IOPixelInformation pixelInfo;

			if (fb->getCurrentDisplayMode(&mode, &depth) == kIOReturnSuccess &&
				fb->getPixelInformation(mode, depth, kIOFBSystemAperture, &pixelInfo) == kIOReturnSuccess) {
				DBGLOG("rad", "fb info 1: %d:%d %d:%d:%d",
					   mode, depth, pixelInfo.bytesPerRow, pixelInfo.bytesPerPlane, pixelInfo.bitsPerPixel);
				DBGLOG("rad", "fb info 2: %d:%d %s %d:%d:%d",
					   pixelInfo.componentCount, pixelInfo.bitsPerComponent, pixelInfo.pixelFormat, pixelInfo.flags, pixelInfo.activeWidth, pixelInfo.activeHeight);

				if (consoleVinfo.v_rowbytes != pixelInfo.bytesPerRow || consoleVinfo.v_width != pixelInfo.activeWidth ||
					consoleVinfo.v_height != pixelInfo.activeHeight || consoleVinfo.v_depth != pixelInfo.bitsPerPixel) {
					zeroFill = false;
					DBGLOG("rad", "this display has different mode");
				}
			} else {
				DBGLOG("rad", "failed to obtain display mode");
				zeroFill = false;
			}
		}

		*gIOFBVerboseBootPtr = 1;
		orgFramebufferInit(that);
		*gIOFBVerboseBootPtr = verboseBoot;

		if (zeroFill && fb->fVramMap) {
			auto dst = reinterpret_cast<uint8_t *>(fb->fVramMap->getVirtualAddress());
			DBGLOG("rad", "doing zero-fill...");
			memset(dst, 0, consoleVinfo.v_rowbytes * consoleVinfo.v_height);
		}
	}
}

bool RAD::wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned intlength) {
	if (intlength > 10 && aKey && reinterpret_cast<const uint32_t *>(aKey)[0] == 'edom' && reinterpret_cast<const uint16_t *>(aKey)[2] == 'l') {
		DBGLOG("rad", "SetProperty caught model %d (%.*s)", intlength, intlength, static_cast<char *>(bytes));
		if (*static_cast<uint32_t *>(bytes) == ' DMA' || *static_cast<uint32_t *>(bytes) == ' ITA') {
			if (that->getProperty(aKey)) {
				DBGLOG("rad", "SetProperty ignored setting %s to %s", aKey, static_cast<char *>(bytes));
				return true;
			} else {
				uint32_t ven, dev, rev, subven, sub;
				if (WIOKit::getOSDataValue(that, "vendor-id", ven) &&
					WIOKit::getOSDataValue(that, "device-id", dev) &&
					WIOKit::getOSDataValue(that, "revision-id", rev) &&
					WIOKit::getOSDataValue(that, "subsystem-vendor-id", subven) &&
					WIOKit::getOSDataValue(that, "subsystem-id", sub)) {
					
					DBGLOG("rad", "SetProperty autodetect found for %04X:%04X:%04X %04X:%04X", ven, dev, rev, subven, sub);
					const char *name = getModel(ven, dev, rev, subven, sub);
					if (name) {
						DBGLOG("rad", "SetProperty autodetect found %s", name);
						return orgSetProperty(that, aKey, const_cast<char *>(name), static_cast<unsigned>(strlen(name)+1));
					}
					
				} else {
					DBGLOG("rad", "SetProperty invalid device identification properties");
				}
			}
			DBGLOG("rad", "SetProperty missing %s, fallback to %s", aKey, static_cast<char *>(bytes));
		}
	}
	
	return orgSetProperty(that, aKey, bytes, intlength);
}

OSObject *RAD::wrapGetProperty(IORegistryEntry *that, const char *aKey) {
	auto obj = orgGetProperty(that, aKey);
	auto props = OSDynamicCast(OSDictionary, obj);

	if (props && aKey) {
		const char *prefix {nullptr};
		auto provider = currentLegacyPropProvider ? currentLegacyPropProvider : currentPropProvider;
		if (provider) {
			if (aKey[0] == 'a') {
				if (!strcmp(aKey, "aty_config"))
					prefix = "CFG,";
				else if (!strcmp(aKey, "aty_properties"))
					prefix = "PP,";
			}
		} else if (aKey[0] == 'c' && !strcmp(aKey, "cail_properties")) {
			provider = OSDynamicCast(IOService, that->getParentEntry(gIOServicePlane));
			DBGLOG("rad", "GetProperty got cail_properties %d, merging from %s", provider != nullptr,
				   provider ? safeString(provider->getName()) : "(null provider)");
			if (provider) prefix = "CAIL,";
		}
		
		if (prefix) {
			DBGLOG("rad", "GetProperty discovered property merge request for %s", aKey);
			auto newProps = OSDynamicCast(OSDictionary, props->copyCollection());
			mergeProperties(newProps, prefix, provider);
			that->setProperty(aKey, newProps);
			obj = newProps;
		}
	}

	return obj;
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
					if (prop) {
						// It is hard to make a boolean from ACPI, so we make a hack here:
						// 1-byte OSData with 0x01 / 0x00 values becomes boolean.
						auto data = OSDynamicCast(OSData, prop);
						if (data && data->getLength() == 1) {
							auto val = static_cast<const uint8_t *>(data->getBytesNoCopy());
							if (val && val[0] == 1) {
								props->setObject(name+prefixlen, kOSBooleanTrue);
								DBGLOG("rad", "prop %s was merged as kOSBooleanTrue", name);
								continue;
							} else if (val && val[0] == 0) {
								props->setObject(name+prefixlen, kOSBooleanFalse);
								DBGLOG("rad", "prop %s was merged as kOSBooleanFalse", name);
								continue;
							}
						}
						
						props->setObject(name+prefixlen, prop);
						DBGLOG("rad", "prop %s was merged", name);
					} else {
						DBGLOG("rad", "prop %s was not merged due to no value", name);
					}
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
				props->setObject(powerGatingFlags[i], OSNumber::withNumber(1, 32));
			}
		}
	}
}

void RAD::applyPropertyFixes(IOService *service, uint32_t connectorNum) {
	if (service && getKernelVersion() >= KernelVersion::HighSierra) {
		// Starting with 10.13.2 this is important to fix sleep issues due to enforced 6 screens
		if (!service->getProperty("CFG,CFG_FB_LIMIT")) {
			DBGLOG("rad", "setting fb limit to %d", connectorNum);
			service->setProperty("CFG_FB_LIMIT", OSNumber::withNumber(connectorNum, 32));
		}

		// This property may have an effect on 5K screens, but it is known to constantly break
		// DP and HDMI outputs on various GPUs at least in 10.13.
		if (!service->getProperty("CFG,CFG_USE_AGDC")) {
			DBGLOG("rad", "disabling agdc");
			service->setProperty("CFG_USE_AGDC", OSBoolean::withBoolean(false));
		}
	}
}

void RAD::updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl, RADConnectors::Connector *connectors, uint8_t *sz) {
	if (atomutils) {
		DBGLOG("rad", "getConnectorsInfo found %d connectors", *sz);
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
				DBGLOG("rad", "getConnectorsInfo got size override to %d", *sz);
			}

			if (consPtr && consSize > 0 && *sz > 0 && RADConnectors::valid(consSize, *sz)) {
				RADConnectors::copy(connectors, *sz, static_cast<const RADConnectors::Connector *>(consPtr), consSize);
				DBGLOG("rad", "getConnectorsInfo installed %d connectors", *sz);
				applyPropertyFixes(ctrl, consSize);
			} else {
				DBGLOG("rad", "getConnectorsInfo conoverrides have invalid size %d for %d num", consSize, *sz);
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
				DBGLOG("rad", "getConnectorsInfo found different displaypaths %d and connectors %d", displayPathNum, connectorObjectNum);
		}

		applyPropertyFixes(ctrl, *sz);

		// Prioritise connectors, since it may cause black screen on e.g. R9 370
		const uint8_t *senseList = nullptr;
		uint8_t senseNum = 0;
		auto priData = OSDynamicCast(OSData, ctrl->getProperty("connector-priority"));
		if (priData) {
			senseList = static_cast<const uint8_t *>(priData->getBytesNoCopy());
			senseNum = static_cast<uint8_t>(priData->getLength());
			DBGLOG("rad", "getConnectorInfo found %d senses in connector-priority", senseNum);
			reprioritiseConnectors(senseList, senseNum, connectors, *sz);
		} else {
			DBGLOG("rad", "getConnectorInfo leaving unchaged priority");
		}
	}

	DBGLOG("rad", "getConnectorsInfo resulting %d connectors follow", *sz);
	RADConnectors::print(connectors, *sz);
}

void RAD::autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum, AtomConnectorObject *connectorObjects,
								uint8_t connectorObjectNum, RADConnectors::Connector *connectors, uint8_t sz) {
	for (uint8_t i = 0; i < displayPathNum; i++) {
		if (!isEncoder(displayPaths[i].usGraphicObjIds)) {
			DBGLOG("rad", "autocorrectConnectors not encoder %X at %d", displayPaths[i].usGraphicObjIds, i);
			continue;
		}
		
		uint8_t txmit = 0, enc = 0;
		if (!getTxEnc(displayPaths[i].usGraphicObjIds, txmit, enc))
			continue;
		
		uint8_t sense = getSenseID(baseAddr + connectorObjects[i].usRecordOffset);
		if (!sense) {
			DBGLOG("rad", "autocorrectConnectors failed to detect sense for %d connector", i);
			continue;
		}
		
		DBGLOG("rad", "autocorrectConnectors found txmit %02X enc %02X sense %02X for %d connector", txmit, enc, sense, i);

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
	
	if (autocorrectDVI) {
		if (connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I &&
			connector != CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D &&
			connector != CONNECTOR_OBJECT_ID_LVDS) {
			DBGLOG("rad", "autocorrectConnector found unsupported connector type %02X", connector);
			return;
		}
		
		auto fixTransmit = [](auto &con, uint8_t idx, uint8_t sense, uint8_t txmit) {
			if (con.sense == sense) {
				if (con.transmitter != txmit && (con.transmitter & 0xCF) == con.transmitter) {
					DBGLOG("rad", "autocorrectConnector replacing txmit %02X with %02X for %d connector sense %02X",
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
						DBGLOG("rad", "reprioritiseConnectors setting priority of sense %02X to %d by sense", con.sense, priCount);
						con.priority = priCount++;
						return true;
					}
				} else {
					if (con.priority == 0 && con.type == typeList[i-senseNum]) {
						DBGLOG("rad", "reprioritiseConnectors setting priority of sense %02X to %d by type", con.sense, priCount);
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

static char kextVersion[] {
#ifdef DEBUG
	'W', 'E', 'A', 'D', '-',
#else
	'W', 'E', 'A', 'R', '-',
#endif
	xStringify(MODULE_VERSION)[0], xStringify(MODULE_VERSION)[2], xStringify(MODULE_VERSION)[4], '-',
	getBuildYear<0>(), getBuildYear<1>(), getBuildYear<2>(), getBuildYear<3>(), '-',
	getBuildMonth<0>(), getBuildMonth<1>(), '-', getBuildDay<0>(), getBuildDay<1>(), '\0'
};

void RAD::reportVersion(IOService *controller) {
	auto orig = OSDynamicCast(OSData, controller->getProperty("ATY,EFIVersion"));
	char efistate = 'A';
	
	if (orig) {
		auto data = static_cast<const char *>(orig->getBytesNoCopy());
		if (data && orig->getLength() >= 2) {
			if (data[0] != 'W' || data[1] != 'E')
				efistate = 'F';
		} else {
			efistate = 'B';
		}
	}
	
	kextVersion[2] = efistate;
	DBGLOG("rad", "setting efiversion to %s", kextVersion);
	controller->setProperty("ATY,EFIVersion", const_cast<char *>(kextVersion), sizeof(kextVersion));
}

uint32_t RAD::wrapGetConnectorsInfoV1(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
	uint32_t code = orgGetConnectorsInfoV1(that, connectors, sz);
	
	if (code == 0 && sz && currentPropProvider) {
		if (getKernelVersion() >= KernelVersion::HighSierra)
			updateConnectorsInfo(nullptr, nullptr, currentPropProvider, connectors, sz);
		else
			updateConnectorsInfo(static_cast<void **>(that)[1], orgGetAtomObjectTableForType, currentPropProvider, connectors, sz);
	} else {
		DBGLOG("rad", "getConnectorsInfoV1 failed %X or undefined %d", code, currentPropProvider == nullptr);
	}
	
	return code;
}

uint32_t RAD::wrapGetConnectorsInfoV2(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
	uint32_t code = orgGetConnectorsInfoV2(that, connectors, sz);
	
	if (code == 0 && sz && currentPropProvider)
		updateConnectorsInfo(nullptr, nullptr, currentPropProvider, connectors, sz);
	else
		DBGLOG("rad", "getConnectorsInfoV2 failed %X or undefined %d", code, currentPropProvider == nullptr);
	
	return code;
}

uint32_t RAD::wrapLegacyGetConnectorsInfo(void *that, RADConnectors::Connector *connectors, uint8_t *sz) {
	uint32_t code = orgLegacyGetConnectorsInfo(that, connectors, sz);
	
	if (code == 0 && sz && currentLegacyPropProvider)
		updateConnectorsInfo(static_cast<void **>(that)[1], orgLegacyGetAtomObjectTableForType, currentLegacyPropProvider, connectors, sz);
	else
		DBGLOG("rad", "legacy getConnectorsInfo failed %X or undefined %d", code, currentLegacyPropProvider == nullptr);
	
	return code;
}

uint32_t RAD::wrapTranslateAtomConnectorInfoV1(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector) {
	uint32_t code = orgTranslateAtomConnectorInfoV1(that, info, connector);
	
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
						autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord), txmit, enc, connector, 1);
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
	uint32_t code = orgTranslateAtomConnectorInfoV2(that, info, connector);
	
	if (code == 0 && info && connector) {
		RADConnectors::print(connector, 1);
		
		uint8_t sense = getSenseID(info->i2cRecord);
		if (sense) {
			DBGLOG("rad", "translateAtomConnectorInfoV2 got sense id %02X", sense);
			uint8_t txmit = 0, enc = 0;
			if (getTxEnc(info->usGraphicObjIds, txmit, enc))
				autocorrectConnector(getConnectorID(info->usConnObjectId), getSenseID(info->i2cRecord), txmit, enc, connector, 1);
		} else {
			DBGLOG("rad", "translateAtomConnectorInfoV2 failed to detect sense for translated connector");
		}
	}
	
	return code;
}

bool RAD::wrapATIControllerStart(IOService *ctrl, IOService *provider) {
	DBGLOG("rad", "starting controller");
	char tmp[16];
	if (PE_parse_boot_argn("-radvesa", tmp, sizeof(tmp))) {
		DBGLOG("rad", "disabling video acceleration on request");
		return false;
	}
	
	currentPropProvider = provider;
	reportVersion(provider);
	bool r = orgATIControllerStart(ctrl, provider);
	currentPropProvider = nullptr;
	DBGLOG("rad", "starting controller done %d", r);
	return r;
}

bool RAD::wrapLegacyATIControllerStart(IOService *ctrl, IOService *provider) {
	DBGLOG("rad", "starting legacy controller");
	char tmp[16];
	if (PE_parse_boot_argn("-radvesa", tmp, sizeof(tmp))) {
		DBGLOG("rad", "disabling legacy video acceleration on request");
		return false;
	}
	
	currentLegacyPropProvider = provider;
	reportVersion(provider);
	bool r = orgLegacyATIControllerStart(ctrl, provider);
	currentLegacyPropProvider = nullptr;
	DBGLOG("rad", "starting legacy controller done %d", r);
	return r;
}

uint32_t RAD::wrapGetDeviceTypeBaffin(IOService *accelService, void *pcidev) {
	char tmp[4];
	if (accelService->getProperty("prefer-4200-driver") || PE_parse_boot_argn("-rad4200", tmp, sizeof(tmp))) {
		uint32_t ven, dev, rev;

		if (WIOKit::getOSDataValue(accelService, "vendor-id", ven) &&
			WIOKit::getOSDataValue(accelService, "device-id", dev) &&
			WIOKit::getOSDataValue(accelService, "revision-id", rev)) {
			DBGLOG("rad", "getdevtype b got %04x:%04X:%04X device", ven, dev, rev);

			uint32_t type = 0;

			// Class code does not affect anything.

			// DeviceListBaffin<1002h, 67FFh, 0C0h, 106Bh, 164h, 0, 0Ah> # Radeon Pro 465
			// DeviceListBaffin<1002h, 67EFh, 0C0h, 106Bh, 16Ah, 0, 0Bh> # Radeon Pro 460/560 (460?)
			// DeviceListBaffin<1002h, 67EFh, 0C7h, 106Bh, 16Bh, 0, 3>   # Radeon Pro 455/555 (455?)
			// DeviceListBaffin<1002h, 67EFh, 0EFh, 106Bh, 16Ch, 0, 2>   # Radeon Pro 450/550 (450?)

			if ((ven == 0x1002 && dev == 0x67FF) ||
				(ven == 0x1002 && dev == 0x67EF)) {
				type = 0x9;
			}

			if (type > 0) {
				DBGLOG("rad", "getdevtype b chose 0x%x type", type);
				return type;
			}
		}
	} else {
		DBGLOG("rad", "getdevtype b asked not to perform vendor unlock on polaris");
	}

	return orgGetDeviceTypeBaffin(accelService, pcidev);
}

uint32_t RAD::wrapGetDeviceTypeEllesmere(IOService *accelService, void *pcidev) {
	char tmp[4];
	if (accelService->getProperty("prefer-4200-driver") || PE_parse_boot_argn("-rad4200", tmp, sizeof(tmp))) {
		uint32_t ven, dev, rev;

		if (WIOKit::getOSDataValue(accelService, "vendor-id", ven) &&
			WIOKit::getOSDataValue(accelService, "device-id", dev) &&
			WIOKit::getOSDataValue(accelService, "revision-id", rev)) {
			DBGLOG("rad", "getdevtype e got %04x:%04X:%04X device", ven, dev, rev);

			uint32_t type = 0;

			// DeviceListEllesmere <1002h, 67DFh, 0, 0C0h, 0, 0, 106Bh, 161h, 9>   # Radeon Pro 480
			// DeviceListEllesmere <1002h, 67DFh, 0, 0C4h, 0, 0, 106Bh, 162h, 8>   # Radeon Pro 480/575 (480?)
			// DeviceListEllesmere <1002h, 67DFh, 0, 0C5h, 0, 0, 106Bh, 163h, 0Ch> # Radeon Pro 470/570 (470?)

			if (ven == 0x1002 && dev == 0x67DF) {
				type = 0x9;
			}

			if (type > 0) {
				DBGLOG("rad", "getdevtype e chose 0x%x type", type);
				return type;
			}
		}
	} else {
		DBGLOG("rad", "getdevtype e asked not to perform vendor unlock on polaris");
	}

	return orgGetDeviceTypeEllesmere(accelService, pcidev);
}

void RAD::updateAccelConfig(IOService *accelService, const char **accelConfig) {
	if (accelService && accelConfig) {
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
}

void RAD::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (progressState != ProcessingState::EverythingDone) {
		for (size_t i = 0; i < kextListSize; i++) {
			if (kextList[i].loadIndex == index) {
				DBGLOG("rad", "current kext is %s progressState %d", kextList[i].id, progressState);

				bool newFB = !(progressState & ProcessingState::FramebufferNew) && i == KextAMDFramebufferIndex;
				if (newFB || (!(progressState & ProcessingState::FramebufferLegacy) && i == KextAMDLegacyFramebufferIndex)) {
					auto bitsPerComponent = patcher.solveSymbol<int *>(index, "__ZL18BITS_PER_COMPONENT", address, size);
					if (bitsPerComponent) {
						while (bitsPerComponent && *bitsPerComponent) {
							if (*bitsPerComponent == 10) {
								auto ret = MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
								if (ret == KERN_SUCCESS) {
									DBGLOG("rad", "fixing __ZL18BITS_PER_COMPONENT");
									*bitsPerComponent = 8;
									MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
								} else {
									SYSLOG("rad", "failed to disable write protection for __ZL18BITS_PER_COMPONENT");
								}
							}
							bitsPerComponent++;
						}
					} else {
						SYSLOG("rad", "failed to find __ZL18BITS_PER_COMPONENT");
					}

					DBGLOG("rad", "fixing pixel types");
					
					KernelPatcher::LookupPatch pixelPatch {
						&kextList[i],
						reinterpret_cast<const uint8_t *>("--RRRRRRRRRRGGGGGGGGGGBBBBBBBBBB"),
						reinterpret_cast<const uint8_t *>("--------RRRRRRRRGGGGGGGGBBBBBBBB"),
						32, 2
					};

					patcher.applyLookupPatch(&pixelPatch);

					progressState |= newFB ? ProcessingState::FramebufferNew : ProcessingState::FramebufferLegacy;
				} else if (!(progressState & ProcessingState::BootLogo) && i == KextIOGraphicsIndex) {
					gIOFBVerboseBootPtr = patcher.solveSymbol<uint8_t *>(index, "__ZL16gIOFBVerboseBoot", address, size);
					if (gIOFBVerboseBootPtr) {
						DBGLOG("rad", "obtained __ZL16gIOFBVerboseBoot");
						auto ioFramebufferinit = patcher.solveSymbol(index, "__ZN13IOFramebuffer6initFBEv", address, size);
						if (ioFramebufferinit) {
							DBGLOG("rad", "obtained __ZN13IOFramebuffer6initFBEv");
							orgFramebufferInit = reinterpret_cast<t_framebufferInit>(patcher.routeFunction(ioFramebufferinit, reinterpret_cast<mach_vm_address_t>(wrapFramebufferInit), true));
							if (patcher.getError() == KernelPatcher::Error::NoError) {
								DBGLOG("rad", "routed __ZN13IOFramebuffer6initFBEv");
							} else {
								SYSLOG("rad", "failed to route __ZN13IOFramebuffer6initFBEv");
							}
						}
					} else {
						SYSLOG("rad", "failed to resolve __ZL16gIOFBVerboseBoot");
					}

					progressState |= ProcessingState::BootLogo;
				} else if (!(progressState & ProcessingState::PatchConnectors) && i == KextAMDSupportIndex) {
					if (getKernelVersion() >= KernelVersion::HighSierra) {
						auto getConnectorsInfoV1 = patcher.solveSymbol(index, "__ZN14AtiBiosParser116getConnectorInfoEP13ConnectorInfoRh", address, size);
						if (getConnectorsInfoV1) {
							orgGetConnectorsInfoV1 = reinterpret_cast<t_getConnectorsInfo>(patcher.routeFunction(getConnectorsInfoV1, reinterpret_cast<mach_vm_address_t>(wrapGetConnectorsInfoV1), true));
							if (patcher.getError() == KernelPatcher::Error::NoError) {
								DBGLOG("rad", "routed __ZN14AtiBiosParser116getConnectorInfoEP13ConnectorInfoRh");
							} else {
								SYSLOG("rad", "failed to route __ZN14AtiBiosParser116getConnectorInfoEP13ConnectorInfoRh");
							}
						} else {
							SYSLOG("rad", "failed to find __ZN14AtiBiosParser116getConnectorInfoEP13ConnectorInfoRh");
						}

						auto getConnectorsInfoV2 = patcher.solveSymbol(index, "__ZN14AtiBiosParser216getConnectorInfoEP13ConnectorInfoRh", address, size);
						if (getConnectorsInfoV2) {
							orgGetConnectorsInfoV2 = reinterpret_cast<t_getConnectorsInfo>(patcher.routeFunction(getConnectorsInfoV2, reinterpret_cast<mach_vm_address_t>(wrapGetConnectorsInfoV2), true));
							if (patcher.getError() == KernelPatcher::Error::NoError) {
								DBGLOG("rad", "routed __ZN14AtiBiosParser216getConnectorInfoEP13ConnectorInfoRh");
							} else {
								SYSLOG("rad", "failed to route __ZN14AtiBiosParser216getConnectorInfoEP13ConnectorInfoRh");
							}
						} else {
							SYSLOG("rad", "failed to find __ZN14AtiBiosParser216getConnectorInfoEP13ConnectorInfoRh");
						}

						auto translateAtomConnectorInfoV1 = patcher.solveSymbol(index, "__ZN14AtiBiosParser126translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V117AtomConnectorInfoER13ConnectorInfo", address, size);
						if (translateAtomConnectorInfoV1) {
							orgTranslateAtomConnectorInfoV1 = reinterpret_cast<t_translateAtomConnectorInfo>(patcher.routeFunction(translateAtomConnectorInfoV1,
								reinterpret_cast<mach_vm_address_t>(wrapTranslateAtomConnectorInfoV1), true));
							if (patcher.getError() == KernelPatcher::Error::NoError) {
								DBGLOG("rad", "routed __ZN14AtiBiosParser126translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V117AtomConnectorInfoER13ConnectorInfo");
							} else {
								SYSLOG("rad", "failed to route __ZN14AtiBiosParser126translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V117AtomConnectorInfoER13ConnectorInfo");
							}
						} else {
							SYSLOG("rad", "failed to find __ZN14AtiBiosParser126translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V117AtomConnectorInfoER13ConnectorInfo");
						}

						auto translateAtomConnectorInfoV2 = patcher.solveSymbol(index, "__ZN14AtiBiosParser226translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V217AtomConnectorInfoER13ConnectorInfo", address, size);
						if (translateAtomConnectorInfoV2) {
							orgTranslateAtomConnectorInfoV2 = reinterpret_cast<t_translateAtomConnectorInfo>(patcher.routeFunction(translateAtomConnectorInfoV2,
								reinterpret_cast<mach_vm_address_t>(wrapTranslateAtomConnectorInfoV2), true));
							if (patcher.getError() == KernelPatcher::Error::NoError) {
								DBGLOG("rad", "routed __ZN14AtiBiosParser226translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V217AtomConnectorInfoER13ConnectorInfo");
							} else {
								SYSLOG("rad", "failed to route __ZN14AtiBiosParser226translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V217AtomConnectorInfoER13ConnectorInfo");
							}
						} else {
							SYSLOG("rad", "failed to find __ZN14AtiBiosParser226translateAtomConnectorInfoERN30AtiObjectInfoTableInterface_V217AtomConnectorInfoER13ConnectorInfo");
						}
					} else {
						auto getConnectorsInfoV1 = patcher.solveSymbol(index, "__ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh", address, size);
						if (getConnectorsInfoV1) {
							orgGetConnectorsInfoV1 = reinterpret_cast<t_getConnectorsInfo>(patcher.routeFunction(getConnectorsInfoV1, reinterpret_cast<mach_vm_address_t>(wrapGetConnectorsInfoV1), true));
							if (patcher.getError() == KernelPatcher::Error::NoError) {
								DBGLOG("rad", "routed __ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh");
							} else {
								SYSLOG("rad", "failed to route __ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh");
							}
						} else {
							SYSLOG("rad", "failed to find __ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh");
						}

						orgGetAtomObjectTableForType = reinterpret_cast<t_getAtomObjectTableForType>(patcher.solveSymbol(index, "__ZN20AtiAtomBiosUtilities25getAtomObjectTableForTypeEhRh", address, size));
						if (!orgGetAtomObjectTableForType) {
							SYSLOG("rad", "failed to find __ZN20AtiAtomBiosUtilities25getAtomObjectTableForTypeEhRh");
						}
					}

					auto controllerStart = patcher.solveSymbol(index, "__ZN13ATIController5startEP9IOService", address, size);
					if (controllerStart) {
						orgATIControllerStart = reinterpret_cast<t_controllerStart>(patcher.routeFunction(controllerStart, reinterpret_cast<mach_vm_address_t>(wrapATIControllerStart), true));
						if (patcher.getError() == KernelPatcher::Error::NoError) {
							DBGLOG("rad", "routed __ZN13ATIController5startEP9IOService");
						} else {
							SYSLOG("rad", "failed to route __ZN13ATIController5startEP9IOService");
						}
					} else {
						SYSLOG("rad", "failed to find __ZN13ATIController5startEP9IOService");
					}

					progressState |= ProcessingState::PatchConnectors;
				} else if (!(progressState & ProcessingState::PatchLegacyConnectors) && i == KextAMDLegacySupportIndex) {
					auto getConnectorsInfo = patcher.solveSymbol(index, "__ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh", address, size);
					if (getConnectorsInfo) {
						orgLegacyGetConnectorsInfo = reinterpret_cast<t_getConnectorsInfo>(patcher.routeFunction(getConnectorsInfo, reinterpret_cast<mach_vm_address_t>(wrapLegacyGetConnectorsInfo), true));
						if (patcher.getError() == KernelPatcher::Error::NoError) {
							DBGLOG("rad", "routed __ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh");
						} else {
							SYSLOG("rad", "failed to route __ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh");
						}
					} else {
						SYSLOG("rad", "failed to find __ZN23AtiAtomBiosDceInterface17getConnectorsInfoEP13ConnectorInfoRh");
					}

					auto controllerStart = patcher.solveSymbol(index, "__ZN19AMDLegacyController5startEP9IOService", address, size);
					if (controllerStart) {
						orgLegacyATIControllerStart = reinterpret_cast<t_controllerStart>(patcher.routeFunction(controllerStart, reinterpret_cast<mach_vm_address_t>(wrapLegacyATIControllerStart), true));
						if (patcher.getError() == KernelPatcher::Error::NoError) {
							DBGLOG("rad", "routed __ZN19AMDLegacyController5startEP9IOService");
						} else {
							SYSLOG("rad", "failed to route __ZN19AMDLegacyController5startEP9IOService");
						}
					} else {
						SYSLOG("rad", "failed to find __ZN19AMDLegacyController5startEP9IOService");
					}

					orgLegacyGetAtomObjectTableForType = patcher.solveSymbol<t_getAtomObjectTableForType>(index, "__ZN20AtiAtomBiosUtilities25getAtomObjectTableForTypeEhRh", address, size);
					if (!orgLegacyGetAtomObjectTableForType) {
						SYSLOG("rad", "failed to find __ZN20AtiAtomBiosUtilities25getAtomObjectTableForTypeEhRh");
					}

					progressState |= ProcessingState::PatchLegacyConnectors;
				} else if ((progressState & ProcessingState::RegisterPatch) != ProcessingState::RegisterPatch) {
					for (uint32_t j = 0; j < HardwareIndex::Total; j++) {
						if (i == j && !(progressState & indexToMask(j))) {
							for (size_t k = 0; k < MaxGetFrameBufferProcs && getFrameBufferProcNames[j][k] != nullptr; k++) {
								auto getFB = patcher.solveSymbol(index, getFrameBufferProcNames[j][k], address, size);
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
									if (patcher.getError() != KernelPatcher::Error::NoError)
										SYSLOG("rad", "failed to patch %s", getFrameBufferProcNames[j][k]);
									else
										DBGLOG("rad", "patched %s", getFrameBufferProcNames[j][k]);
								} else {
									SYSLOG("rad", "failed to find %s", getFrameBufferProcNames[j][k]);
								}
							}

							// On 10.13~10.13.3(?) and newer X4250 driver is problematic for some GPUs
							if (j == HardwareIndex::X4200 && getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() < 5) {
								auto getDeviceType = patcher.solveSymbol(index, "__ZN43AMDRadeonX4200_AMDBaffinGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice", address, size);
								if (getDeviceType) {
									orgGetDeviceTypeBaffin = reinterpret_cast<t_getDeviceType>(patcher.routeFunction(getDeviceType, reinterpret_cast<mach_vm_address_t>(wrapGetDeviceTypeBaffin), true));
									if (patcher.getError() == KernelPatcher::Error::NoError) {
										DBGLOG("rad", "routed __ZN43AMDRadeonX4200_AMDBaffinGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice");
									} else {
										SYSLOG("rad", "failed to route __ZN43AMDRadeonX4200_AMDBaffinGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice");
									}
								} else {
									SYSLOG("rad", "failed to find __ZN43AMDRadeonX4200_AMDBaffinGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice");
								}

								getDeviceType = patcher.solveSymbol(index, "__ZN46AMDRadeonX4200_AMDEllesmereGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice", address, size);
								if (getDeviceType) {
									orgGetDeviceTypeEllesmere = reinterpret_cast<t_getDeviceType>(patcher.routeFunction(getDeviceType, reinterpret_cast<mach_vm_address_t>(wrapGetDeviceTypeEllesmere), true));
									if (patcher.getError() == KernelPatcher::Error::NoError) {
										DBGLOG("rad", "routed __ZN46AMDRadeonX4200_AMDEllesmereGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice");
									} else {
										SYSLOG("rad", "failed to route __ZN46AMDRadeonX4200_AMDEllesmereGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice");
									}
								} else {
									SYSLOG("rad", "failed to find __ZN46AMDRadeonX4200_AMDEllesmereGraphicsAccelerator13getDeviceTypeEP11IOPCIDevice");
								}
							}

							auto populate = patcher.solveSymbol(index, populateAccelConfigProcNames[j], address, size);
							if (populate) {
								orgPopulateAccelConfig[j] = reinterpret_cast<t_populateAccelConfig>(patcher.routeFunction(populate, reinterpret_cast<mach_vm_address_t>(wrapPopulateAccelConfig[j]), true));
								if (patcher.getError() == KernelPatcher::Error::NoError) {
									DBGLOG("rad", "routed %s", populateAccelConfigProcNames[j]);
								} else {
									SYSLOG("rad", "failed to route %s", populateAccelConfigProcNames[j]);
								}
							} else {
								SYSLOG("rad", "failed to find %s", populateAccelConfigProcNames[j]);
							}

							int tmp;
							if (PE_parse_boot_argn("-radgl", &tmp, sizeof(tmp))) {
								DBGLOG("rad", "disabling Metal support");
								uint8_t find1[] {0x4D, 0x65, 0x74, 0x61, 0x6C, 0x53, 0x74, 0x61};
								uint8_t find2[] {0x4D, 0x65, 0x74, 0x61, 0x6C, 0x50, 0x6C, 0x75};
								uint8_t repl1[] {0x50, 0x65, 0x74, 0x61, 0x6C, 0x53, 0x74, 0x61};
								uint8_t repl2[] {0x50, 0x65, 0x74, 0x61, 0x6C, 0x50, 0x6C, 0x75};

								KernelPatcher::LookupPatch antimetal[] {
									{&kextList[i], find1, repl1, sizeof(find1), 2},
									{&kextList[i], find2, repl2, sizeof(find1), 2}
								};

								for (auto &p : antimetal) {
									patcher.applyLookupPatch(&p);
									patcher.clearError();
								}
							}
							
							progressState |= indexToMask(j);
						}
					}
				}

				// Ignore all the errors for other processors
				patcher.clearError();
				break;
			}
		}
	}
}

void RAD::processProperties(KernelPatcher &patcher) {
	auto setProperty = patcher.solveSymbol(KernelPatcher::KernelID, "__ZN15IORegistryEntry11setPropertyEPKcPvj");
	if (setProperty) {
		orgSetProperty = reinterpret_cast<t_setProperty>(patcher.routeFunction(setProperty, reinterpret_cast<mach_vm_address_t>(wrapSetProperty), true));
		if (patcher.getError() == KernelPatcher::Error::NoError) {
			DBGLOG("rad", "routed __ZN15IORegistryEntry11setPropertyEPKcPvj");
		} else {
			SYSLOG("rad", "failed to route __ZN15IORegistryEntry11setPropertyEPKcPvj");
		}
	} else {
		SYSLOG("rad", "failed to find __ZN15IORegistryEntry11setPropertyEPKcPvj");
	}

	if (!(progressState & ProcessingState::DisablePowerGating)) {
		auto getProperty = patcher.solveSymbol(KernelPatcher::KernelID, "__ZNK15IORegistryEntry11getPropertyEPKc");
		if (getProperty) {
			orgGetProperty = reinterpret_cast<t_getProperty>(patcher.routeFunction(getProperty, reinterpret_cast<mach_vm_address_t>(wrapGetProperty), true));
			if (patcher.getError() == KernelPatcher::Error::NoError) {
				DBGLOG("rad", "routed __ZNK15IORegistryEntry11getPropertyEPKc");
			} else {
				SYSLOG("rad", "failed to route __ZNK15IORegistryEntry11getPropertyEPKc");
			}
		} else {
			SYSLOG("rad", "failed to find __ZNK15IORegistryEntry11getPropertyEPKc");
		}
	}

	// Ignore all the errors for other processors
	patcher.clearError();
}

void RAD::processScreenFlicker(KernelPatcher &patcher) {
	auto sect = WIOKit::findEntryByPrefix("/IOResources", "IntelGraphicsFixup", gIOServicePlane);
	if (sect) {
		DBGLOG("rad", "Discovered IntelGraphicsFixup, disabling custom screenfix");
		progressState |= ProcessingState::BootLogo;
	} else {
		auto info = reinterpret_cast<vc_info *>(patcher.solveSymbol(KernelPatcher::KernelID, "_vinfo"));
		if (info) {
			consoleVinfo = *info;
			DBGLOG("rad", "vinfo 1: %d:%d %d:%d:%d",
				   consoleVinfo.v_height, consoleVinfo.v_width, consoleVinfo.v_depth, consoleVinfo.v_rowbytes, consoleVinfo.v_type);
			DBGLOG("rad", "vinfo 2: %s %d:%d %d:%d:%d",
				   consoleVinfo.v_name, consoleVinfo.v_rows, consoleVinfo.v_columns, consoleVinfo.v_rowscanbytes, consoleVinfo.v_scale, consoleVinfo.v_rotate);
			gotVideoInfo = true;
		} else {
			SYSLOG("rad", "failed to obtain vcinfo");
		}

		// Ignore all the errors for other processors
		patcher.clearError();
	}
}
