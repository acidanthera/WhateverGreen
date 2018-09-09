//
//  kern_weg.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_cpu.hpp>
#include "kern_weg.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#include <IOKit/graphics/IOFramebuffer.h>
#pragma clang diagnostic pop

// This is a hack to let us access protected properties.
struct FramebufferViewer : public IOFramebuffer {
	static IOMemoryMap *&getVramMap(IOFramebuffer *fb) {
		// This is a hack to fix old Xcode compilation.
#ifdef __MAC_10_13
		return static_cast<FramebufferViewer *>(fb)->fVramMap;
#else
		return static_cast<FramebufferViewer *>(fb)->vramMap;
#endif
	}
};

static const char *pathIOGraphics[] { "/System/Library/Extensions/IOGraphicsFamily.kext/IOGraphicsFamily" };
static const char *pathAGDPolicy[]  { "/System/Library/Extensions/AppleGraphicsControl.kext/Contents/PlugIns/AppleGraphicsDevicePolicy.kext/Contents/MacOS/AppleGraphicsDevicePolicy" };

static KernelPatcher::KextInfo kextIOGraphics { "com.apple.iokit.IOGraphicsFamily", pathIOGraphics, arrsize(pathIOGraphics), {true}, {}, KernelPatcher::KextInfo::Unloaded };
static KernelPatcher::KextInfo kextAGDPolicy  { "com.apple.driver.AppleGraphicsDevicePolicy", pathAGDPolicy, arrsize(pathAGDPolicy), {true}, {}, KernelPatcher::KextInfo::Unloaded };

WEG *WEG::callbackWEG;

void WEG::init() {
	callbackWEG = this;

	// Background init fix is only necessary on 10.10 and newer.
	// Former boot-arg name is igfxrst.
	if (getKernelVersion() >= KernelVersion::Yosemite) {
		PE_parse_boot_argn("gfxrst", &resetFramebuffer, sizeof(resetFramebuffer));
		if (resetFramebuffer >= FB_TOTAL) {
			SYSLOG("weg", "invalid igfxrset value %d, falling back to autodetect", resetFramebuffer);
			resetFramebuffer = FB_DETECT;
		}
	} else {
		resetFramebuffer = FB_NONE;
	}

	// Black screen fix is needed everywhere, but the form depends on the boot-arg.
	// Former boot-arg name is ngfxpatch.
	char agdp[128];
	if (PE_parse_boot_argn("agdpmod", agdp, sizeof(agdp))) {
		if (strstr(agdp, "detect")) {
			graphicsDisplayPolicyMod = AGDP_DETECT;
		} else {
			graphicsDisplayPolicyMod = AGDP_NONE;
			if (strstr(agdp, "vit9696"))
				graphicsDisplayPolicyMod |= AGDP_VIT9696;
			if (strstr(agdp, "pikera"))
				graphicsDisplayPolicyMod |= AGDP_PIKERA;
			if (strstr(agdp, "cfgmap"))
				graphicsDisplayPolicyMod |= AGDP_CFGMAP;
		}
	}

	// Callback setup is only done here for compatibility.
	lilu.onPatcherLoadForce([](void *user, KernelPatcher &patcher) {
		static_cast<WEG *>(user)->processKernel(patcher);
	}, this);

	lilu.onKextLoadForce(nullptr, 0,
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		static_cast<WEG *>(user)->processKext(patcher, index, address, size);
	}, this);

	// Perform a background fix.
	if (resetFramebuffer != FB_NONE)
		lilu.onKextLoadForce(&kextIOGraphics);

	// Perform a black screen fix.
	if (graphicsDisplayPolicyMod != AGDP_NONE)
		lilu.onKextLoad(&kextAGDPolicy);

	igfx.init();
	ngfx.init();
	rad.init();
	shiki.init();
	cdf.init();
}

void WEG::deinit() {
	igfx.deinit();
	ngfx.deinit();
	rad.deinit();
	shiki.deinit();
	cdf.deinit();
}

