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

static mach_vm_address_t orgEnableController;
static mach_vm_address_t orgGetAttribute;
static mach_vm_address_t orgSetAttribute;
static mach_vm_address_t orgSetDisplayMode;
static mach_vm_address_t orgConnectionProbe;
static mach_vm_address_t orgGetDisplayStatus;
static mach_vm_address_t orgGetOnlineInfo;
static mach_vm_address_t orgDoSetPowerState;
static mach_vm_address_t orgValidateDisplayMode;
static mach_vm_address_t orgIsMultilinkDisplay;
static mach_vm_address_t orgHasExternalDisplay;
static mach_vm_address_t orgSetDPPowerState;
static mach_vm_address_t orgSetDisplayPipe;
static mach_vm_address_t orgSetFBMemory;
static mach_vm_address_t orgFBClientDoAttribute;

// Check these, 10.15.4 changed their logic?
// AppleIntelFramebuffer::getPixelInformation
// AppleIntelFramebuffer::populateFBState

static IOReturn wrapEnableController(IOService *framebuffer) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "enableController %d start", idx);
	IOReturn ret = FunctionCast(wrapEnableController, orgEnableController)(framebuffer);
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

static IOReturn wrapGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t * value) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "getAttributeForConnection %d %d %s (%x) start (non-null - %d)", idx, connectIndex, getAttributeName(attribute), attribute, value != nullptr);
	IOReturn ret = FunctionCast(wrapGetAttribute, orgGetAttribute)(framebuffer, connectIndex, attribute, value);
	SYSLOG("igfx", "getAttributeForConnection %d %d %s (%x) end - %x / %llx", idx, connectIndex, getAttributeName(attribute), attribute, ret, value ? *value : 0);
	return ret;
}

static IOReturn wrapSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t value) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "setAttributeForConnection %d %d %s (%x) start -> %llx", idx, connectIndex, getAttributeName(attribute), attribute, value);
	IOReturn ret = FunctionCast(wrapSetAttribute, orgSetAttribute)(framebuffer, connectIndex, attribute, value);
	SYSLOG("igfx", "setAttributeForConnection %d %d %s (%x) end -> %llx - %x", idx, connectIndex, getAttributeName(attribute), attribute, value, ret);
	return ret;
}

static IOReturn wrapSetDisplayMode(IOService *framebuffer, IODisplayModeID displayMode, IOIndex depth) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "setDisplayMode %d %x start", idx, displayMode, depth);
	IOReturn ret = FunctionCast(wrapSetDisplayMode, orgSetDisplayMode)(framebuffer, displayMode, depth);
	SYSLOG("igfx", "setDisplayMode %d %x end - %x", idx, displayMode, depth, ret);
	return ret;
}

static IOReturn wrapConnectionProbe(IOService *framebuffer, uint8_t unk1, uint8_t unk2) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "connectionProbe %d %x %x start", idx, unk1, unk2);
	IOReturn ret = FunctionCast(wrapConnectionProbe, orgConnectionProbe)(framebuffer, unk1, unk2);
	SYSLOG("igfx", "connectionProbe %d %x %x end - %x", idx, unk1, unk2, ret);
	return ret;
}

static bool wrapGetDisplayStatus(IOService *framebuffer, void *displayPath) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "getDisplayStatus %d start", idx);
	bool ret = FunctionCast(wrapGetDisplayStatus, orgGetDisplayStatus)(framebuffer, displayPath);
	SYSLOG("igfx", "getDisplayStatus %d end - %d", idx, ret);
	//FIXME: This is just a hack.
	SYSLOG("igfx", "[HACK] forcing STATUS 1");
	ret = 1;
	return ret;
}

static IOReturn wrapGetOnlineInfo(IOService *framebuffer, void *displayPath, uint8_t *displayConnected, uint8_t *edid, void *displayPortType, bool *unk1, bool unk2) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "getOnlineInfo %d %d start", idx, unk2);
	IOReturn ret = FunctionCast(wrapGetOnlineInfo, orgGetOnlineInfo)(framebuffer, displayPath, displayConnected, edid, displayPortType, unk1, unk2);
	SYSLOG("igfx", "getOnlineInfo %d %d -> %x - %x", idx, unk2, displayConnected ? *displayConnected : (uint32_t)-1, ret);
	return ret;
}

static IOReturn wrapDoSetPowerState(IOService *framebuffer, uint32_t state) {
	// state 0 = sleep, 1 = wake, 2 = doze, cap at doze if higher.
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "doSetPowerState %d %u start", idx, state);
	IOReturn ret = FunctionCast(wrapDoSetPowerState, orgDoSetPowerState)(framebuffer, state);
	SYSLOG("igfx", "setDisplayMode %d %u end - %x", idx, state, ret);
	return ret;
}

static bool wrapIsMultilinkDisplay(IOService *framebuffer) {
	bool ret = FunctionCast(wrapIsMultilinkDisplay, orgIsMultilinkDisplay)(framebuffer);
	SYSLOG("igfx", "isMultilinkDisplay - %d", ret);
	return ret;
}

