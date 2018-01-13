//
//  kern_rad.hpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_rad_hpp
#define kern_rad_hpp

#include <Headers/kern_patcher.hpp>
#include <Library/LegacyIOService.h>
#include "kern_atom.hpp"
#include "kern_con.hpp"

class RAD {
public:
	bool init();
	void deinit();
	
	/**
	 *  GPU hardware list kept in sync with symbol names and progress mask
	 */
	struct HardwareIndex {
		enum : uint32_t {
			X3000,
			X4000,
			X4100,
			X4150,
			X4200,
			X4250,
			X5000,
			Total
		};
	};

	/**
	 *  PCI GPU Vendor identifiers
	 */
	struct VendorID {
		enum : uint16_t {
			ATIAMD = 0x1002,
			NVIDIA = 0x10de,
			Intel = 0x8086
		};
	};

	/**
	 *  Console info structure, taken from osfmk/console/video_console.h
	 *  Last updated from XNU 4570.1.46.
	 */
	struct vc_info {
		unsigned int   v_height;        /* pixels */
		unsigned int   v_width;         /* pixels */
		unsigned int   v_depth;
		unsigned int   v_rowbytes;
		unsigned long  v_baseaddr;
		unsigned int   v_type;
		char           v_name[32];
		uint64_t       v_physaddr;
		unsigned int   v_rows;          /* characters */
		unsigned int   v_columns;       /* characters */
		unsigned int   v_rowscanbytes;  /* Actualy number of bytes used for display per row*/
		unsigned int   v_scale;
		unsigned int   v_rotate;
		unsigned int   v_reserved[3];
	};

	/**
	 *  Framebuffer initialisation function type
	 */
	using t_framebufferInit = void (*)(void *);
	
	/**
	 *  Set property function type
	 */
	using t_setProperty =  bool (*)(IORegistryEntry *that, const char *aKey, void *bytes, unsigned intlength);
	
	/**
	 *  Get property function type
	 */
	using t_getProperty =  OSObject *(*)(IORegistryEntry *that, const char *aKey);
	
	/**
	 *  Get connectors info function type
	 */
	using t_getConnectorsInfo = uint32_t (*)(void *that, RADConnectors::Connector *connectors, uint8_t *sz);
	
	/**
	 *  Get atom object table function type
	 */
	using t_getAtomObjectTableForType = void *(*)(void *that, AtomObjectTableType type, uint8_t *sz);
	
	/**
	 *  Translate atom connector into modern connector function type
	 */
	using t_translateAtomConnectorInfo = uint32_t (*)(void *that, RADConnectors::AtomConnectorInfo *info, RADConnectors::Connector *connector);
	
	/**
	 *  ATIController start function type
	 */
	using t_controllerStart = bool (*)(IOService *ctrl, IOService *provider);
	
	/**
	 *  Populate Accel config function type
	 */
	using t_populateAccelConfig = void (*)(IOService *accelService, const char **accelConfig);

	/**
	 *  Get device type for later decisions
	 */
	using t_getDeviceType = uint32_t (*)(IOService *accelService, void *pcidev);

	/**
	 *  Update IOAccelConfig with a real GPU model name
	 *
	 *  @param accelService IOAccelerator service
	 *  @param accelConfig  IOAccelConfig
	 */
	static void updateAccelConfig(IOService *accelService, const char **accelConfig);
	
private:
	/**
	 *  Wrapped framebuffer initialisation function
	 */
	static void wrapFramebufferInit(void *that);
	
	/**
	 *  Wrapped set property function
	 */
	static bool wrapSetProperty(IORegistryEntry *that, const char *aKey, void *bytes, unsigned intlength);
	
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
	 *  Wrapped get device type functions for enforcing X4200 driver on 10.13
	 */
	static uint32_t wrapGetDeviceTypeBaffin(IOService *accelService, void *pcidev);
	static uint32_t wrapGetDeviceTypeEllesmere(IOService *accelService, void *pcidev);

	/**
	 *  Merge configuration properties from ioreg
	 *
	 *  @param props     target dictionary with original properties
	 *  @param prefix    property name prefix in provider
	 *  @param provider  property provider for merging
	 */
	static void mergeProperties(OSDictionary *props, const char *prefix, IOService *provider);
	
	/**
	 *  Refresh connectors with the provided ones if necessary
	 *
	 *  @param atomutils  AtiAtomBiosUtilities instance
	 *  @param gettable   relevant atom object table getting function
	 *  @param ctrl       ATIController service
	 *  @param connectors autodetected controllers
	 *  @param sz         number of autodetected controllers
	 */
	static void updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl, RADConnectors::Connector *connectors, uint8_t *sz);
	
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
	static void autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum, AtomConnectorObject *connectorObjects,
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
	static void autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit, uint8_t enc, RADConnectors::Connector *connectors, uint8_t sz);
	
	/**
	 *  Changes connector priority according to provided sense id list
	 *
	 *  @param senseList   list of sense ids in ascending order
	 *  @param senseNum    number of sense ids in the list
	 *  @param connectors  pointer to autodetected connectors
	 *  @param sz          number of autodetected connectors
	 */
	static void reprioritiseConnectors(const uint8_t *senseList, uint8_t senseNum, RADConnectors::Connector *connectors, uint8_t sz);
	
	/**
	 *  Attempts to find a printable name of the GPU
	 *
	 *  @param ven    vendor-id
	 *  @param dev    devide-id
	 *  @param rev    revision-id
	 *  @param subven subsystem-vendor-id
	 *  @param sub    susbsytem-id
	 *
	 *  @return autodetected GPU name or nullptr
	 */
	static const char *getModel(uint16_t ven, uint16_t dev, uint16_t rev, uint16_t subven, uint16_t sub);
	
	/**
	 *  Updates ATI/AMD EFIVersion to avoid confusing different kext versions
	 *
	 *  @param controller  gpu controller
	 */
	static void reportVersion(IOService *controller);
	
	/**
	 *  Configure available kexts for different OS
	 */
	void osCompat();
	
	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
	
	/**
	 *  Property patching routine
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processProperties(KernelPatcher &patcher);

	/**
	 *  Obtain the necessary data for logo fixup
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processScreenFlicker(KernelPatcher &patcher);

	/**
	 *  Current progress mask
	 */
	struct ProcessingState {
		enum : uint32_t {
			NothingReady = 0,
			X3000Hardware = 1,
			X4000Hardware = 2,
			X4100Hardware = 4,
			X4150Hardware = 8,
			X4200Hardware = 16,
			X4250Hardware = 32,
			X5000Hardware = 64,
			RegisterPatch = X3000Hardware | X4000Hardware | X4100Hardware | X4150Hardware | X4200Hardware | X4250Hardware | X5000Hardware,
			BootLogo = 128,
			FramebufferLegacy = 256,
			FramebufferNew = 512,
			BlinkingHell = FramebufferLegacy | FramebufferNew,
			DisablePowerGating = 1024,
			PatchConnectors = 2048,
			PatchLegacyConnectors = 4096,
			EverythingDone = BlinkingHell | BootLogo | RegisterPatch | DisablePowerGating | PatchConnectors | PatchLegacyConnectors
		};
	};
	uint32_t progressState {ProcessingState::NothingReady};
	
	/**
	 *  Hardware index to progress mask
	 */
	constexpr uint32_t indexToMask(uint32_t index) {
		if (index < HardwareIndex::Total)
			return 1 << index;
		return 0;
	}
};

#endif /* kern_rad_hpp */