void WEG::processKernel(KernelPatcher &patcher) {
	// Correct GPU properties
	auto devInfo = DeviceInfo::create();
	if (devInfo) {
		if (devInfo->requestedExternalSwitchOff) {
			DBGLOG("weg", "disabling all external GPUs");
			size_t extNum = devInfo->videoExternal.size();
			for (size_t i = 0; i < extNum; i++) {
				auto &v = devInfo->videoExternal[i];

				auto gpu = OSDynamicCast(IOService, v.video);
				auto hda = OSDynamicCast(IOService, v.audio);
				auto pci = OSDynamicCast(IOService, v.video->getParentEntry(gIOServicePlane));
				if (gpu && pci) {
					if (gpu->requestTerminate(pci, 0) && gpu->terminate())
						gpu->stop(pci);
					else
						SYSLOG("weg", "failed to terminate external gpu %ld", i);
					if (hda && hda->requestTerminate(pci, 0) && hda->terminate())
						hda->stop(pci);
					else if (hda)
						SYSLOG("weg", "failed to terminate external hdau %ld", i);
				} else {
					SYSLOG("weg", "incompatible external gpu %ld discovered", i);
				}
			}

			devInfo->videoExternal.deinit();
		}


		// Do not inject properties unless non-Apple
		if (devInfo->firmwareVendor != DeviceInfo::FirmwareVendor::Apple) {
			DBGLOG("weg", "non-apple-fw proceeding with devprops %d", graphicsDisplayPolicyMod);
			if (devInfo->videoBuiltin) {
				processBuiltinProperties(devInfo->videoBuiltin, devInfo);

				// Assume that enabled IGPU with connectors is the boot display.
				if (resetFramebuffer == FB_DETECT && !devInfo->reportedFramebufferIsConnectorLess)
					resetFramebuffer = FB_COPY;
			}

			size_t extNum = devInfo->videoExternal.size();
			for (size_t i = 0; i < extNum; i++) {
				auto &v = devInfo->videoExternal[i];
				processExternalProperties(v.video, devInfo, v.vendor);

				// Assume that AMD GPU is the boot display.
				if (v.vendor == WIOKit::VendorID::ATIAMD && resetFramebuffer == FB_DETECT)
					resetFramebuffer = FB_ZEROFILL;
			}

			if (graphicsDisplayPolicyMod == AGDP_DETECT && isGraphicsPolicyModRequired(devInfo))
				graphicsDisplayPolicyMod = AGDP_VIT9696 | AGDP_PIKERA;

			if (devInfo->managementEngine)
				processManagementEngineProperties(devInfo->managementEngine);
		}

		igfx.processKernel(patcher, devInfo);
		ngfx.processKernel(patcher, devInfo);
		rad.processKernel(patcher, devInfo);
		shiki.processKernel(patcher, devInfo);
		cdf.processKernel(patcher, devInfo);

		DeviceInfo::deleter(devInfo);
	}

	// Disable mods that did not find a way to function.
	if (resetFramebuffer == FB_DETECT) {
		resetFramebuffer = FB_NONE;
		kextIOGraphics.switchOff();
	}

	if (graphicsDisplayPolicyMod == AGDP_DETECT) {
		graphicsDisplayPolicyMod = AGDP_NONE;
		kextAGDPolicy.switchOff();
	}

	// We need to load vinfo for cleanup and copy.
	if (resetFramebuffer == FB_COPY || resetFramebuffer == FB_ZEROFILL) {
		auto info = reinterpret_cast<vc_info *>(patcher.solveSymbol(KernelPatcher::KernelID, "_vinfo"));
		if (info) {
			consoleVinfo = *info;
			DBGLOG("weg", "vinfo 1: %u:%u %u:%u:%u",
				   consoleVinfo.v_height, consoleVinfo.v_width, consoleVinfo.v_depth, consoleVinfo.v_rowbytes, consoleVinfo.v_type);
			DBGLOG("weg", "vinfo 2: %s %u:%u %u:%u:%u",
				   consoleVinfo.v_name, consoleVinfo.v_rows, consoleVinfo.v_columns, consoleVinfo.v_rowscanbytes, consoleVinfo.v_scale, consoleVinfo.v_rotate);
			gotConsoleVinfo = true;
		} else {
			SYSLOG("weg", "failed to obtain vcinfo");
			patcher.clearError();
		}
	}
}