static IOReturn wrapValidateDisplayMode(IOService *framebuffer, uint32_t mode, void const **modeDescription, IODetailedTimingInformationV2 **timing) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "validateDisplayMode %d %x start", idx, mode);
	IOReturn ret = FunctionCast(wrapValidateDisplayMode, orgValidateDisplayMode)(framebuffer, mode, modeDescription, timing);
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

static bool wrapHasExternalDisplay(IOService *controller) {
	bool ret = FunctionCast(wrapHasExternalDisplay, orgHasExternalDisplay)(controller);
	SYSLOG("igfx", "hasExternalDisplay - %d", ret);
	return ret;
}

static IOReturn wrapSetDPPowerState(IOService *controller, IOService *framebuffer, bool status, void *displayPath) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "SetDPPowerState %d %d start", idx, status);
	IOReturn ret = FunctionCast(wrapSetDPPowerState, orgSetDPPowerState)(controller, framebuffer, status, displayPath);
	SYSLOG("igfx", "SetDPPowerState %d %d end - %x", idx, status, ret);
	return ret;
}

static bool wrapSetDisplayPipe(IOService *controller, void *displayPath) {
	SYSLOG("igfx", "setDisplayPipe start");
	bool ret = FunctionCast(wrapSetDisplayPipe, orgSetDisplayPipe)(controller, displayPath);
	SYSLOG("igfx", "setDisplayPipe end - %d", ret);
	return ret;
}

static bool wrapSetFBMemory(IOService *controller, IOService *framebuffer) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "SetFBMemory %d start", idx);
	bool ret = FunctionCast(wrapSetFBMemory, orgSetFBMemory)(controller, framebuffer);
	SYSLOG("igfx", "SetFBMemory %d end - %d", idx, ret);
	return ret;
}

static IOReturn wrapFBClientDoAttribute(void *fbclient, uint32_t attribute, unsigned long* unk1, unsigned long unk2, unsigned long* unk3, unsigned long* unk4, void* externalMethodArguments) {
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
	IOReturn ret = FunctionCast(wrapFBClientDoAttribute, orgFBClientDoAttribute)(fbclient, attribute, unk1, unk2, unk3, unk4, externalMethodArguments);
	SYSLOG("igfx", "FBClientDoAttribute %s (%x) end - %x", name, attribute, ret);

	return ret;
}

void IGFX::loadFramebufferDebug(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	SYSLOG("igfx", "using framebuffer debug r15");
	KernelPatcher::RouteRequest requests[] = {
		{"__ZN21AppleIntelFramebuffer16enableControllerEv", wrapEnableController, orgEnableController},
		{"__ZN21AppleIntelFramebuffer25setAttributeForConnectionEijm", wrapSetAttribute, orgSetAttribute},
		{"__ZN21AppleIntelFramebuffer25getAttributeForConnectionEijPm", wrapGetAttribute, orgGetAttribute},
		{"__ZN21AppleIntelFramebuffer14setDisplayModeEii", wrapSetDisplayMode, orgSetDisplayMode},
		{"__ZN21AppleIntelFramebuffer15connectionProbeEjj", wrapConnectionProbe, orgConnectionProbe},
		{"__ZN21AppleIntelFramebuffer16getDisplayStatusEP21AppleIntelDisplayPath", wrapGetDisplayStatus, orgGetDisplayStatus},
		{"__ZN21AppleIntelFramebuffer13GetOnlineInfoEP21AppleIntelDisplayPathPhS2_PNS0_15DisplayPortTypeEPbb", wrapGetOnlineInfo, orgGetOnlineInfo},
		{"__ZN21AppleIntelFramebuffer15doSetPowerStateEj", wrapDoSetPowerState, orgDoSetPowerState},
		{"__ZN21AppleIntelFramebuffer18IsMultiLinkDisplayEv", wrapIsMultilinkDisplay, orgIsMultilinkDisplay},
		{"__ZN21AppleIntelFramebuffer19validateDisplayModeEiPPKNS_15ModeDescriptionEPPK29IODetailedTimingInformationV2", wrapValidateDisplayMode, orgValidateDisplayMode},
		{"__ZN31AppleIntelFramebufferController18hasExternalDisplayEv", wrapHasExternalDisplay, orgHasExternalDisplay},
		{"__ZN31AppleIntelFramebufferController15SetDPPowerStateEP21AppleIntelFramebufferhP21AppleIntelDisplayPath", wrapSetDPPowerState, orgSetDPPowerState},
		{"__ZN31AppleIntelFramebufferController14setDisplayPipeEP21AppleIntelDisplayPath", wrapSetDisplayPipe, orgSetDisplayPipe},
		{"__ZN31AppleIntelFramebufferController11setFBMemoryEP21AppleIntelFramebuffer", wrapSetFBMemory, orgSetFBMemory},
		{"__ZN20IntelFBClientControl11doAttributeEjPmmS0_S0_P25IOExternalMethodArguments", wrapFBClientDoAttribute, orgFBClientDoAttribute},
	};

	if (!patcher.routeMultiple(index, requests, address, size, true, true))
		SYSLOG("igfx", "failed to route igfx tracing");
}

#else
void IGFX::loadFramebufferDebug(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	PANIC("igfx", "fb debug is a debug-only feature");
}
#endif
