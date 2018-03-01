//
//  kern_audio.cpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_iokit.hpp>
#include <Headers/plugin_start.hpp>

#include "kern_audio.hpp"
#include "kern_rad.hpp"

OSDefineMetaClassAndStructors(WhateverAudio, IOService)

uint32_t WhateverAudio::getAnalogLayout() {
	// For some DP monitors layout-id value should match HDEF layout-id
	// If we have HDEF properly configured, get the value
	static uint32_t layout = 0;

	if (!layout) {
		const char *tree[] {"AppleACPIPCI"};
		auto sect = WIOKit::findEntryByPrefix("/AppleACPIPlatformExpert", "PCI", gIOServicePlane);
		for (size_t i = 0; sect && i < arrsize(tree); i++) {
			sect = WIOKit::findEntryByPrefix(sect, tree[i], gIOServicePlane);
			if (sect && i+1 == arrsize(tree)) {
				auto iterator = sect->getChildIterator(gIOServicePlane);
				if (iterator) {
					IORegistryEntry *obj = nullptr;
					while ((obj = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) != nullptr) {
						uint32_t vendor = 0;
						// We can technically check the class-code too, but it is not required.
						// HDEF and HDAU should have the same layout-id if any, so it is irrelevant which we find.
						if (WIOKit::getOSDataValue(obj, "layout-id", layout) &&
							WIOKit::getOSDataValue(obj, "vendor-id", vendor) &&
							vendor == WIOKit::VendorID::Intel) {
							DBGLOG("audio", "found intel audio %s with layout-id %u", safeString(obj->getName()), layout);
							return layout;
						}
					}
					iterator->release();
				}
			}
		}

		DBGLOG("audio", "failed to find HDEF layout-id, falling back to 1");
		layout = 0x1;
	}

	return layout;
}