void WEG::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (kextIOGraphics.loadIndex == index) {
		gIOFBVerboseBootPtr = patcher.solveSymbol<uint8_t *>(index, "__ZL16gIOFBVerboseBoot", address, size);
		if (gIOFBVerboseBootPtr) {
			KernelPatcher::RouteRequest request("__ZN13IOFramebuffer6initFBEv", wrapFramebufferInit, orgFramebufferInit);
			patcher.routeMultiple(index, &request, 1, address, size);
		} else {
			SYSLOG("rad", "failed to resolve gIOFBVerboseBoot");
			patcher.clearError();
		}

		return;
	}

	if (kextAGDPolicy.loadIndex == index) {
		processGraphicsPolicyMods(patcher, address, size);
		return;
	}

	if (igfx.processKext(patcher, index, address, size))
		return;

	if (ngfx.processKext(patcher, index, address, size))
		return;

	if (rad.processKext(patcher, index, address, size))
		return;

	if (cdf.processKext(patcher, index, address, size))
		return;
}

void WEG::processBuiltinProperties(IORegistryEntry *device, DeviceInfo *info) {
	auto name = device->getName();

	// There could be only one IGPU, and it must be named IGPU for AppleGVA to function properly.
	if (!name || strcmp(name, "IGPU") != 0)
		WIOKit::renameDevice(device, "IGPU");

	// Obtain the real device info, should we cast to IOPCIDevice here?
	auto obj = OSDynamicCast(IOService, device);
	if (obj) {
		uint32_t realDevice = WIOKit::readPCIConfigValue(obj, WIOKit::kIOPCIConfigDeviceID);
		uint32_t acpiDevice = 0, fakeDevice = 0;

		if (!WIOKit::getOSDataValue(obj, "device-id", acpiDevice))
			DBGLOG("weg", "missing IGPU device-id");

		// Set the right Intel model name here.
		auto model = getIntelModel(realDevice, fakeDevice);
		DBGLOG("weg", "IGPU has real %04X acpi %04X fake %04X and model %s",
			   realDevice, acpiDevice, fakeDevice, safeString(model));
		if (model && !obj->getProperty("model")) {
			DBGLOG("weg", "adding missing model %s from autotodetect", model);
			obj->setProperty("model", OSData::withBytes(model, static_cast<unsigned>(strlen(model)+1)));
		}

		// User may request to fake device-id even if it is supported.
		if (realDevice != acpiDevice) {
			DBGLOG("weg", "user requested to fake with normal device-id");
			fakeDevice = acpiDevice;
		}

		// Update vtable I/O functions to ensure that a correct fake device ID is read.
		if (fakeDevice) {
			// Incorrect device-id means Intel drivers will most likely fail to do matching, error to log.
			if (fakeDevice != acpiDevice) {
				uint8_t bus = 0, dev = 0, fun = 0;
				WIOKit::getDeviceAddress(obj, bus, dev, fun);
				SYSLOG("weg", "IGPU device (%02X:%02X.%02X) has device-id 0x%04X, you should change it to 0x%04X",
					   bus, dev, fun, acpiDevice, fakeDevice);
			}
			if (fakeDevice != realDevice) {
				if (KernelPatcher::routeVirtual(obj, WIOKit::PCIConfigOffset::ConfigRead16, wrapConfigRead16, &orgConfigRead16) &&
					KernelPatcher::routeVirtual(obj, WIOKit::PCIConfigOffset::ConfigRead32, wrapConfigRead32, &orgConfigRead32))
					DBGLOG("weg", "hooked configRead read methods!");
				else
					SYSLOG("weg", "failed to hook configRead read methods!");
			}
		}
	} else {
		SYSLOG("weg", "invalid IGPU device type");
	}

	// Update the requested framebuffer identifier.
	if (info->reportedFramebufferName)
		device->setProperty(info->reportedFramebufferName, &info->reportedFramebufferId, sizeof(info->reportedFramebufferId));

	// Ensure built-in.
	if (!device->getProperty("built-in")) {
		DBGLOG("weg", "fixing built-in");
		uint8_t builtBytes[] { 0x00 };
		device->setProperty("built-in", builtBytes, sizeof(builtBytes));
	} else {
		DBGLOG("weg", "found existing built-in");
	}
}

