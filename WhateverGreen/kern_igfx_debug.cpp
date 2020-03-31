//
//  kern_igfx_debug.cpp
//  WhateverGreen
//
//  Copyright © 2020 vit9696. All rights reserved.
//

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <kern_igfx.hpp>
#include <Library/LegacyIOService.h>
#include <IOKit/graphics/IOFramebuffer.h>

#ifdef DEBUG

static mach_vm_address_t fbdebugOrgEnableController;
static mach_vm_address_t fbdebugOrgGetAttribute;
static mach_vm_address_t fbdebugOrgSetAttribute;
static mach_vm_address_t fbdebugOrgSetDisplayMode;
static mach_vm_address_t fbdebugOrgConnectionProbe;
static mach_vm_address_t fbdebugOrgGetDisplayStatus;
static mach_vm_address_t fbdebugOrgGetOnlineInfo;
static mach_vm_address_t fbdebugOrgDoSetPowerState;
static mach_vm_address_t fbdebugOrgValidateDisplayMode;
static mach_vm_address_t fbdebugOrgIsMultilinkDisplay;
static mach_vm_address_t fbdebugOrgHasExternalDisplay;
static mach_vm_address_t fbdebugOrgSetDPPowerState;
static mach_vm_address_t fbdebugOrgSetDisplayPipe;
static mach_vm_address_t fbdebugOrgSetFBMemory;
static mach_vm_address_t fbdebugOrgFBClientDoAttribute;

// Check these, 10.15.4 changed their logic?
// AppleIntelFramebuffer::getPixelInformation
// AppleIntelFramebuffer::populateFBState

static IOReturn fbdebugWrapEnableController(IOService *framebuffer) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "enableController %d start", idx);
	IOReturn ret = FunctionCast(fbdebugWrapEnableController, fbdebugOrgEnableController)(framebuffer);
	SYSLOG("igfx", "enableController %d end - %x", idx, ret);
	return ret;
}

static const char *getAttributeName(IOSelect attr) {
	struct {
		uint32_t attr;
		const char *name;
	} mapping[] = {
		{ 'flgs',  "kConnectionFlags" },
		{ 'sync',  "kConnectionSyncEnable" },
		{ 'sycf',  "kConnectionSyncFlags" },
		{ 'asns',  "kConnectionSupportsAppleSense" },
		{ 'lddc',  "kConnectionSupportsLLDDCSense" },
		{ 'hddc',  "kConnectionSupportsHLDDCSense" },
		{ 'enab',  "kConnectionEnable" },
		{ 'cena',  "kConnectionCheckEnable" },
		{ 'prob',  "kConnectionProbe" },
		{ '\0igr', "kConnectionIgnore" },
		{ 'chng',  "kConnectionChanged" },
		{ 'powr',  "kConnectionPower" },
		{ 'pwak',  "kConnectionPostWake" },
		{ 'pcnt',  "kConnectionDisplayParameterCount" },
		{ 'parm',  "kConnectionDisplayParameters" },
		{ 'oscn',  "kConnectionOverscan" },
		{ 'vbst',  "kConnectionVideoBest" },
		{ 'rgsc',  "kConnectionRedGammaScale" },
		{ 'ggsc',  "kConnectionGreenGammaScale" },
		{ 'bgsc',  "kConnectionBlueGammaScale" },
		{ 'gsc ',  "kConnectionGammaScale" },
		{ 'flus',  "kConnectionFlushParameters" },
		{ 'vblm',  "kConnectionVBLMultiplier" },
		{ 'dpir',  "kConnectionHandleDisplayPortEvent" },
		{ 'pnlt',  "kConnectionPanelTimingDisable" },
		{ 'cyuv',  "kConnectionColorMode" },
		{ 'colr',  "kConnectionColorModesSupported" },
		{ ' bpc',  "kConnectionColorDepthsSupported" },
		{ '\0grd', "kConnectionControllerDepthsSupported" },
		{ '\0dpd', "kConnectionControllerColorDepth" },
		{ '\0gdc', "kConnectionControllerDitherControl" },
		{ 'dflg',  "kConnectionDisplayFlags" },
		{ 'aud ',  "kConnectionEnableAudio" },
		{ 'auds',  "kConnectionAudioStreaming" },
		{ 'soft',  "kConnectionStartOfFrameTime" },
	};

	for (auto &map : mapping)
		if (attr == map.attr)
			return map.name;
	return "<unknown>";
}

