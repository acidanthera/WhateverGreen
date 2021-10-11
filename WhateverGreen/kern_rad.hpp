//
//  kern_rad.hpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_rad_hpp
#define kern_rad_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include "kern_agdc.hpp"
#include "kern_atom.hpp"
#include "kern_con.hpp"

class RAD {
public:
	void init(bool enableNavi10Bkl = false);
	void deinit();

	/**
	 *  AMD Hardware kext index
	 */
	enum HardwareIndex {
		IndexRadeonHardwareX4000,
		IndexRadeonHardwareX5000,
		IndexRadeonHardwareX6000,
		IndexRadeonHardwareX3000,
		IndexRadeonHardwareX4100,
		IndexRadeonHardwareX4150,
		IndexRadeonHardwareX4200,
		IndexRadeonHardwareX4250,
		MaxRadeonHardware,
		MaxRadeonHardwareCatalina = IndexRadeonHardwareX6000 + 1,
		MaxRadeonHardwareMojave = IndexRadeonHardwareX5000 + 1,
		MaxRadeonHardwareModernHighSierra = IndexRadeonHardwareX3000 + 1
	};

	/**
	 *  Property patching routine
	 *
	 *  @param patcher  KernelPatcher instance
	 *  @param info     device info
	 */
	void processKernel(KernelPatcher &patcher, DeviceInfo *info);

	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 *
	 *  @return true if patched anything
	 */
	bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

private:
	/**
	 *  Private self instance for callbacks
	 */
	static RAD *callbackRAD;

	/**
	 *  Original set property function
	 */
	mach_vm_address_t orgSetProperty {};

	/**
	 *  Original get property function
	 */
	mach_vm_address_t orgGetProperty {};

	/**
	 *  Original connector info functions
	 */
	mach_vm_address_t orgGetConnectorsInfoV1 {};
	mach_vm_address_t orgGetConnectorsInfoV2 {};
	mach_vm_address_t orgLegacyGetConnectorsInfo {};

	/**
	 *  Get atom object table function type
	 */
	using t_getAtomObjectTableForType = void *(*)(void *that, AtomObjectTableType type, uint8_t *sz);

	/**
	 *  Original get atom object table functions
	 */
	t_getAtomObjectTableForType orgGetAtomObjectTableForType {nullptr};
	t_getAtomObjectTableForType orgLegacyGetAtomObjectTableForType {nullptr};

	/**
	 *  Original translate atom connector into modern connector function
	 */
	mach_vm_address_t orgTranslateAtomConnectorInfoV1 {};
	mach_vm_address_t orgTranslateAtomConnectorInfoV2 {};

	/**
	 *  Original controller start functions
	 */
	mach_vm_address_t orgATIControllerStart {};
	mach_vm_address_t orgLegacyATIControllerStart {};

	/**
	 *  Original AGDP handler
	 */
	mach_vm_address_t orgNotifyLinkChange {};

	/**
	 *  Current controller property provider, 8 for max GPUs at once.
	 */
	ThreadLocal<IOService *, 8> currentPropProvider;
	ThreadLocal<IOService *, 8> currentLegacyPropProvider;

	/**
	 *  Original populateAccelConfig functions
	 */
	mach_vm_address_t orgPopulateAccelConfig[MaxRadeonHardware] {};

	/**
	 *  Original getHWInfo functions
	 */
	mach_vm_address_t orgGetHWInfo[MaxRadeonHardware] {};

	/**
	 *  Max framebuffer base functions per kext
	 */
	static constexpr size_t MaxGetFrameBufferProcs = 3;
	
	/**
	 *  Store the current backlight level of Amd Navi10 pwm backlight control
	 */
	uint32_t curPwmBacklightLvl = 0;
	
	/**
	 *  Store the maximum backlight level of Amd Navi10 pwm backlight control
	 *  0xff7b is the max brightness from intel CFL backlight panel data, this value will be override in updatePwmMaxBrightnessFromInternalDisplay
	 */
	uint32_t maxPwmBacklightLvl = 0xff7b;
	
	/**
	 *  Store the panel_cntl pointer of Amd Navi10 pwm backlight control, will use it when set backlight level
	 */
	void* panelCntlPtr = NULL;
	
	/**
	 *  Read maximum brightness from the property of AppleBacklightDisplay
	 */
	void updatePwmMaxBrightnessFromInternalDisplay();
	
	/**
	 *  Prototype of orgDceDriverSetBacklight
	 */
	using t_DceDriverSetBacklight = void (*)(void *panel_cntl, uint32_t backlight_pwm_u16_16);
	
	/**
	 *  Original dce_driver_set_backlight functions
	 *  Use it to set brightness level of Amd Navi10 pwm backlight
	 */
	t_DceDriverSetBacklight orgDceDriverSetBacklight {nullptr};
	