void WEG::processExternalProperties(IORegistryEntry *device, DeviceInfo *info, uint32_t vendor) {
	auto name = device->getName();

	// It is unclear how to properly name the GPUs, and supposedly it does not really matter.
	// However, we will try to at least name them in a unique manner (GFX0, GFX1, ...)
	if (currentExternalGfxIndex <= MaxExternalGfxIndex && (!name || strncmp(name, "GFX", strlen("GFX")) != 0)) {
		char name[16];
		snprintf(name, sizeof(name), "GFX%u", currentExternalGfxIndex++);
		WIOKit::renameDevice(device, name);
	}

	// AAPL,slot-name is used to distinguish GPU slots in Mac Pro.
	// NVIDIA Web Drivers have a preference panel, where they read this value and allow up to 4 GPUs.
	// Each NVIDIA GPU is then displayed on the ECC tab. We permit more slots, since 4 is an artificial restriction.
	// iMac on the other side has only one GPU and is not expected to have multiple slots.
	// Here we pass AAPL,slot-name if the GPU is NVIDIA or we have more than one GPU.
	bool wantSlot = info->videoExternal.size() > 1 || vendor == WIOKit::VendorID::NVIDIA;
	if (wantSlot && currentExternalSlotIndex <= MaxExternalSlotIndex && !device->getProperty("AAPL,slot-name")) {
		char name[16];
		snprintf(name, sizeof(name), "Slot-%u", currentExternalSlotIndex++);
		device->setProperty("AAPL,slot-name", name, sizeof("Slot-1"));
	}

	// Set the autodetected AMD GPU name here, it will later be handled by RAD to not get overridden.
	// This is not necessary for NVIDIA, as their drivers properly detect the name.
	if (vendor == WIOKit::VendorID::ATIAMD && !device->getProperty("model")) {
		uint32_t dev, rev, subven, sub;
		if (WIOKit::getOSDataValue(device, "device-id", dev) &&
			WIOKit::getOSDataValue(device, "revision-id", rev) &&
			WIOKit::getOSDataValue(device, "subsystem-vendor-id", subven) &&
			WIOKit::getOSDataValue(device, "subsystem-id", sub)) {
			auto model = getRadeonModel(dev, rev, subven, sub);
			if (model)
				device->setProperty("model", OSData::withBytes(model, static_cast<unsigned>(strlen(model)+1)));
		}
	}

	// Ensure built-in.
	if (!device->getProperty("built-in")) {
		DBGLOG("weg", "fixing built-in");
		uint8_t builtBytes[] { 0x00 };
		device->setProperty("built-in", builtBytes, sizeof(builtBytes));
	} else {
		DBGLOG("weg", "found existing built-in");
	}
}

