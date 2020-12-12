//
//  kern_igfx_debug.cpp
//  WhateverGreen
//
//  Copyright © 2020 vit9696. All rights reserved.
//

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include "kern_agdc.hpp"
#include "kern_igfx.hpp"

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

static uint32_t fbdebugWrapGetDisplayStatus(IOService *framebuffer, void *displayPath) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "getDisplayStatus %d start", idx);
	uint32_t ret = FunctionCast(fbdebugWrapGetDisplayStatus, fbdebugOrgGetDisplayStatus)(framebuffer, displayPath);
	SYSLOG("igfx", "getDisplayStatus %d end - %u", idx, ret);
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
		{ kAGDCVendorInfo, "kAGDCVendorInfo" },
		{ kAGDCVendorEnableController, "kAGDCVendorEnableController" },
		{ kAGDCPMInfo, "kAGDCPMInfo" },
		{ kAGDCPMStateCeiling, "kAGDCPMStateCeiling" },
		{ kAGDCPMStateFloor, "kAGDCPMStateFloor" },
		{ kAGDCPMPState, "kAGDCPMPState" },
		{ kAGDCPMPowerLimit, "kAGDCPMPowerLimit" },
		{ kAGDCPMGetGPUInfo, "kAGDCPMGetGPUInfo" },
		{ kAGDCPMGetPStateFreqTable, "kAGDCPMGetPStateFreqTable" },
		{ kAGDCPMGetPStateResidency, "kAGDCPMGetPStateResidency" },
		{ kAGDCPMGetCStateNames, "kAGDCPMGetCStateNames" },
		{ kAGDCPMGetCStateResidency, "kAGDCPMGetCStateResidency" },
		{ kAGDCPMGetMiscCntrNum, "kAGDCPMGetMiscCntrNum" },
		{ kAGDCPMGetMiscCntrInfo, "kAGDCPMGetMiscCntrInfo" },
		{ kAGDCPMGetMiscCntr, "kAGDCPMGetMiscCntr" },
		{ kAGDCPMTakeCPStateResidencySnapshot, "kAGDCPMTakeCPStateResidencySnapshot" },
		{ kAGDCPMGetPStateResidencyDiff, "kAGDCPMGetPStateResidencyDiff" },
		{ kAGDCPMGetCStateResidencyDiff, "kAGDCPMGetCStateResidencyDiff" },
		{ kAGDCPMGetPStateResidencyDiffAbs, "kAGDCPMGetPStateResidencyDiffAbs" },
		{ kAGDCFBPerFramebufferCMD, "kAGDCFBPerFramebufferCMD" },
		{ kAGDCFBOnline, "kAGDCFBOnline" },
		{ kAGDCFBSetEDID, "kAGDCFBSetEDID" },
		{ kAGDCFBSetMode, "kAGDCFBSetMode" },
		{ kAGDCFBInjectEvent, "kAGDCFBInjectEvent" },
		{ kAGDCFBDoControl, "kAGDCFBDoControl" },
		{ kAGDCFBDPLinkConfig, "kAGDCFBDPLinkConfig" },
		{ kAGDCFBSetEDIDEx, "kAGDCFBSetEDIDEx" },
		{ kAGDCFBGetCapability, "kAGDCFBGetCapability" },
		{ kAGDCFBGetCapabilityEx, "kAGDCFBGetCapabilityEx" },
		{ kAGDCMultiLinkConfig, "kAGDCMultiLinkConfig" },
		{ kAGDCLinkConfig, "kAGDCLinkConfig" },
		{ kAGDCRegisterCallback, "kAGDCRegisterCallback" },
		{ kAGDCGetPortStatus, "kAGDCGetPortStatus" },
		{ kAGDCConfigureAudio, "kAGDCConfigureAudio" },
		{ kAGDCCallbackCapability, "kAGDCCallbackCapability" },
		{ kAGDCStreamSleepControl, "kAGDCStreamSleepControl" },
		{ kAGDCPortEnable, "kAGDCPortEnable" },
		{ kAGDCPortCapability, "kAGDCPortCapability" },
		{ kAGDCDiagnoseGetDevicePropertySize, "kAGDCDiagnoseGetDevicePropertySize" },
		{ kAGDCDiagnoseGetDeviceProperties, "kAGDCDiagnoseGetDeviceProperties" },
		{ kAGDCGPUCapability, "kAGDCGPUCapability" },
		{ kAGDCStreamAssociate, "kAGDCStreamAssociate" },
		{ kAGDCStreamRequest, "kAGDCStreamRequest" },
		{ kAGDCStreamAccessI2C, "kAGDCStreamAccessI2C" },
		{ kAGDCStreamAccessI2CCapability, "kAGDCStreamAccessI2CCapability" },
		{ kAGDCStreamAccessAUX, "kAGDCStreamAccessAUX" },
		{ kAGDCStreamGetEDID, "kAGDCStreamGetEDID" },
		{ kAGDCStreamSetState, "kAGDCStreamSetState" },
		{ kAGDCStreamConfig, "kAGDCStreamConfig" },
		{ kAGDCEnableController, "kAGDCEnableController" },
		{ kAGDCTrainingBegin, "kAGDCTrainingBegin" },
		{ kAGDCTrainingAttempt, "kAGDCTrainingAttempt" },
		{ kAGDCTrainingEnd, "kAGDCTrainingEnd" },
		{ kAGDCTestConfiguration, "kAGDCTestConfiguration" },
		{ kAGDCCommitConfiguration, "kAGDCCommitConfiguration" },
		{ kAGDCReleaseConfiguration, "kAGDCReleaseConfiguration" },
		{ kAGDCPluginMetricsPlug, "kAGDCPluginMetricsPlug" },
		{ kAGDCPluginMetricsUnPlug, "kAGDCPluginMetricsUnPlug" },
		{ kAGDCPluginMetricsHPD, "kAGDCPluginMetricsHPD" },
		{ kAGDCPluginMetricsSPI, "kAGDCPluginMetricsSPI" },
		{ kAGDCPluginMetricsSyncLT, "kAGDCPluginMetricsSyncLT" },
		{ kAGDCPluginMetricsSyncLTEnd, "kAGDCPluginMetricsSyncLTEnd" },
		{ kAGDCPluginMetricsLTBegin, "kAGDCPluginMetricsLTBegin" },
		{ kAGDCPluginMetricsLTEnd, "kAGDCPluginMetricsLTEnd" },
		{ kAGDCPluginMetricsDisplayInfo, "kAGDCPluginMetricsDisplayInfo" },
		{ kAGDCPluginMetricsMonitorInfo, "kAGDCPluginMetricsMonitorInfo" },
		{ kAGDCPluginMetricsLightUpDp, "kAGDCPluginMetricsLightUpDp" },
		{ kAGDCPluginMetricsHDCPStart, "kAGDCPluginMetricsHDCPStart" },
		{ kAGDCPluginMetricsFirstPhaseComplete, "kAGDCPluginMetricsFirstPhaseComplete" },
		{ kAGDCPluginMetricsLocalityCheck, "kAGDCPluginMetricsLocalityCheck" },
		{ kAGDCPluginMetricsRepeaterAuthenticatio, "kAGDCPluginMetricsRepeaterAuthenticatio" },
		{ kAGDCPluginMetricsHDCPEncryption, "kAGDCPluginMetricsHDCPEncryption" },
		{ kAGDCPluginMetricsHPDSinktoTB, "kAGDCPluginMetricsHPDSinktoTB" },
		{ kAGDCPluginMetricsHPDTBtoGPU, "kAGDCPluginMetricsHPDTBtoGPU" },
		{ kAGDCPluginMetricsVersion, "kAGDCPluginMetricsVersion" },
		{ kAGDCPluginMetricsGetMetricInfo, "kAGDCPluginMetricsGetMetricInfo" },
		{ kAGDCPluginMetricsGetMetricData, "kAGDCPluginMetricsGetMetricData" },
		{ kAGDCPluginMetricsMarker, "kAGDCPluginMetricsMarker" },
		{ kAGDCPluginMetricsGetMessageTracer, "kAGDCPluginMetricsGetMessageTracer" },
		{ kAGDCPluginMetricsXgDiscovery, "kAGDCPluginMetricsXgDiscovery" },
		{ kAGDCPluginMetricsXgDriversStart, "kAGDCPluginMetricsXgDriversStart" },
		{ kAGDCPluginMetricsXgPublished, "kAGDCPluginMetricsXgPublished" },
		{ kAGDCPluginMetricsXgResetPort, "kAGDCPluginMetricsXgResetPort" },
		{ kAGDCPluginMetricsPowerOff, "kAGDCPluginMetricsPowerOff" },
		{ kAGDCPluginMetricsPowerOn, "kAGDCPluginMetricsPowerOn" },
		{ kAGDCPluginMetricsSPIData, "kAGDCPluginMetricsSPIData" },
		{ kAGDCPluginMetricsEFIData, "kAGDCPluginMetricsEFIData" },
	};

	const char *name = "<unknown>";
	for (auto &map : mapping) {
		if (attribute == map.attr) {
			name = map.name;
			break;
		}
	}

	//FIXME: we are just getting rid of AGDC.
	if (attribute == kAGDCRegisterCallback) {
		SYSLOG("igfx", "[HACK] FBClientDoAttribute -> disabling AGDC!");
		return kIOReturnUnsupported;
	}

	SYSLOG("igfx", "FBClientDoAttribute %s (%x) start", name, attribute);
	IOReturn ret = FunctionCast(fbdebugWrapFBClientDoAttribute, fbdebugOrgFBClientDoAttribute)(fbclient, attribute, unk1, unk2, unk3, unk4, externalMethodArguments);
	SYSLOG("igfx", "FBClientDoAttribute %s (%x) end - %x", name, attribute, ret);

	return ret;
}

void IGFX::FramebufferDebugSupport::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	SYSLOG("igfx", "using framebuffer debug r15");

	if (callbackIGFX->modAGDCDisabler.enabled)
		PANIC("igfx", "igfxagdc=0 is not compatible with framebuffer debugging");

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
		SYSLOG("igfx", "DBG: Failed to route igfx tracing.");
}

#else
void IGFX::FramebufferDebugSupport::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	PANIC("igfx", "DBG: Framebuffer debug support is only available in debug build.");
}
#endif

void IGFX::FramebufferDebugSupport::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingFramebuffer = true;
}

void IGFX::FramebufferDebugSupport::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
#ifdef DEBUG
	enabled = checkKernelArgument("-igfxfbdbg");
#endif
}