static IOReturn fbdebugWrapGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t * value) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "getAttributeForConnection %d %d %s (%x) start (non-null - %d)", idx, connectIndex, getAttributeName(attribute), attribute, value != nullptr);
	IOReturn ret = FunctionCast(fbdebugWrapGetAttribute, fbdebugOrgGetAttribute)(framebuffer, connectIndex, attribute, value);
	SYSLOG("igfx", "getAttributeForConnection %d %d %s (%x) end - %x / %llx", idx, connectIndex, getAttributeName(attribute), attribute, ret, value ? *value : 0);
	return ret;
}

static IOReturn fbdebugWrapSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t value) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "setAttributeForConnection %d %d %s (%x) start -> %llx", idx, connectIndex, getAttributeName(attribute), attribute, value);
	IOReturn ret = FunctionCast(fbdebugWrapSetAttribute, fbdebugOrgSetAttribute)(framebuffer, connectIndex, attribute, value);
	SYSLOG("igfx", "setAttributeForConnection %d %d %s (%x) end -> %llx - %x", idx, connectIndex, getAttributeName(attribute), attribute, value, ret);
	return ret;
}

static IOReturn fbdebugWrapSetDisplayMode(IOService *framebuffer, IODisplayModeID displayMode, IOIndex depth) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "setDisplayMode %d %x start", idx, displayMode, depth);
	IOReturn ret = FunctionCast(fbdebugWrapSetDisplayMode, fbdebugOrgSetDisplayMode)(framebuffer, displayMode, depth);
	SYSLOG("igfx", "setDisplayMode %d %x end - %x", idx, displayMode, depth, ret);
	return ret;
}

static IOReturn fbdebugWrapConnectionProbe(IOService *framebuffer, uint8_t unk1, uint8_t unk2) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "connectionProbe %d %x %x start", idx, unk1, unk2);
	IOReturn ret = FunctionCast(fbdebugWrapConnectionProbe, fbdebugOrgConnectionProbe)(framebuffer, unk1, unk2);
	SYSLOG("igfx", "connectionProbe %d %x %x end - %x", idx, unk1, unk2, ret);
	return ret;
}

static bool fbdebugWrapGetDisplayStatus(IOService *framebuffer, void *displayPath) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "getDisplayStatus %d start", idx);
	bool ret = FunctionCast(fbdebugWrapGetDisplayStatus, fbdebugOrgGetDisplayStatus)(framebuffer, displayPath);
	SYSLOG("igfx", "getDisplayStatus %d end - %d", idx, ret);
	//FIXME: This is just a hack.
	SYSLOG("igfx", "[HACK] forcing STATUS 1");
	ret = 1;
	return ret;
}

static IOReturn fbdebugWrapGetOnlineInfo(IOService *framebuffer, void *displayPath, uint8_t *displayConnected, uint8_t *edid, void *displayPortType, bool *unk1, bool unk2) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "getOnlineInfo %d %d start", idx, unk2);
	IOReturn ret = FunctionCast(fbdebugWrapGetOnlineInfo, fbdebugOrgGetOnlineInfo)(framebuffer, displayPath, displayConnected, edid, displayPortType, unk1, unk2);
	SYSLOG("igfx", "getOnlineInfo %d %d -> %x - %x", idx, unk2, displayConnected ? *displayConnected : (uint32_t)-1, ret);
	return ret;
}

static IOReturn fbdebugWrapDoSetPowerState(IOService *framebuffer, uint32_t state) {
	// state 0 = sleep, 1 = wake, 2 = doze, cap at doze if higher.
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "doSetPowerState %d %u start", idx, state);
	IOReturn ret = FunctionCast(fbdebugWrapDoSetPowerState, fbdebugOrgDoSetPowerState)(framebuffer, state);
	SYSLOG("igfx", "setDisplayMode %d %u end - %x", idx, state, ret);
	return ret;
}

static bool fbdebugWrapIsMultilinkDisplay(IOService *framebuffer) {
	bool ret = FunctionCast(fbdebugWrapIsMultilinkDisplay, fbdebugOrgIsMultilinkDisplay)(framebuffer);
	SYSLOG("igfx", "isMultilinkDisplay - %d", ret);
	return ret;
}