IOService *WhateverAudio::probe(IOService *hdaService, SInt32 *score) {
	if (!ADDPR(startSuccess))
		return nullptr;

	if (!hdaService) {
		DBGLOG("audio", "received null digitial audio device");
		return nullptr;
	}
	
	uint32_t hdaVen, hdaDev;
	if (!WIOKit::getOSDataValue(hdaService, "vendor-id", hdaVen) ||
		!WIOKit::getOSDataValue(hdaService, "device-id", hdaDev)) {
		SYSLOG("audio", "found an unknown device");
		return nullptr;
	}
	
	auto hdaPlaneName = hdaService->getName();
	DBGLOG("audio", "corrects digital audio for hdau at %s with %04X:%04X",
		   hdaPlaneName ? hdaPlaneName : "(null)", hdaVen, hdaDev);
	
	if (hdaVen != WIOKit::VendorID::ATIAMD) {
		DBGLOG("audio", "unsupported hdau vendor");
		return nullptr;
	}
	
	IORegistryEntry *gpuService {nullptr};
	
	// Cannot iterate over siblings, so firstly get to IOPP
	auto controller = hdaService->getParentEntry(gIOServicePlane);
	if (controller) {
		// Then iterate over IOPP children (GFX0 and HDAU)
		auto iterator = controller->getChildIterator(gIOServicePlane);
		if (iterator) {
			while ((gpuService = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) != nullptr) {
				uint32_t classCode;
				if (WIOKit::getOSDataValue(gpuService, "class-code", classCode)) {
					if (classCode == WIOKit::ClassCode::VGAController || classCode == WIOKit::ClassCode::DisplayController)
						break;
					else
						DBGLOG("audio", "found incompatible class-code %04X", classCode);
				} else {
					auto name = gpuService->getName();
					if (!name) name = "null";
					DBGLOG("audio", "failed to find class-code in %s", name);
				}
				
				gpuService = nullptr;
			}
			
			iterator->release();
		} else {
			DBGLOG("audio", "hdau parent iterator error");
		}
	} else {
		DBGLOG("audio", "hdau parent error");
	}
	
	if (!gpuService) {
		DBGLOG("audio", "failed to find gpu service");
		return nullptr;
	}
	
	uint32_t gpuVen, gpuDev;
	if (!WIOKit::getOSDataValue(gpuService, "vendor-id", gpuVen) ||
		!WIOKit::getOSDataValue(gpuService, "device-id", gpuDev)) {
		SYSLOG("audio", "found an unknown gpu device");
		return nullptr;
	}
	
	auto gpuPlaneName = gpuService->getName();
	if (!gpuPlaneName) gpuPlaneName = "(null)";
	DBGLOG("audio", "corrects digital audio for gpu at %s with %04X:%04X", gpuPlaneName, gpuVen, gpuDev);
	
	if (gpuVen != WIOKit::VendorID::ATIAMD) {
		DBGLOG("audio", "unsupported GPU vendor");
		return nullptr;
	}

    char tmp[4];
    if (gpuService->getProperty("no-audio-autofix") || PE_parse_boot_argn("-radnoaudio", tmp, sizeof(tmp))) {
        DBGLOG("audio", "asked to avoid messing with digital audio");
        return nullptr;
    }

	// Power management may cause issues for non GFX0
	if (!gpuPlaneName || strncmp(gpuPlaneName, "GFX", strlen("GFX"))) {
		DBGLOG("audio", "fixing gpu plane name to GFX0");
		WIOKit::renameDevice(gpuService, "GFX0");
	}

	// AppleHDAController only recognises HDEF and HDAU
	if (!hdaPlaneName || strcmp(hdaPlaneName, "HDAU")) {
		DBGLOG("audio", "fixing audio plane name to HDAU");
		WIOKit::renameDevice(hdaService, "HDAU");
	}
	
	// hda-gfx allows to separate the devices, must be unique
	
	auto hdaHdaGfx = hdaService->getProperty("hda-gfx");
	auto gpuHdaGfx = gpuService->getProperty("hda-gfx");
	if (!hdaHdaGfx && !gpuHdaGfx) {
		static uint32_t hdaCounter {0};
		static const char *hdaNames[] {
			"onboard-2",
			"onboard-3",
			"onboard-4"
		};
		
		if (hdaCounter < arrsize(hdaNames)) {
			DBGLOG("audio", "fixing hda-gfx to %s", hdaNames[hdaCounter]);
			auto hda = OSData::withBytes(hdaNames[hdaCounter], sizeof("onboard-2"));
			hdaService->setProperty("hda-gfx", hda);
			gpuService->setProperty("hda-gfx", hda);
			hdaCounter++;
		} else {
			SYSLOG("audio", "out of hda-gfx indexes");
		}
	} else {
		DBGLOG("audio", "existing hda-gfx in gpu (%d) or hdau (%d), assuming complete inject",
			   gpuHdaGfx != nullptr, hdaHdaGfx != nullptr);
	}
	
	// layout-id is heard to be required in rare cases
	
	if (!hdaService->getProperty("layout-id")) {
		DBGLOG("audio", "fixing layout-id in hdau");
		uint32_t layout = getAnalogLayout();
		hdaService->setProperty("layout-id", &layout, sizeof(layout));
	} else {
		DBGLOG("audio", "found existing layout-id in hdau");
	}
	
	// built-in is required for non-renamed devices
	
	if (!hdaService->getProperty("built-in")) {
		DBGLOG("audio", "fixing built-in in hdau");
		uint8_t builtBytes[] { 0x00 };
		hdaService->setProperty("built-in", builtBytes, sizeof(builtBytes));
	} else {
		DBGLOG("audio", "found existing built-in in hdau");
	}
	
	if (!gpuService->getProperty("built-in")) {
		DBGLOG("audio", "fixing built-in in gpu");
		uint8_t builtBytes[] { 0x00 };
		gpuService->setProperty("built-in", builtBytes, sizeof(builtBytes));
	} else {
		DBGLOG("audio", "found existing built-in in gpu");
	}

	return nullptr;
}
