//
//  kern_weg.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_weg_hpp
#define kern_weg_hpp

#include <Headers/kern_iokit.hpp>
#include <Headers/kern_devinfo.hpp>

#include "kern_cdf.hpp"
#include "kern_igfx.hpp"
#include "kern_ngfx.hpp"
#include "kern_rad.hpp"
#include "kern_shiki.hpp"
#include "kern_unfair.hpp"

class IOFramebuffer;
class IODisplay;

class WEG {
public:
	void init();
	void deinit();

	/**
	 *  Get overridable boot argument from kernel args (priority) and GPU properties
	 */
	static bool getVideoArgument(DeviceInfo *info, const char *name, void *bootarg, int size);

private:
	/**
	 *  Private self instance for callbacks
	 */
	static WEG *callbackWEG;

	/**
	 *  High resolution unlocker instances
	 */
	CDF cdf;

	/**
	 *  Intel GPU fixes instances
	 */
	IGFX igfx;

	/**
	 *  NVIDIA GPU fixes instance
	 */
	NGFX ngfx;

	/**
	 *  Radeon GPU fixes instance
	 */
	RAD rad;

	/**
	 *  Hardware acceleration and FairPlay fixes instance
	 */
	SHIKI shiki;

	/**
	 *  FairPlay fixes for modern operating systems
	 */
	UNFAIR unfair;

	/**
	 *  FB_DETECT   autodetects based on the installed GPU.
	 *  FB_RESET    enforces -v like usual patch.
	 *  FB_COPY     enforces screen copy (default on IGPU).
	 *  FB_ZEROFILL erases screen content (default on AMD).
	 *  FB_NONE     does nothing.
	 */
	enum FramebufferFixMode {
		FB_DETECT   = 0,
		FB_RESET    = 1,
		FB_COPY     = 2,
		FB_ZEROFILL = 3,
		FB_NONE     = 4,
		FB_TOTAL    = 5
	};

	/**
	 *  Framebuffer distortion fix mode
	 */
	uint32_t resetFramebuffer {FB_DETECT};

	/**
	 *  APPLBKL_OFF     disables AppleBacklight patches.
	 *  APPLBKL_ON      enforces AppleBacklight patches.
	 *  APPLBKL_DETECT  enables AppleBacklight patches for IGPU-only non-Apple setups.
	 *  APPLBKL_NAVI10  enables AppleBacklight patches for AMD Navi10 PWM backlight control.
	 */
	enum BacklightPatchMode {
		APPLBKL_OFF    = 0,
		APPLBKL_ON     = 1,
		APPLBKL_DETECT = 2,
		APPLBKL_NAVI10 = 3
	};

	/**
	 *  applbkl boot-arg controlled AppleBacklight kext patch
	 */
	uint32_t appleBacklightPatch {APPLBKL_DETECT};

	/**
	 *  applbkl custom device name if any
	 */
	OSData *appleBacklightCustomName {nullptr};

	/**
	 *  applbkl custom device data if any
	 */
	OSData *appleBacklightCustomData {nullptr};

	/**
	 *  Backlight panel data format
	 */
	struct ApplePanelData {
		const char *deviceName;
		uint8_t deviceData[36];
	};

	/**
	 *  Backlight panel data
	 */
	static ApplePanelData appleBacklightData[];

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
	 *  Loaded vinfo
	 */
	vc_info consoleVinfo {};

	/**
	 *  Console buffer backcopy
	 */
	uint8_t *consoleBuffer {nullptr};

	/**
	 *  Original IOGraphics framebuffer init handler
	 */
	mach_vm_address_t orgFramebufferInit {};

	/**
	 *  Verbose boot global variable pointer
	 */
	uint8_t *gIOFBVerboseBootPtr {nullptr};

	/**
	 *  Original IGPU PCI Config readers
	 */
	WIOKit::t_PCIConfigRead16 orgConfigRead16 {nullptr};
	WIOKit::t_PCIConfigRead32 orgConfigRead32 {nullptr};

	/**
	 *  Original AppleGraphicsDevicePolicy start handler
	 */
	mach_vm_address_t orgGraphicsPolicyStart {0};

	/**
	 *  Original AppleIntelPanel set display handler
	 */
	mach_vm_address_t orgApplePanelSetDisplay {0};

	/**
	 *  vinfo presence status
	 */
	bool applePanelDisplaySet {false};

	/**
	 *  vinfo presence status
	 */
	bool gotConsoleVinfo {false};
	
	/**
	 *  Device identification spoofing for IGPU
	 */
	bool hasIgpuSpoof {false};

	/**
	 *  Device identification spoofing for GFX0
	 */
	bool hasGfxSpoof {false};

	/**
	 *  Maximum GFX naming index (due to ACPI name restrictions)
	 */
	static constexpr uint8_t MaxExternalGfxIndex {9};

	/**
	 *  GPU index used for GFXx naming in IORegistry
	 *  Must be single digits (i.e. 0~9 inclusive).
	 */
	uint8_t currentExternalGfxIndex {0};