void WEG::processManagementEngineProperties(IORegistryEntry *imei) {
	auto name = imei->getName();
	// Rename mislabeled IMEI device
	if (!name || strcmp(name, "IMEI") != 0)
		WIOKit::renameDevice(imei, "IMEI");

	uint32_t device = 0;
	auto cpuGeneration = CPUInfo::getGeneration();
	if ((cpuGeneration == CPUInfo::CpuGeneration::SandyBridge ||
		 cpuGeneration == CPUInfo::CpuGeneration::IvyBridge) &&
		WIOKit::getOSDataValue(imei, "device-id", device)) {
		// Exotic cases like SNB CPU on 7-series motherboards or IVB CPU on 6-series
		// require device-id faking. Unfortunately it is too late to change it at this step,
		// because device matching happens earlier, but we will spill a warning to make sure
		// one fixes them at device property or ACPI level.
		uint32_t suggest = 0;
		if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge && device != 0x1C3A)
			suggest = 0x1C3A;
		else if (cpuGeneration == CPUInfo::CpuGeneration::IvyBridge && device != 0x1E3A)
			suggest = 0x1E3A;

		if (suggest != 0) {
			uint8_t bus = 0, dev = 0, fun = 0;
			WIOKit::getDeviceAddress(imei, bus, dev, fun);
			SYSLOG("weg", "IMEI device (%02X:%02X.%02X) has device-id 0x%04X, you should change it to 0x%04X",
				   bus, dev, fun, device, suggest);
		}
	}
}

void WEG::processGraphicsPolicyMods(KernelPatcher &patcher, mach_vm_address_t address, size_t size) {
	if (graphicsDisplayPolicyMod & AGDP_VIT9696) {
		uint8_t find[]    = {0xBA, 0x05, 0x00, 0x00, 0x00};
		uint8_t replace[] = {0xBA, 0x00, 0x00, 0x00, 0x00};
		KernelPatcher::LookupPatch patch {
			&kextAGDPolicy, find, replace, sizeof(find), 1
		};

		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("weg", "failed to apply agdp vit9696's patch %d", patcher.getError());
			patcher.clearError();
		}
	}

	if (graphicsDisplayPolicyMod & AGDP_PIKERA) {
		KernelPatcher::LookupPatch patch {
			&kextAGDPolicy,
			reinterpret_cast<const uint8_t *>("board-id"),
			reinterpret_cast<const uint8_t *>("board-ix"),
			sizeof("board-id"), 1
		};

		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("weg", "failed to apply agdp Piker-Alpha's patch %d", patcher.getError());
			patcher.clearError();
		}
	}

	if (graphicsDisplayPolicyMod & AGDP_CFGMAP) {
		//FIXME: Does not function in 10.13.x, as the symbols have been stripped.
		// Should not be needed really, remove it?
		KernelPatcher::RouteRequest request("__ZN25AppleGraphicsDevicePolicy5startEP9IOService", wrapGraphicsPolicyStart, orgGraphicsPolicyStart);
		patcher.routeMultiple(kextAGDPolicy.loadIndex, &request, 1, address, size);
	}
}

bool WEG::isGraphicsPolicyModRequired(DeviceInfo *info) {
	DBGLOG("weg", "detecting policy");
	// Graphics policy patches are only applicable to discrete GPUs.
	if (info->videoExternal.size() == 0) {
		DBGLOG("weg", "no external gpus");
		return false;
	}

	// Graphics policy patches do harm on Apple MacBooks, see:
	// https://github.com/acidanthera/bugtracker/issues/260
	if (info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple) {
		DBGLOG("weg", "apple firmware");
		return false;
	}

	// We do not need AGDC patches on compatible devices.
	char boardIdentifier[64];
	if (WIOKit::getComputerInfo(nullptr, 0, boardIdentifier, sizeof(boardIdentifier)) && boardIdentifier[0] != '\0') {
		DBGLOG("weg", "board is %s", boardIdentifier);
		const char *compatibleBoards[] {
			"Mac-00BE6ED71E35EB86", // iMac13,1
			"Mac-27ADBB7B4CEE8E61", // iMac14,2
			"Mac-4B7AC7E43945597E", // MacBookPro9,1
			"Mac-77EB7D7DAF985301", // iMac14,3
			"Mac-C3EC7CD22292981F", // MacBookPro10,1
			"Mac-C9CF552659EA9913", // ???
			"Mac-F221BEC8",         // MacPro5,1 (and MacPro4,1)
			"Mac-F221DCC8",         // iMac10,1
			"Mac-F42C88C8",         // MacPro3,1
			"Mac-FC02E91DDD3FA6A4", // iMac13,2
			"Mac-2BD1B31983FE1663"  // MacBookPro11,3
		};
		for (size_t i = 0; i < arrsize(compatibleBoards); i++) {
			if (!strcmp(compatibleBoards[i], boardIdentifier)) {
				DBGLOG("weg", "disabling nvidia patches on model %s", boardIdentifier);
				return false;
			}
		}
	}

	return true;
}