static IOReturn fbdebugWrapValidateDisplayMode(IOService *framebuffer, uint32_t mode, void const **modeDescription, IODetailedTimingInformationV2 **timing) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "validateDisplayMode %d %x start", idx, mode);
	IOReturn ret = FunctionCast(fbdebugWrapValidateDisplayMode, fbdebugOrgValidateDisplayMode)(framebuffer, mode, modeDescription, timing);
	int w = -1, h = -1;
	if (ret == kIOReturnSuccess && timing && timing) {
		w = (int)(*timing)->horizontalActive;
		h = (int)(*timing)->verticalActive;
	}
	SYSLOG("igfx", "validateDisplayMode %d %x end -> %d/%d - %x", idx, mode, w, h, ret);
	// Mostly comes from AppleIntelFramebufferController::hwSetCursorState, which itself comes from deferredMoveCursor/showCursor.
	// SYSTRACE("igfx", "validateDisplayMode trace");
	return ret;
}

static bool fbdebugWrapHasExternalDisplay(IOService *controller) {
	bool ret = FunctionCast(fbdebugWrapHasExternalDisplay, fbdebugOrgHasExternalDisplay)(controller);
	SYSLOG("igfx", "hasExternalDisplay - %d", ret);
	return ret;
}

static IOReturn fbdebugWrapSetDPPowerState(IOService *controller, IOService *framebuffer, bool status, void *displayPath) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "SetDPPowerState %d %d start", idx, status);
	IOReturn ret = FunctionCast(fbdebugWrapSetDPPowerState, fbdebugOrgSetDPPowerState)(controller, framebuffer, status, displayPath);
	SYSLOG("igfx", "SetDPPowerState %d %d end - %x", idx, status, ret);
	return ret;
}

static bool fbdebugWrapSetDisplayPipe(IOService *controller, void *displayPath) {
	SYSLOG("igfx", "setDisplayPipe start");
	bool ret = FunctionCast(fbdebugWrapSetDisplayPipe, fbdebugOrgSetDisplayPipe)(controller, displayPath);
	SYSLOG("igfx", "setDisplayPipe end - %d", ret);
	return ret;
}

static bool fbdebugWrapSetFBMemory(IOService *controller, IOService *framebuffer) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "SetFBMemory %d start", idx);
	bool ret = FunctionCast(fbdebugWrapSetFBMemory, fbdebugOrgSetFBMemory)(controller, framebuffer);
	SYSLOG("igfx", "SetFBMemory %d end - %d", idx, ret);
	return ret;
}

static IOReturn fbdebugWrapFBClientDoAttribute(void *fbclient, uint32_t attribute, unsigned long* unk1, unsigned long unk2, unsigned long* unk3, unsigned long* unk4, void* externalMethodArguments) {
	struct {
		uint32_t attr;
		const char *name;
	} mapping[] = {
		{ 0x1,   "_id" },
		{ 0x120, "pmtGetGPUInfo" },
		{ 0x121, "pmtGetPStateFreqTable" },
		{ 0x122, "pmtGetPStateResidency" },
		{ 0x123, "pmtGetCStateNames" },
		{ 0x124, "pmtGetCStateResidency" },
		{ 0x125, "pmtGetMiscCntrNum" },
		{ 0x126, "pmtGetMiscCntrInfo" },
		{ 0x127, "pmtGetMiscCntr" },
		{ 0x128, "pmtTakeCPStateResidencySnapshot" },
		{ 0x129, "pmtGetPStateResidencyDiff" },
		{ 0x12A, "pmtGetCStateResidencyDiff" },
		{ 0x12B, "pmtGetPStateResidencyDiffAbs" },
		{ 0x701, "SetFbStatusOnNextProbe" },
		{ 0x704, "AGDCInjectEvent" },
		{ 0x707, "FBSetEDID" },
		{ 0x710, "getDisplayPipeCapability" },
		{ 0x711, "getDisplayPipeCapabilityExtended" },
		{ 0x920, "ApplyMultiLinkConfig" },
		{ 0x921, "GetLinkConfig" },
		{ 0x922, "command???" },
		{ 0x923, "RegisterAGDCCallback" },
		{ 0x924, "command???" },
		{ 0x925, "GetPortStatus" },
		{ 0x926, "ConfigureAudio" },
		{ 0x927, "???" },
		{ 0x928, "AGDCStreamSleepControl" },
		{ 0x940, "AGDCPortEnable" },
		{ 0x941, "GetPortCapability" },
		{ 0x980, "GetGPUCapability" },
		{ 0x981, "Get/SetStreamAssociation" },
		{ 0x982, "GetStreamRequest" },
		{ 0x983, "StreamAccessI2C" },
		{ 0x984, "???" },
		{ 0x985, "GetStreamAccessAUX" },
		{ 0x986, "StreamGetEDID" },
		{ 0x987, "SetStreamState" },
		{ 0x988, "Get/SetStreamConfig" },
		{ 0x989, "AGDCEnableController" },
	};

	const char *name = "<unknown>";
	for (auto &map : mapping) {
		if (attribute == map.attr) {
			name = map.name;
			break;
		}
	}

	//FIXME: we are just getting rid of AGDC.
	if (attribute == 0x923) {
		SYSLOG("igfx", "[HACK] FBClientDoAttribute -> disabling AGDC!");
		return kIOReturnUnsupported;
	}

	SYSLOG("igfx", "FBClientDoAttribute %s (%x) start", name, attribute);
	IOReturn ret = FunctionCast(fbdebugWrapFBClientDoAttribute, fbdebugOrgFBClientDoAttribute)(fbclient, attribute, unk1, unk2, unk3, unk4, externalMethodArguments);
	SYSLOG("igfx", "FBClientDoAttribute %s (%x) end - %x", name, attribute, ret);

	return ret;
}