	/**
	 *  Maximum GFX slot naming index
	 *  Should be 1~4 to display properly in NVIDIA panel.
	 *  However, we permit more to match external GFX naming.
	 */
	static constexpr uint8_t MaxExternalSlotIndex {10};

	/**
	 *  GPU index used for AAPL,slot-name naming in IORegistry
	 *  Should be 1~4 to display properly in NVIDIA panel.
	 */
	uint8_t currentExternalSlotIndex {1};

	/**
	 *  AppleGraphicsDisplayPolicy modifications if applicable.
	 *
	 *  AGDP_NONE     no modifications
	 *  AGDP_DETECT   detect on firmware vendor and hardware installed
	 *  AGDP_VIT9696  null config string size at strcmp
	 *  AGDP_PIKERA   board-id -> board-ix replace
	 *  AGDP_CFGMAP   add board-id with none to ConfigMap
	 *  SET bit is used to distinguish from agpmod=detect.
	 */
	enum GraphicsDisplayPolicyMod {
		AGDP_SET        = 0x8000,
		AGDP_NONE_SET   = AGDP_SET | 0,
		AGDP_DETECT     = 1,
		AGDP_DETECT_SET = AGDP_SET | AGDP_DETECT,
		AGDP_VIT9696    = 2,
		AGDP_PIKERA     = 4,
		AGDP_CFGMAP     = 8,
		AGDP_PATCHES    = AGDP_VIT9696 | AGDP_PIKERA | AGDP_CFGMAP
	};

	/**
	 *  Current AppleGraphicsDisplayPolicy modifications.
	 */
	int graphicsDisplayPolicyMod {AGDP_DETECT};

	/**
	 *  Apply pre-kext patches and setup the configuration
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processKernel(KernelPatcher &patcher);

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
	 *  Apply builtin GPU properties and renamings
	 *
	 *  @param device  IGPU device
	 *  @param info    device information
	 */
	void processBuiltinProperties(IORegistryEntry *device, DeviceInfo *info);

	/**
	 *  Apply external GPU properties and renamings
	 *
	 *  @param device  GFX0 device
	 *  @param info    device information
	 */
	void processExternalProperties(IORegistryEntry *device, DeviceInfo *info, uint32_t vendor);

	/**
	 *  Apply IMEI properties and renamings
	 *
	 *  @param device  GFX0 device
	 *  @param info    device information
	 */
	void processManagementEngineProperties(IORegistryEntry *imei);

	/**
	 *  Parse AppleGraphicsDevicePolicy (AGDP) patch configuration
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param address agdp load address
	 *  @param size    agdp memory size
	 */
	void processGraphicsPolicyStr(const char *agdp);

	/**
	 *  Apply AppleGraphicsDevicePolicy (AGDP) patches if any
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param address agdp load address
	 *  @param size    agdp memory size
	 */
	void processGraphicsPolicyMods(KernelPatcher &patcher, mach_vm_address_t address, size_t size);

	/**
	 *  Check whether the graphics policy modification patches are required
	 *
	 *  @param info  device information
	 *
	 *  @return true if we should continue
	 */
	bool isGraphicsPolicyModRequired(DeviceInfo *info);

	/**
	 *  Attempts to find a printable name of an Intel GPU
	 *
	 *  @param dev      devide-id
	 *  @param fakeId   fake devide-id
	 *
	 *  @return autodetected GPU name or nullptr
	 */
	const char *getIntelModel(uint32_t dev, uint32_t &fakeId);

	/**
	 *  Attempts to find a printable name of a Radeon GPU
	 *
	 *  @param dev    devide-id
	 *  @param rev    revision-id
	 *  @param subven subsystem-vendor-id
	 *  @param sub    susbsytem-id
	 *
	 *  @return autodetected GPU name or nullptr
	 */
	const char *getRadeonModel(uint16_t dev, uint16_t rev, uint16_t subven, uint16_t sub);

	/**
	 *  IGPU PCI Config device-id faking wrappers
	 */
	static uint16_t wrapConfigRead16(IORegistryEntry *service, uint32_t space, uint8_t offset);
	static uint32_t wrapConfigRead32(IORegistryEntry *service, uint32_t space, uint8_t offset);

	/**
	 *  IOFramebuffer initialisation wrapper used for screen distortion fixes
	 *
	 *  @param fb  framebuffer instance
	 */
	static void wrapFramebufferInit(IOFramebuffer *fb);
	
	/**
	 *  wrapper for function that only return zero
	 *
	 */
	static size_t wrapFunctionReturnZero();

	/**
	 *  AppleGraphicsDevicePolicy start wrapper used for black screen fixes in AGDP_CFGMAP mode
	 *
	 *  @param that      agdp instance
	 *  @param provider  agdp provider
	 *
	 *  @return agdp start status
	 */
	static bool wrapGraphicsPolicyStart(IOService *that, IOService *provider);

	/**
	 *  AppleIntelPanel start wrapper used for extra panel injection
	 *
	 *  @param that      backlight panel instance
	 *  @param display  backlight panel display
	 *
	 *  @return backlight panel start status
	 */
	static bool wrapApplePanelSetDisplay(IOService *that, IODisplay *display);
};

#endif /* kern_weg_hpp */