void WEG::wrapFramebufferInit(IOFramebuffer *fb) {
	bool backCopy = callbackWEG->gotConsoleVinfo && callbackWEG->resetFramebuffer == FB_COPY;
	bool zeroFill  = callbackWEG->gotConsoleVinfo && callbackWEG->resetFramebuffer == FB_ZEROFILL;
	auto &info = callbackWEG->consoleVinfo;

	// Copy back usually happens in a separate call to frameBufferInit
	// Furthermore, v_baseaddr may not be available on subsequent calls, so we have to copy
	if (backCopy && info.v_baseaddr) {
		// Note, this buffer is left allocated and never freed, yet there actually is no way to free it.
		callbackWEG->consoleBuffer = Buffer::create<uint8_t>(info.v_rowbytes * info.v_height);
		if (callbackWEG->consoleBuffer)
			lilu_os_memcpy(callbackWEG->consoleBuffer, reinterpret_cast<uint8_t *>(info.v_baseaddr), info.v_rowbytes * info.v_height);
		else
			SYSLOG("weg", "console buffer allocation failure");
		// Even if we may succeed next time, it will be unreasonably dangerous
		info.v_baseaddr = 0;
	}

	uint8_t verboseBoot = *callbackWEG->gIOFBVerboseBootPtr;
	// For back copy we need a console buffer and no verbose
	backCopy = backCopy && callbackWEG->consoleBuffer && !verboseBoot;

	// Now check if the resolution and parameters match
	if (backCopy || zeroFill) {
		IODisplayModeID mode;
		IOIndex depth;
		IOPixelInformation pixelInfo;

		if (fb->getCurrentDisplayMode(&mode, &depth) == kIOReturnSuccess &&
			fb->getPixelInformation(mode, depth, kIOFBSystemAperture, &pixelInfo) == kIOReturnSuccess) {
			DBGLOG("weg", "fb info 1: %d:%d %u:%u:%u",
				   mode, depth, pixelInfo.bytesPerRow, pixelInfo.bytesPerPlane, pixelInfo.bitsPerPixel);
			DBGLOG("weg", "fb info 2: %u:%u %s %u:%u:%u",
				   pixelInfo.componentCount, pixelInfo.bitsPerComponent, pixelInfo.pixelFormat, pixelInfo.flags, pixelInfo.activeWidth, pixelInfo.activeHeight);

			if (info.v_rowbytes != pixelInfo.bytesPerRow || info.v_width != pixelInfo.activeWidth ||
				info.v_height != pixelInfo.activeHeight || info.v_depth != pixelInfo.bitsPerPixel) {
				backCopy = zeroFill = false;
				DBGLOG("weg", "this display has different mode");
			}
		} else {
			DBGLOG("weg", "failed to obtain display mode");
			backCopy = zeroFill = false;
		}
	}

	// For whatever reason not resetting Intel framebuffer (back copy mode) twice works better.
	if (!backCopy) *callbackWEG->gIOFBVerboseBootPtr = 1;
	FunctionCast(wrapFramebufferInit, callbackWEG->orgFramebufferInit)(fb);
	if (!backCopy) *callbackWEG->gIOFBVerboseBootPtr = verboseBoot;

	// Finish the framebuffer initialisation by filling with black or copying the image back.
	if (FramebufferViewer::getVramMap(fb)) {
		auto src = reinterpret_cast<uint8_t *>(callbackWEG->consoleBuffer);
		auto dst = reinterpret_cast<uint8_t *>(FramebufferViewer::getVramMap(fb)->getVirtualAddress());
		if (backCopy) {
			DBGLOG("weg", "attempting to copy...");
			// Here you can actually draw at your will, but looks like only on Intel.
			// On AMD you technically can draw too, but it happens for a very short while, and is not worth it.
			lilu_os_memcpy(dst, src, info.v_rowbytes * info.v_height);
		} else if (zeroFill) {
			// On AMD we do a zero-fill to ensure no visual glitches.
			DBGLOG("weg", "doing zero-fill...");
			memset(dst, 0, info.v_rowbytes * info.v_height);
		}
	}
}