void IGFX::loadFramebufferDebug(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	SYSLOG("igfx", "using framebuffer debug r15");
	KernelPatcher::RouteRequest requests[] = {
		{"__ZN21AppleIntelFramebuffer16enableControllerEv", fbdebugWrapEnableController, fbdebugOrgEnableController},
		{"__ZN21AppleIntelFramebuffer25setAttributeForConnectionEijm", fbdebugWrapSetAttribute, fbdebugOrgSetAttribute},
		{"__ZN21AppleIntelFramebuffer25getAttributeForConnectionEijPm", fbdebugWrapGetAttribute, fbdebugOrgGetAttribute},
		{"__ZN21AppleIntelFramebuffer14setDisplayModeEii", fbdebugWrapSetDisplayMode, fbdebugOrgSetDisplayMode},
		{"__ZN21AppleIntelFramebuffer15connectionProbeEjj", fbdebugWrapConnectionProbe, fbdebugOrgConnectionProbe},
		{"__ZN21AppleIntelFramebuffer16getDisplayStatusEP21AppleIntelDisplayPath", fbdebugWrapGetDisplayStatus, fbdebugOrgGetDisplayStatus},
		{"__ZN21AppleIntelFramebuffer13GetOnlineInfoEP21AppleIntelDisplayPathPhS2_PNS0_15DisplayPortTypeEPbb", fbdebugWrapGetOnlineInfo, fbdebugOrgGetOnlineInfo},
		{"__ZN21AppleIntelFramebuffer15doSetPowerStateEj", fbdebugWrapDoSetPowerState, fbdebugOrgDoSetPowerState},
		{"__ZN21AppleIntelFramebuffer18IsMultiLinkDisplayEv", fbdebugWrapIsMultilinkDisplay, fbdebugOrgIsMultilinkDisplay},
		{"__ZN21AppleIntelFramebuffer19validateDisplayModeEiPPKNS_15ModeDescriptionEPPK29IODetailedTimingInformationV2", fbdebugWrapValidateDisplayMode, fbdebugOrgValidateDisplayMode},
		{"__ZN31AppleIntelFramebufferController18hasExternalDisplayEv", fbdebugWrapHasExternalDisplay, fbdebugOrgHasExternalDisplay},
		{"__ZN31AppleIntelFramebufferController15SetDPPowerStateEP21AppleIntelFramebufferhP21AppleIntelDisplayPath", fbdebugWrapSetDPPowerState, fbdebugOrgSetDPPowerState},
		{"__ZN31AppleIntelFramebufferController14setDisplayPipeEP21AppleIntelDisplayPath", fbdebugWrapSetDisplayPipe, fbdebugOrgSetDisplayPipe},
		{"__ZN31AppleIntelFramebufferController11setFBMemoryEP21AppleIntelFramebuffer", fbdebugWrapSetFBMemory, fbdebugOrgSetFBMemory},
		{"__ZN20IntelFBClientControl11doAttributeEjPmmS0_S0_P25IOExternalMethodArguments", fbdebugWrapFBClientDoAttribute, fbdebugOrgFBClientDoAttribute},
	};

	if (!patcher.routeMultiple(index, requests, address, size, true, true))
		SYSLOG("igfx", "failed to route igfx tracing");
}

#else
void IGFX::loadFramebufferDebug(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	PANIC("igfx", "fb debug is a debug-only feature");
}
#endif