	/**
	 *  Original dce_panel_cntl_hw_init functions
	 *  Use it to get the panel_cntl pointer
	 */
	mach_vm_address_t orgDcePanelCntlHwInit {};
	
	/**
	 *  Original AMDRadeonX6000_AmdRadeonFramebuffer::SetAttribute functions
	 */
	mach_vm_address_t orgAMDRadeonX6000AmdRadeonFramebufferSetAttribute {};
	
	/**
	 *  Original AMDRadeonX6000_AmdRadeonFramebuffer::GetAttribute functions
	 */
	mach_vm_address_t orgAMDRadeonX6000AmdRadeonFramebufferGetAttribute {};
	
	/**
	 *  Wrapped dce_panel_cntl_hw_init function to get the panel_cntl pointor
	 */
	static uint32_t wrapDcePanelCntlHwInit(void *panel_cntl) ;
	
	/**
	 *  Wrapped AMDRadeonX6000_AmdRadeonFramebuffer::SetAttribute functions
	 */
	static IOReturn wrapAMDRadeonX6000AmdRadeonFramebufferSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t value);
	
	/**
	 *  Wrapped AMDRadeonX6000_AmdRadeonFramebuffer::GetAttribute functions
	 */
	static IOReturn wrapAMDRadeonX6000AmdRadeonFramebufferGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t * value);

	/**
	 *  Framebuffer base function names
	 */
	const char *getFrameBufferProcNames[MaxRadeonHardware][MaxGetFrameBufferProcs] {
		[IndexRadeonHardwareX3000] = {
			"__ZN15AMDR8xxHardware25getFrameBufferBaseAddressEv"
		},
		[IndexRadeonHardwareX4000] = {
			"__ZN13AMDSIHardware25getFrameBufferBaseAddressEv",
			"__ZN13AMDCIHardware25getFrameBufferBaseAddressEv",
			"__ZN28AMDRadeonX4000_AMDVIHardware25getFrameBufferBaseAddressEv"
		},
		[IndexRadeonHardwareX4100] = {
			"__ZN28AMDRadeonX4100_AMDVIHardware25getFrameBufferBaseAddressEv"
		},
		[IndexRadeonHardwareX4150] = {
			"__ZN28AMDRadeonX4150_AMDVIHardware25getFrameBufferBaseAddressEv"
		},
		[IndexRadeonHardwareX4200] = {
			"__ZN28AMDRadeonX4200_AMDVIHardware25getFrameBufferBaseAddressEv"
		},
		[IndexRadeonHardwareX4250] = {
			"__ZN28AMDRadeonX4250_AMDVIHardware25getFrameBufferBaseAddressEv"
		},
	};

	/**
	 *  populateAccelConfig function type
	 */
	using t_populateAccelConfig = void (*)(IOService *accelService, const char **accelConfig);

	/**
	 *  Wrapped populateAccelConfig functions
	 */
	t_populateAccelConfig wrapPopulateAccelConfig[MaxRadeonHardware] {
		[RAD::IndexRadeonHardwareX3000] = populdateAccelConfig<RAD::IndexRadeonHardwareX3000>,
		[RAD::IndexRadeonHardwareX4000] = populdateAccelConfig<RAD::IndexRadeonHardwareX4000>,
		[RAD::IndexRadeonHardwareX4100] = populdateAccelConfig<RAD::IndexRadeonHardwareX4100>,
		[RAD::IndexRadeonHardwareX4150] = populdateAccelConfig<RAD::IndexRadeonHardwareX4150>,
		[RAD::IndexRadeonHardwareX4200] = populdateAccelConfig<RAD::IndexRadeonHardwareX4200>,
		[RAD::IndexRadeonHardwareX4250] = populdateAccelConfig<RAD::IndexRadeonHardwareX4250>,
		[RAD::IndexRadeonHardwareX5000] = populdateAccelConfig<RAD::IndexRadeonHardwareX5000>
	};

	/**
	 *  Register read function names
	 */
	const char *populateAccelConfigProcNames[MaxRadeonHardware] {
		[RAD::IndexRadeonHardwareX3000] = "__ZN37AMDRadeonX3000_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
		[RAD::IndexRadeonHardwareX4000] = "__ZN37AMDRadeonX4000_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
		[RAD::IndexRadeonHardwareX4100] = "__ZN37AMDRadeonX4100_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
		[RAD::IndexRadeonHardwareX4150] = "__ZN37AMDRadeonX4150_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
		[RAD::IndexRadeonHardwareX4200] = "__ZN37AMDRadeonX4200_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
		[RAD::IndexRadeonHardwareX4250] = "__ZN37AMDRadeonX4250_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig",
		[RAD::IndexRadeonHardwareX5000] = "__ZN37AMDRadeonX5000_AMDGraphicsAccelerator19populateAccelConfigEP13IOAccelConfig"
	};

	/**
	 *  getHWInfo function type
	 */
	using t_getHWInfo = IOReturn (*)(IOService *accelVideoCtx, void *hwInfo);

	/**
	 *  getHWInfo wrapping functions used for AppleGVA enable patch
	 */
	template <size_t Index>
	static IOReturn populateGetHWInfo(IOService *accelVideoCtx, void *hwInfo) {
		if (callbackRAD->orgGetHWInfo[Index]) {
			int ret = FunctionCast(populateGetHWInfo<Index>, callbackRAD->orgGetHWInfo[Index])(accelVideoCtx, hwInfo);
			callbackRAD->updateGetHWInfo(accelVideoCtx, hwInfo);
			return ret;
		} else {
			SYSLOG("rad", "populateGetHWInfo invalid use for %lu", Index);
		}
		return kIOReturnInvalid;
	}

	/**
	 *  Wrapped getHWInfo functions
	 */
	t_getHWInfo wrapGetHWInfo[MaxRadeonHardware] {
		[RAD::IndexRadeonHardwareX4000] = populateGetHWInfo<RAD::IndexRadeonHardwareX4000>,
		[RAD::IndexRadeonHardwareX5000] = populateGetHWInfo<RAD::IndexRadeonHardwareX5000>,
		[RAD::IndexRadeonHardwareX6000] = populateGetHWInfo<RAD::IndexRadeonHardwareX6000>
	};


	/**
	 *  Register read function names
	 */
	const char *getHWInfoProcNames[MaxRadeonHardware] {
		[RAD::IndexRadeonHardwareX4000] = "__ZN35AMDRadeonX4000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo",
		[RAD::IndexRadeonHardwareX5000] = "__ZN35AMDRadeonX5000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo",
		[RAD::IndexRadeonHardwareX6000] = "__ZN35AMDRadeonX6000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo"
	};

	/**
	 *  Enforce 24-bit output
	 */
	bool force24BppMode {false};

	/**
	 *  Take AGDP decisions on our own
	 */
	bool useCustomAgdpDecision {false};

	/**
	 *  Limit DVI resolution to single link
	 */
	bool dviSingleLink {false};

	/**
	 *  Disable Metal support (and force OpenGL)
	 */
	bool forceOpenGL {false};

	/**
	 *  Report real GPU name in Accelerator config
	 */
	bool fixConfigName {false};

	/**
	 *  Enable gva decoding and encoding support
	 */
	bool enableGvaSupport {false};

	/**
	 *  Boot ATI/AMD graphics without acceleration
	 */
	bool forceVesaMode {false};

	/**
	 *  Force getHWInfo call to return spoofed PID for codec support
	 */
	bool forceCodecInfo {false};

	/**
	 *  Current max used hardware kexts
	 */
	size_t maxHardwareKexts {MaxRadeonHardware};

	/**
	 *  Configure available kexts for different OS
	 */
	void initHardwareKextMods();

	/**
	 *  Merge property with a correct type from ioreg
	 *
	 *  @param props  target dictionary with original properties
	 *  @param name   property name
	 *  @param value  property value
	 */
	void mergeProperty(OSDictionary *props, const char *name, OSObject *value);

	/**
	 *  Merge configuration properties from ioreg
	 *
	 *  @param props     target dictionary with original properties
	 *  @param prefix    property name prefix in provider
	 *  @param provider  property provider for merging
	 */
	void mergeProperties(OSDictionary *props, const char *prefix, IOService *provider);

	/**
	 *  Automatically add properties to fix various bugs
	 *
	 *  @param service       gpu controller
	 *  @param connectorNum  number of connectors
	 */
	void applyPropertyFixes(IOService *service, uint32_t connectorNum=0);

	/**
	 *  Refresh connectors with the provided ones if necessary
	 *
	 *  @param atomutils  AtiAtomBiosUtilities instance
	 *  @param gettable   relevant atom object table getting function
	 *  @param ctrl       ATIController service
	 *  @param connectors autodetected controllers
	 *  @param sz         number of autodetected controllers
	 */
	void updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl, RADConnectors::Connector *connectors, uint8_t *sz);

	/**
	 *  Apply various fixes to automatically detected connectors
	 *
	 *  @param baseAddr            atom object offset base address
	 *  @param displayPaths        pointer to atom display object path table
	 *  @param connectorObjects    pointer to connector objects table
	 *  @param connectorObjectNum  number of elements in connectorObjects
	 *  @param connectors          pointer to autodetected connectors
	 *  @param sz                  number of autodetected connectors
	 */
	void autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum, AtomConnectorObject *connectorObjects,
							   uint8_t connectorObjectNum, RADConnectors::Connector *connectors, uint8_t sz);

	/**
	 *  Actually correct a certain found connector
	 *
	 *  @param connector   connector id
	 *  @param sense       sense id
	 *  @param txmit       transmitter
	 *  @param enc         encoder
	 *  @param connectors  pointer to autodetected connectors
	 *  @param sz          number of autodetected connectors
	 */
	void autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit, uint8_t enc, RADConnectors::Connector *connectors, uint8_t sz);

	/**
	 *  Changes connector priority according to provided sense id list
	 *
	 *  @param senseList   list of sense ids in ascending order
	 *  @param senseNum    number of sense ids in the list
	 *  @param connectors  pointer to autodetected connectors
	 *  @param sz          number of autodetected connectors
	 */
	void reprioritiseConnectors(const uint8_t *senseList, uint8_t senseNum, RADConnectors::Connector *connectors, uint8_t sz);

	/**
	 *  populateAccelConfig wrapping functions used for accelerator name correction
	 */
	template <size_t Index>
	static void populdateAccelConfig(IOService *accelService, const char **accelConfig) {
		if (callbackRAD->orgPopulateAccelConfig[Index]) {
			FunctionCast(populdateAccelConfig<Index>, callbackRAD->orgPopulateAccelConfig[Index])(accelService, accelConfig);
			callbackRAD->updateAccelConfig(Index, accelService, accelConfig);
		} else {
			SYSLOG("rad", "populdateAccelConfig invalid use for %lu", Index);
		}
	}

	/**
	 *  Enable forced 24-bit output
	 *
	 *  @param patcher  kernel patcher instance
	 *  @param info     loaded kinfo of the right framebuffer
	 *  @param address  kinfo load address
	 *  @param size     kinfo memory size
	 */
	void process24BitOutput(KernelPatcher &patcher, KernelPatcher::KextInfo &info, mach_vm_address_t address, size_t size);

	/**
	 *  Apply connector modifications (for support kexts)
	 *
	 *  @param patcher  kernel patcher instance
	 *  @param address  kinfo load address
	 *  @param size     kinfo memory size
	 *  @param modern   legacy or normal kext
	 */
	void processConnectorOverrides(KernelPatcher &patcher, mach_vm_address_t address, size_t size, bool modern);

	/**
	 *  Apply hardware kext modifications (X3000~X5000)
	 *
	 *  @param patcher  kernel patcher instance
	 *  @param hwIndex  hardware kext index
	 *  @param address  kinfo load address
	 *  @param size     kinfo memory size
	 */
	void processHardwareKext(KernelPatcher &patcher, size_t hwIndex, mach_vm_address_t address, size_t size);

	/**
	 *  Update IOAccelConfig with a GVA properties
	 *
	 *  @param accelService IOAccelerator service
	 */
	void setGvaProperties(IOService *accelService);

	/**
	 *  Update IOAccelConfig with a real GPU model name
	 *
	 *  @param hwIndex  hardware kext index
	 *  @param accelService IOAccelerator service
	 *  @param accelConfig  IOAccelConfig
	 */
	void updateAccelConfig(size_t hwIndex, IOService *accelService, const char **accelConfig);

	/**
	 *  Wrapped set property function
	 */
	static bool wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned length);

	/**
	 *  Wrapped get property function
	 */
	static OSObject *wrapGetProperty(IORegistryEntry *that, const char *aKey);

	/**
	 *  Wrapped get connectors info functions
	 */
	static uint32_t wrapGetConnectorsInfoV1(void *that, RADConnectors::Connector *connectors, uint8_t *sz);
	static uint32_t wrapGetConnectorsInfoV2(void *that, RADConnectors::Connector *connectors, uint8_t *sz);
	static uint32_t wrapLegacyGetConnectorsInfo(void *that, RADConnectors::Connector *connectors, uint8_t *sz);

	/**
	 *  Wrapped translate atom connector into modern connector functions
	 */
	static uint32_t wrapTranslateAtomConnectorInfoV1(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector);
	static uint32_t wrapTranslateAtomConnectorInfoV2(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector);

	/**
	 *  Wrapped ATIController start functions
	 */
	static bool wrapATIControllerStart(IOService *ctrl, IOService *provider);
	static bool wrapLegacyATIControllerStart(IOService *ctrl, IOService *provider);

	/**
	 * Wrapped AGDP handler
	 */
	static bool wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData, uint32_t eventFlags);

	/**
	 *  Wrapped polaris controller project creator
	 */
	static IOReturn findProjectByPartNumber(IOService *ctrl, void *properties);

	/**
	 *  Wrapped VRAM testing method
	 */
	static bool doNotTestVram(IOService *ctrl, uint32_t reg, bool retryOnFail);
	
	/**
	 *  Wrapped codec hw info method
	 */
	static void updateGetHWInfo(IOService *accelVideoCtx, void *hwInfo);
};

#endif /* kern_rad_hpp */