uint16_t WEG::wrapConfigRead16(IORegistryEntry *service, uint32_t space, uint8_t offset) {
	auto result = callbackWEG->orgConfigRead16(service, space, offset);
	if (offset == WIOKit::kIOPCIConfigDeviceID && service != nullptr) {
		auto name = service->getName();
		if (name && name[0] == 'I' && name[1] == 'G' && name[2] == 'P' && name[3] == 'U') {
			DBGLOG("weg", "configRead16 IGPU 0x%08X at off 0x%02X, result = 0x%04x", space, offset, result);
			uint32_t device;
			if (WIOKit::getOSDataValue(service, "device-id", device) && device != result) {
				DBGLOG("weg", "configRead16 IGPU reported 0x%04x instead of 0x%04x", device, result);
				return device;
			}
		}
	}

	return result;
}

uint32_t WEG::wrapConfigRead32(IORegistryEntry *service, uint32_t space, uint8_t offset) {
	auto result = callbackWEG->orgConfigRead32(service, space, offset);
	// According to lvs1974 unaligned reads may actually happen!
	if ((offset == WIOKit::kIOPCIConfigDeviceID || offset == WIOKit::kIOPCIConfigVendorID) && service != nullptr) {
		auto name = service->getName();
		if (name && name[0] == 'I' && name[1] == 'G' && name[2] == 'P' && name[3] == 'U') {
			DBGLOG("weg", "configRead32 IGPU 0x%08X at off 0x%02X, result = 0x%08X", space, offset, result);
			uint32_t device;
			if (WIOKit::getOSDataValue(service, "device-id", device) && device != (result & 0xFFFF)) {
				device = (result & 0xFFFF) | (device << 16);
				DBGLOG("weg", "configRead32 reported 0x%08x instead of 0x%08x", device, result);
				return device;
			}
		}
	}

	return result;
}

bool WEG::wrapGraphicsPolicyStart(IOService *that, IOService *provider) {
	char boardIdentifier [32];
	if (WIOKit::getComputerInfo(nullptr, 0, boardIdentifier, sizeof(boardIdentifier)) && boardIdentifier[0] != '\0') {
		DBGLOG("weg", "agdp fix got board-id %s", boardIdentifier);
		auto oldConfigMap = OSDynamicCast(OSDictionary, that->getProperty("ConfigMap"));
		if (oldConfigMap) {
			auto newConfigMap = OSDynamicCast(OSDictionary, oldConfigMap->copyCollection());
			if (newConfigMap) {
				newConfigMap->setObject(boardIdentifier, OSString::withCString("none"));
				that->setProperty("ConfigMap", newConfigMap);
			} else {
				SYSLOG("weg", "agdp fix failed to clone ConfigMap");
			}
		} else {
			SYSLOG("weg", "agdp fix failed to obtain valid ConfigMap");
		}
	} else {
		SYSLOG("weg", "failed to obtain computer board-id for agdp fix");
	}

	bool result = FunctionCast(wrapGraphicsPolicyStart, callbackWEG->orgGraphicsPolicyStart)(that, provider);
	DBGLOG("weg", "agdp start returned %d", result);

	return result;
}
