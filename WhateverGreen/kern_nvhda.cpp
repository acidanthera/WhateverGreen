//
//  kern_nvhda.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_nvhda.hpp"

#include <Headers/kern_util.hpp>
#include <libkern/libkern.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IODeviceTreeSupport.h>

// Workaround for systems with BIOSes that default-disable the HD Audio function on their NVIDIA GPUs.
// We match the device with a higher IOProbeScore than the NVIDIA drivers, use our probe routine to
// enable the HD Audio function, trigger a PCI rescan, and then return a probe failure so that the
// real driver can continue to load.
//
// References:
// https://bugs.freedesktop.org/show_bug.cgi?id=75985
// https://devtalk.nvidia.com/default/topic/1024022/linux/gtx-1060-no-audio-over-hdmi-only-hda-intel-detected-azalia/
// https://github.com/acidanthera/bugtracker/issues/292

OSDefineMetaClassAndStructors(NVHDAEnabler, IOService);

IOService* NVHDAEnabler::probe(IOService *provider, SInt32 *score) {
	auto pciDevice = OSDynamicCast(IOPCIDevice, provider);
	if (!pciDevice) {
		SYSLOG("NVHDAEnabler", "probe: pciDevice is NULL");
		return nullptr;
	}

	uint32_t hdaEnableDword = pciDevice->configRead32(HDAEnableReg);
	if (hdaEnableDword & HDAEnableBit) {
		DBGLOG("NVHDAEnabler", "probe: HDA enable bit is already set, nothing to do");
		return nullptr;
	}

	DBGLOG("NVHDAEnabler", "probe: reg is 0x%x, setting HDA enable bit", hdaEnableDword);
	hdaEnableDword |= HDAEnableBit;
	pciDevice->configWrite32(HDAEnableReg, hdaEnableDword);

	// Verify with readback
	DBGLOG("NVHDAEnabler", "probe: readback: reg is 0x%x", pciDevice->configRead32(HDAEnableReg));

	// Find the parent IOPCIBridge
	auto parentBridge = OSDynamicCast(IOPCIDevice, pciDevice->getParentEntry(gIODTPlane));
	if (!parentBridge) {
		DBGLOG("NVHDAEnabler", "probe: Can't find the parent bridge's IOPCIDevice");
		return nullptr;
	}

	DBGLOG("NVHDAEnabler", "probe: Requesting parent bridge rescan");

	// Mark this device and the parent bridge as needing scanning, then trigger the rescan.
	pciDevice->kernelRequestProbe(kIOPCIProbeOptionNeedsScan);
	parentBridge->kernelRequestProbe(kIOPCIProbeOptionNeedsScan | kIOPCIProbeOptionDone);

	// This probe must always fail so that the real driver can get a chance to load afterwards.
	return nullptr;
}

bool NVHDAEnabler::start(IOService *provider) {
	SYSLOG("NVHDAEnabler", "start: shouldn't be called!");
	return false;
}
