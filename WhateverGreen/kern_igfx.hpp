//
//  kern_igfx.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_igfx_hpp
#define kern_igfx_hpp

#include "kern_fb.hpp"
#include "kern_igfx_lspcon.hpp"

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <IOKit/IOService.h>
#include <IOKit/IOLocks.h>

class IGFX {
public:
	void init();
	void deinit();

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
	 *  Framebuffer patch flags
	 */
	union FramebufferPatchFlags {
		// Sandy+ bits
		struct FramebufferPatchFlagBits {
			uint8_t FPFFramebufferId            :1;
			uint8_t FPFModelNameAddr            :1;
			uint8_t FPFMobile                   :1;
			uint8_t FPFPipeCount                :1;
			uint8_t FPFPortCount                :1;
			uint8_t FPFFBMemoryCount            :1;
			uint8_t FPFStolenMemorySize         :1;
			uint8_t FPFFramebufferMemorySize    :1;
			uint8_t FPFUnifiedMemorySize        :1;
			uint8_t FPFFramebufferCursorSize    :1; // Haswell only
			uint8_t FPFFlags                    :1;
			uint8_t FPFBTTableOffsetIndexSlice  :1;
			uint8_t FPFBTTableOffsetIndexNormal :1;
			uint8_t FPFBTTableOffsetIndexHDMI   :1;
			uint8_t FPFCamelliaVersion          :1;
			uint8_t FPFNumTransactionsThreshold :1;
			uint8_t FPFVideoTurboFreq           :1;
			uint8_t FPFBTTArraySliceAddr        :1;
			uint8_t FPFBTTArrayNormalAddr       :1;
			uint8_t FPFBTTArrayHDMIAddr         :1;
			uint8_t FPFSliceCount               :1;
			uint8_t FPFEuCount                  :1;
		} bits;
		
		// Westmere bits
		struct FramebufferWestmerePatchFlagBits {
			uint8_t LinkWidth                               : 1;
			uint8_t SingleLink                              : 1;
			uint8_t FBCControlCompression                   : 1;
			uint8_t FeatureControlFBC                       : 1;
			uint8_t FeatureControlGPUInterruptHandling      : 1;
			uint8_t FeatureControlGamma                     : 1;
			uint8_t FeatureControlMaximumSelfRefreshLevel   : 1;
			uint8_t FeatureControlPowerStates               : 1;
			uint8_t FeatureControlRSTimerTest               : 1;
			uint8_t FeatureControlRenderStandby             : 1;
			uint8_t FeatureControlWatermarks                : 1;
		} bitsWestmere;
		
		uint32_t value;
	};

	/**
	 *  Connector patch flags
	 */
	union ConnectorPatchFlags {
		struct ConnectorPatchFlagBits {
			uint8_t CPFIndex        :1;
			uint8_t CPFBusId        :1;
			uint8_t CPFPipe         :1;
			uint8_t CPFType         :1;
			uint8_t CPFFlags        :1;
		} bits;
		uint32_t value;
	};

	/**
	 *  Framebuffer find / replace patch struct
	 */
	struct FramebufferPatch {
		uint32_t framebufferId;
		OSData *find;
		OSData *replace;
		size_t count;
	};

	/**
	 *  Framebuffer patching flags
	 */
	FramebufferPatchFlags framebufferPatchFlags {};

	/**
	 *  Connector patching flags
	 */
	ConnectorPatchFlags connectorPatchFlags[MaxFramebufferConnectorCount] {};

	/**
	 *  Framebuffer hard-code patch
	 */
	FramebufferICLLP framebufferPatch {};

	/**
	 *  Patch value for fCursorMemorySize in Haswell framebuffer
	 *  This member is not present in FramebufferCFL, hence its addition here.
	 */
	uint32_t fPatchCursorMemorySize;

	/**
	 *  Maximum find / replace patches
	 */
	static constexpr size_t MaxFramebufferPatchCount = 10;

	/**
	 *  Backlight registers
	 */
	static constexpr uint32_t BXT_BLC_PWM_CTL1 = 0xC8250;
	static constexpr uint32_t BXT_BLC_PWM_FREQ1 = 0xC8254;
	static constexpr uint32_t BXT_BLC_PWM_DUTY1 = 0xC8258;

	/**
	 *  Number of SNB frames in a framebuffer kext
	 */
	static constexpr size_t SandyPlatformNum = 9;

	/**
	 *  SNB frame ids in a framebuffer kext
	 */
	uint32_t sandyPlatformId[SandyPlatformNum] {
		0x00010000,
		0x00020000,
		0x00030010,
		0x00030030,
		0x00040000,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0x00030020,
		0x00050000
	};

	/**
	 *  Framebuffer find / replace patches
	 */
	FramebufferPatch framebufferPatches[MaxFramebufferPatchCount] {};
	
	/**
	 *  Framebuffer patches for first generation (Westmere).
	 */
	struct FramebufferWestmerePatches {
		uint32_t LinkWidth {1};
		uint32_t SingleLink {0};
		
		uint32_t FBCControlCompression {0};
		uint32_t FeatureControlFBC {0};
		uint32_t FeatureControlGPUInterruptHandling {0};
		uint32_t FeatureControlGamma {0};
		uint32_t FeatureControlMaximumSelfRefreshLevel {0};
		uint32_t FeatureControlPowerStates {0};
		uint32_t FeatureControlRSTimerTest {0};
		uint32_t FeatureControlRenderStandby {0};
		uint32_t FeatureControlWatermarks {0};
	};
	
	/**
	 *  Framebuffer patches for first generation (Westmere).
	 */
	FramebufferWestmerePatches framebufferWestmerePatches;

	/**
	 *  Framebuffer list, imported from the framebuffer kext
	 */
	void *gPlatformInformationList {nullptr};

	/**
	 *  Framebuffer list is in Sandy Bridge format
	 */
	bool gPlatformListIsSNB {false};

	/**
	 *  Private self instance for callbacks
	 */
	static IGFX *callbackIGFX;

	/**
	 *  Current graphics kext used for modification
	 */
	KernelPatcher::KextInfo *currentGraphics {nullptr};

	/**
	 *  Current framebuffer kext used for modification
	 */
	KernelPatcher::KextInfo *currentFramebuffer {nullptr};

	/**
	 *  Current framebuffer optional kext used for modification
	 */
	KernelPatcher::KextInfo *currentFramebufferOpt {nullptr};

	/**
	 *  Original PAVP session callback function used for PAVP command handling
	 */
	mach_vm_address_t orgPavpSessionCallback {};

	/**
	 *  Original AppleIntelFramebufferController::ComputeLaneCount function used for DP lane count calculation
	 */
	mach_vm_address_t orgComputeLaneCount {};

	/**
	 *  Original IOService::copyExistingServices function from the kernel
	 */
	mach_vm_address_t orgCopyExistingServices {};

	/**
	 *  Original IntelAccelerator::start function
	 */
	mach_vm_address_t orgAcceleratorStart {};

	/**
	 *  Original AppleIntelFramebufferController::getOSInformation function
	 */
	mach_vm_address_t orgGetOSInformation {};

	/**
	 *  Original IGHardwareGuC::loadGuCBinary function
	 */
	mach_vm_address_t orgLoadGuCBinary {};

	/**
	 *  Original IGScheduler4::loadFirmware function
	 */
	mach_vm_address_t orgLoadFirmware {};

	/**
	 *  Original IGHardwareGuC::initSchedControl function
	 */
	mach_vm_address_t orgInitSchedControl {};

	/**
	 *  Original IGSharedMappedBuffer::withOptions function
	 */
	mach_vm_address_t orgIgBufferWithOptions {};

	/**
	 *  Original IGMappedBuffer::getGPUVirtualAddress function
	 */
	mach_vm_address_t orgIgBufferGetGpuVirtualAddress {};

	/**
	 *  Original AppleIntelFramebufferController::hwRegsNeedUpdate function
	 */
	mach_vm_address_t orgHwRegsNeedUpdate {};

	/**
	 *  Original IntelFBClientControl::doAttribute function
	 */
	mach_vm_address_t orgFBClientDoAttribute {};

	/**
	 *  Original AppleIntelFramebuffer::getDisplayStatus function
	 */
	mach_vm_address_t orgGetDisplayStatus {};

	/**
	 *  Original AppleIntelFramebufferController::ReadRegister32 function
	 */
	uint32_t (*orgCflReadRegister32)(void *, uint32_t) {nullptr};
	uint32_t (*orgKblReadRegister32)(void *, uint32_t) {nullptr};

	/**
	 *  Original AppleIntelFramebufferController::WriteRegister32 function
	 */
	void (*orgCflWriteRegister32)(void *, uint32_t, uint32_t) {nullptr};
	void (*orgKblWriteRegister32)(void *, uint32_t, uint32_t) {nullptr};

	/**
	 *  Set to true if a black screen ComputeLaneCount patch is required
	 */
	bool blackScreenPatch {false};

	/**
	 *  Coffee Lake backlight patch configuration options
	 */
	enum class CoffeeBacklightPatch {
		Auto = -1,
		On = 1,
		Off = 0
	};

	/**
	 *  Set to On if Coffee Lake backlight patch type required
	 *  - boot-arg igfxcflbklt=0/1 forcibly turns patch on or off (override)
	 *  - IGPU property enable-cfl-backlight-fix turns patch on
	 *  - laptop with CFL CPU and CFL IGPU drivers turns patch on
	 */
	CoffeeBacklightPatch cflBacklightPatch {CoffeeBacklightPatch::Off};

	/**
	 *  Set to true if PAVP code should be disabled
	 */
	bool pavpDisablePatch {false};

	/**
	 *  Set to true if read descriptor patch should be enabled
	 */
	bool readDescriptorPatch {false};

	/**
	 *  Set to true to disable Metal support
	 */
	bool forceOpenGL {false};

	/**
	 *  Set to true to enable Metal support for offline rendering
	 */
	bool forceMetal {false};

	/**
	 *  Set to true if Sandy Bridge Gen6Accelerator should be renamed
	 */
	bool moderniseAccelerator {false};

	/**
	 *  Disable AGDC configuration
	 */
	bool disableAGDC {false};

	/**
	 *  GuC firmware loading scheme
	 */
	enum FirmwareLoad {
		FW_AUTO    = -1 /* Use as is for Apple, disable for others */,
		FW_DISABLE = 0, /* Use host scheduler without GuC */
		FW_GENERIC = 1, /* Use reference scheduler with GuC */
		FW_APPLE   = 2, /* Use Apple GuC scheduler */
	};

	/**
	 *  Set to true to avoid incompatible GPU firmware loading
	 */
	FirmwareLoad fwLoadMode {FW_AUTO};

	/**
	 *  Requires framebuffer modifications
	 */
	bool applyFramebufferPatch {false};

	/**
	 *  Perform framebuffer dump to /AppleIntelFramebufferNUM
	 */
	bool dumpFramebufferToDisk {false};

	/**
	 *  Trace framebuffer logic
	 */
	bool debugFramebuffer {false};

	/**
	 * Per-framebuffer helper script.
	 */
	struct FramebufferModifer {
		bool supported {false}; // compatible CPU
		bool legacy {false}; // legacy CPU (Skylake)
		bool enable {false}; // enable the patch
		bool customised {false}; // override default patch behaviour
		uint8_t fbs[sizeof(uint64_t)] {}; // framebuffers to force modeset for on override

		bool inList(IORegistryEntry* fb) {
			uint32_t idx;
			if (AppleIntelFramebufferExplorer::getIndex(fb, idx))
				for (auto i : fbs)
					if (i == idx)
						return true;
			return false;
		}
	};
	
	// NOTE: the MMIO space is also available at RC6_RegBase
	uint32_t (*AppleIntelFramebufferController__ReadRegister32)(void*,uint32_t) {};
	void (*AppleIntelFramebufferController__WriteRegister32)(void*,uint32_t,uint32_t) {};

	class AppleIntelFramebufferController;
	// Populated at AppleIntelFramebufferController::start
	// Useful for getting access to Read/WriteRegister, rather than having
	// to compute the offsets
	AppleIntelFramebufferController** gFramebufferController {};
	
	// Available on ICL+
	// Apple has refactored quite a large amount of code into a new class `AppleIntelPort` in the ICL graphics driver,
	// and the framebuffer controller now maintains an array of `ports`.
	class AppleIntelPort;

	struct RPSControl {
		bool available {false};
		bool enabled {false};
		uint32_t freq_max {0};
		
		void initFB(IGFX&,KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
		void initGraphics(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

		static int pmNotifyWrapper(unsigned int,unsigned int,unsigned long long *,unsigned int *);
		mach_vm_address_t orgPmNotifyWrapper;
	} RPSControl;
	
	struct ForceWakeWorkaround {
		bool enabled {false};

		void initGraphics(IGFX&,KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
		
		static bool pollRegister(uint32_t, uint32_t, uint32_t, uint32_t);
		static bool forceWakeWaitAckFallback(uint32_t, uint32_t, uint32_t);
		static void forceWake(void*, uint8_t set, uint32_t dom, uint32_t);
	} ForceWakeWorkaround;
	
	/**
	 *  Interface of a submodule to fix Intel graphics drivers
	 */
	class PatchSubmodule {
	public:
		/**
		 *  Virtual destructor
		 */
		virtual ~PatchSubmodule() = default;
		
		/**
		 *  True if this submodule should be enabled
		 */
		bool enabled {false};
		
		/**
		 *  True if this submodule requires patching the framebuffer driver
		 */
		bool requiresPatchingFramebuffer {false};
		
		/**
		 *  True if this submodule requires patching the graphics acceleration driver
		 */
		bool requiresPatchingGraphics {false};
		
		/**
		 *  Initialize any data structure required by this submodule if necessary
		 */
		virtual void init() {}
		
		/**
		 *  Release any resources obtained by this submodule if necessary
		 */
		virtual void deinit() {}
		
		/**
		 *  Setup the fix and retrieve the device information if necessary
		 *
		 *  @param patcher  KernelPatcher instance
		 *  @param info     Information about the graphics device
		 *  @note This function is called when the main IGFX module processes the kernel.
		 */
		virtual void processKernel(KernelPatcher &patcher, DeviceInfo *info) {}

		/**
		 *  Process the framebuffer kext, retrieve and/or route functions if necessary
		 *
		 *  @param patcher KernelPatcher instance
		 *  @param index   kinfo handle
		 *  @param address kinfo load address
		 *  @param size    kinfo memory size
		 *  @note This function is called when the main IGFX module processes the kext.
		 */
		virtual void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {}
		
		/**
		 *  Process the graphics accelerator kext, retrieve and/or route functions if necessary
		 *
		 *  @param patcher KernelPatcher instance
		 *  @param index   kinfo handle
		 *  @param address kinfo load address
		 *  @param size    kinfo memory size
		 *  @note This function is called when the main IGFX module processes the kext.
		 */
		virtual void processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {}
	};
	
	/**
	 *  A submodule to fix the calculation of DVMT preallocated memory on ICL+ platforms
	 */
	class DVMTCalcFix: public PatchSubmodule {
		/**
		 *  The amount of DVMT preallocated memory in bytes set in the BIOS
		 */
		uint32_t dvmt {0};
		
	public:
		/**
		 *  True if this fix is available for the current Intel platform
		 */
		bool available {false};
		
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modDVMTCalcFix;
	
	/**
	 *  A submodule to fix the maximum link rate reported by DPCD
	 */
	class DPCDMaxLinkRateFix: public PatchSubmodule {
		/**
		 *  User-specified maximum link rate value in the DPCD buffer
		 *
		 *  Auto: Default value is 0x00 to detect the maximum link rate automatically;
		 *  User: Specify a custom link rate via the `dpcd-max-link-rate` property.
		 */
		uint32_t maxLinkRate {0x00};
		
		/**
		 *  [CFL-] The framebuffer controller instance passed to `ReadAUX()`
		 *
		 *  @note This field is set to invoke platform-independent ReadAUX().
		 */
		AppleIntelFramebufferController *controller {nullptr};
		
		/**
		 *  [CFL-] The framebuffer instance passed to `ReadAUX()`
		 *
		 *  @note This field is set to invoke platform-independent ReadAUX().
		 */
		IORegistryEntry *framebuffer {nullptr};
		
		/**
		 *  [CFL-] The display path instance passed to `ReadAUX()`
		 *
		 *  @note This field is set to invoke platform-independent ReadAUX().
		 */
		void *displayPath {nullptr};
		
		/**
		 *  [ICL+] The port instance passed to `ReadAUX()`
		 *
		 *  @note This field is set to invoke platform-independent ReadAUX().
		 */
		AppleIntelPort *port {nullptr};
		
		/**
		 *  [CFL-] Original AppleIntelFramebufferController::ReadAUX function
		 *
		 *  @seealso Refer to the document of `wrapReadAUX()` below.
		 */
		IOReturn (*orgCFLReadAUX)(AppleIntelFramebufferController *, IORegistryEntry *, uint32_t, uint16_t, void *, void *) {nullptr};
		
		/**
		 *  [ICL+] Original AppleIntelPort::readAUX function
		 *
		 *  @seealso Refer to the document of `wrapICLReadAUX()` below.
		 */
		IOReturn (*orgICLReadAUX)(AppleIntelPort *, uint32_t, void *, uint32_t) {nullptr};
		
		/**
		 *  [ICL+] Original AppleIntelPort::getFBFromPort function
		 *
		 *  @param that The hidden implicity framebuffer controller instance
		 *  @param port Specify the port instance to retrieve the corresponding framebuffer instance
		 *  @return The framebuffer instance associated with the given port on success, `NULL` otherwise.
		 *  @note This function is required to retrieve the framebuffer index via a port.
		 */
		IORegistryEntry *(*orgICLGetFBFromPort)(AppleIntelFramebufferController *, AppleIntelPort *) {nullptr};
		
		/**
		 *  [CFL-] ReadAUX wrapper to modify the maximum link rate value in the DPCD buffer
		 *
		 *  @param that The hidden implicit framebuffer controller instance
		 *  @param framebuffer The framebuffer instance
		 *  @param address DPCD register address
		 *  @param length Specify the number of bytes read from the register at `address`
		 *  @param buffer A non-null buffer to store bytes read from DPCD
		 *  @param displayPath The display path instance
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note The actual work is delegated to the platform-independent function `wrapReadAUX()`.
		 */
		static IOReturn wrapCFLReadAUX(AppleIntelFramebufferController *that, IORegistryEntry *framebuffer, uint32_t address, uint16_t length, void *buffer, void *displayPath);
		
		/**
		 *  [ICL+] ReadAUX wrapper to modify the maximum link rate value in the DPCD buffer
		 *
		 *  @param that The hidden implicit port instance
		 *  @param address DPCD register address
		 *  @param buffer A non-null buffer to store bytes read from DPCD
		 *  @param length Specify the number of bytes read from the register at `address`
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note The actual work is delegated to the platform-independent function `wrapReadAUX()`.
		 */
		static IOReturn wrapICLReadAUX(AppleIntelPort *that, uint32_t address, void *buffer, uint32_t length);
		
		/**
		 *  [Common] ReadAUX wrapper to modify the maximum link rate value in the DPCD buffer
		 *
		 *  @param address DPCD register address
		 *  @param buffer A non-null buffer to store bytes read from DPCD
		 *  @param length Specify the number of bytes read from the register at `address`
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note This function is independent of the platform.
		 */
		static IOReturn wrapReadAUX(uint32_t address, void *buffer, uint32_t length);
		
		/**
		 *  [Common] Read from DPCD via DisplayPort AUX channel
		 *
		 *  @param address DPCD register address
		 *  @param buffer A non-null buffer to store bytes read from DPCD
		 *  @param length Specify the number of bytes read from the register at `address`
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note This function is independent of the platform.
		 */
		IOReturn orgReadAUX(uint32_t address, void* buffer, uint32_t length);
		
		/**
		 *  [Common] Retrieve the framebuffer index
		 *
		 *  @param index The framebuffer index on return
		 *  @return `true` on success, `false` otherwise.
		 *  @note This function is independent of the platform.
		 */
		bool getFramebufferIndex(uint32_t &index);
		
		/**
		 *  [Helper] Verify the given link rate value
		 *
		 *  @param rate The decimal link rate value
		 *  @return The given rate value if it is supported by the driver, `0` otherwise.
		 */
		static inline uint32_t verifyLinkRateValue(uint32_t rate) {
			switch (rate) {
				case 0x1E: // HBR3 8.1  Gbps
				case 0x14: // HBR2 5.4  Gbps
				case 0x0C: // 3_24 3.24 Gbps Used by Apple internally
				case 0x0A: // HBR  2.7  Gbps
				case 0x06: // RBR  1.62 Gbps
					return rate;

				default:
					return 0;
			}
		}
		
		/**
		 *  [Helper] Probe the maximum link rate from DPCD
		 *
		 *  @return The maximum link rate value on success, `0` otherwise.
		 *  @note This function is independent of the platform.
		 *  @note This function also returns `0` if the maximum link rate found in the table is not supported by Apple's driver.
		 *  @note The driver only supports `0x06` (RBR), `0x0A` (HBR), `0x0C` (Apple's Internal), `0x14` (HBR2), `0x1E` (HBR3).
		 */
		uint32_t probeMaxLinkRate();
		
		/**
		 *  [CFL-] Process the framebuffer kext for CFL- platforms
		 */
		void processFramebufferKextForCFL(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
		
		/**
		 *  [ICL+] Process the framebuffer kext for ICL+ platforms
		 */
		void processFramebufferKextForICL(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
		
		// MARK: Patch Submodule IMP
	public:
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modDPCDMaxLinkRateFix;
	
	/**
	 *  A submodule to support all valid Core Display Clock frequencies on ICL+ platforms
	 */
	class CoreDisplayClockFix: public PatchSubmodule {
		/**
		 *  [ICL+] Original AppleIntelFramebufferController::ReadRegister32 function
		 *
		 *  @param that The implicit hidden framebuffer controller instance
		 *  @param address Address of the MMIO register
		 *  @return The 32-bit integer read from the register.
		 */
		uint32_t (*orgIclReadRegister32)(AppleIntelFramebufferController *, uint32_t) {nullptr};
		
		/**
		 *  [ICL+] Original AppleIntelFramebufferController::probeCDClockFrequency function
		 *
		 *  @seealso Refer to the document of `wrapProbeCDClockFrequency()` below.
		 */
		uint32_t (*orgProbeCDClockFrequency)(AppleIntelFramebufferController *) {nullptr};
		
		/**
		 *  [ICL+] Original AppleIntelFramebufferController::disableCDClock function
		 *
		 *  @param that The implicit hidden framebuffer controller instance
		 *  @note This function is required to reprogram the Core Display Clock.
		 */
		void (*orgDisableCDClock)(AppleIntelFramebufferController *) {nullptr};
		
		/**
		 *  [ICL+] Original AppleIntelFramebufferController::setCDClockFrequency function
		 *
		 *  @param that The implicit hidden framebuffer controller instance
		 *  @param frequency The Core Display Clock PLL frequency in Hz
		 *  @note This function changes the frequency of the Core Display Clock and reenables it.
		 */
		void (*orgSetCDClockFrequency)(AppleIntelFramebufferController *, unsigned long long) {nullptr};
		
		/**
		 *  [Helper] A helper to change the Core Display Clock frequency to a supported value
		 */
		static void sanitizeCDClockFrequency(AppleIntelFramebufferController *that);
		
		/**
		 *  [Wrapper] Probe and adjust the Core Display Clock frequency if necessary
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @return The PLL VCO frequency in Hz derived from the current Core Display Clock frequency.
		 *  @note This is a wrapper for Apple's original `AppleIntelFramebufferController::probeCDClockFrequency()` method.
		 *        Used to inject code to reprogram the clock so that its frequency is natively supported by the driver.
		 */
		static uint32_t wrapProbeCDClockFrequency(AppleIntelFramebufferController *that);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modCoreDisplayClockFix;
	
	/**
	 *  A submodule to fix the calculation of HDMI dividers to avoid the infinite loop
	 */
	class HDMIDividersCalcFix: public PatchSubmodule {
		/**
		 *  Represents the current context of probing dividers for HDMI connections
		 */
		struct ProbeContext {
			/// The current minimum deviation
			uint64_t minDeviation;

			/// The current chosen central frequency
			uint64_t central;

			/// The current DCO frequency
			uint64_t frequency;

			/// The current selected divider
			uint32_t divider;

			/// The corresponding pdiv value [P0]
			uint32_t pdiv;

			/// The corresponding qdiv value [P1]
			uint32_t qdiv;

			/// The corresponding kqiv value [P2]
			uint32_t kdiv;
		};
		
		/**
		 *  [Helper] Compute the final P0, P1, P2 values based on the current frequency divider
		 *
		 *  @param context The current context for probing P0, P1 and P2.
		 *  @note Implementation adopted from the Intel Graphics Programmer Reference Manual;
		 *        Volume 12 Display, Page 135, Algorithm to Find HDMI and DVI DPLL Programming.
		 *        Volume 12 Display, Page 135, Pseudo-code for HDMI and DVI DPLL Programming.
		 *  @ref static void skl_wrpll_get_multipliers(p:p0:p1:p2:)
		 *  @seealso Intel Linux Graphics Driver
		 *  https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/i915/intel_dpll_mgr.c?h=v5.1.13#n1112
		 */
		static void populateP0P1P2(struct ProbeContext *context);

		/**
		 *  Compute dividers for a HDMI connection with the given pixel clock
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @param pixelClock The pixel clock value (in Hz) used for the HDMI connection
		 *  @param displayPath The corresponding display path
		 *  @param parameters CRTC parameters populated on return
		 *  @return Never used by its caller, so this method might return void.
		 *  @note Method Signature: `AppleIntelFramebufferController::ComputeHdmiP0P1P2(pixelClock:displayPath:parameters:)`
		 */
		static void wrapComputeHdmiP0P1P2(AppleIntelFramebufferController *that, uint32_t pixelClock, void *displayPath, void *parameters);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modHDMIDividersCalcFix;

	/**
	 *  A submodule to support advanced I2C-over-AUX transactions on SKL, KBL and CFL platforms
	 *
	 *  @note LSPCON driver enables this module automatically. This module must be placed after the LSPCON module.
	 *  @note ICL platform does not require this module as it provides native HDMI 2.0 output.
	 */
	class AdvancedI2COverAUXSupport: public PatchSubmodule {
		/**
		 *  Set to true to enable verbose output in I2C-over-AUX transactions
		 */
		bool verbose {false};
		
		/**
		 *  Original AppleIntelFramebufferController::ReadI2COverAUX function
		 *
		 *  @seealso Refer to the document of `wrapReadI2COverAUX()` below.
		 */
		IOReturn (*orgReadI2COverAUX)(void *, IORegistryEntry *, void *, uint32_t, uint16_t, uint8_t *, bool, uint8_t) {nullptr};

		/**
		 *  Original AppleIntelFramebufferController::WriteI2COverAUX function
		 *
		 *  @seealso Refer to the document of `wrapWriteI2COverAUX()` below.
		 */
		IOReturn (*orgWriteI2COverAUX)(void *, IORegistryEntry *, void *, uint32_t, uint16_t, uint8_t *, bool) {nullptr};
		
	public:
		/// MARK: I2C-over-AUX Transaction APIs

		/**
		 *  [Basic] Read from an I2C slave via the AUX channel
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @param framebuffer A framebuffer instance
		 *  @param displayPath A display path instance
		 *  @param address The 7-bit address of an I2C slave
		 *  @param length The number of bytes requested to read and must be <= 16 (See below)
		 *  @param buffer A buffer to store the read bytes (See below)
		 *  @param intermediate Set `true` if this is an intermediate read (See below)
		 *  @param flags A flag reserved by Apple; currently always 0.
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note The number of bytes requested to read cannot be greater than 16, because the burst data size in
		 *        a single AUX transaction is 20 bytes, in which the first 4 bytes are used for the message header.
		 *  @note Passing a `0` buffer length and a `NULL` buffer will start or stop an I2C transaction.
		 *  @note When `intermediate` is `true`, the Middle-of-Transaction (MOT, bit 30 in the header) bit will be set to 1.
		 *        (See Section 2.7.5.1 I2C-over-AUX Request Transaction Command in VESA DisplayPort Specification 1.2)
		 *  @note Similar logic could be found at `intel_dp.c` (Intel Linux Graphics Driver; Linux Kernel)
		 *        static ssize_t intel_dp_aux_transfer(struct drm_dp_aux* aux, struct drm_dp_aux_msg* msg)
		 *  @note Method Signature: `AppleIntelFramebufferController::ReadI2COverAUX(framebuffer:displayPath:address:length:buffer:intermediate:flags:)`
		 *  @note This is a wrapper for Apple's original `AppleIntelFramebufferController::ReadI2COverAUX()` method.
		 *  @seealso See the actual implementation extracted from my reverse engineering research for detailed information.
		 */
		static IOReturn wrapReadI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint16_t length, uint8_t *buffer, bool intermediate, uint8_t flags);

		/**
		 *  [Basic] Write to an I2C adapter via the AUX channel
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @param framebuffer A framebuffer instance
		 *  @param displayPath A display path instance
		 *  @param address The 7-bit address of an I2C slave
		 *  @param length The number of bytes requested to write and must be <= 16 (See below)
		 *  @param buffer A buffer that stores the bytes to write (See below)
		 *  @param intermediate Set `true` if this is an intermediate write (See below)
		 *  @param flags A flag reserved by Apple; currently always 0.
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note The number of bytes requested to read cannot be greater than 16, because the burst data size in
		 *        a single AUX transaction is 20 bytes, in which the first 4 bytes are used for the message header.
		 *  @note Passing a `0` buffer length and a `NULL` buffer will start or stop an I2C transaction.
		 *  @note When `intermediate` is `true`, the Middle-of-Transaction (MOT, bit 30 in the header) bit will be set to 1.
		 *        (See Section 2.7.5.1 I2C-over-AUX Request Transaction Command in VESA DisplayPort Specification 1.2)
		 *  @note Similar logic could be found at `intel_dp.c` (Intel Linux Graphics Driver; Linux Kernel)
		 *        static ssize_t intel_dp_aux_transfer(struct drm_dp_aux* aux, struct drm_dp_aux_msg* msg)
		 *  @note Method Signature: `AppleIntelFramebufferController::WriteI2COverAUX(framebuffer:displayPath:address:length:buffer:intermediate:)`
		 *  @note This is a wrapper for Apple's original `AppleIntelFramebufferController::WriteI2COverAUX()` method.
		 *  @seealso See the actual implementation extracted from my reverse engineering research for detailed information.
		 */
		static IOReturn wrapWriteI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint16_t length, uint8_t *buffer, bool intermediate);
	
		/**
		 *  [Advanced] Reposition the offset for an I2C-over-AUX access
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @param framebuffer A framebuffer instance
		 *  @param displayPath A display path instance
		 *  @param address The 7-bit address of an I2C slave
		 *  @param offset The address of the next register to access
		 *  @param flags A flag reserved by Apple. Currently always 0.
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note Method Signature: `AppleIntelFramebufferController::advSeekI2COverAUX(framebuffer:displayPath:address:offset:flags:)`
		 *  @note Built upon Apple's original `ReadI2COverAUX()` and `WriteI2COverAUX()` APIs.
		 */
		static IOReturn advSeekI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint8_t flags);

		/**
		 *  [Advanced] Read from an I2C slave via the AUX channel
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @param framebuffer A framebuffer instance
		 *  @param displayPath A display path instance
		 *  @param address The 7-bit address of an I2C slave
		 *  @param offset Address of the first register to read from
		 *  @param length The number of bytes requested to read starting from `offset`
		 *  @param buffer A non-null buffer to store the bytes
		 *  @param flags A flag reserved by Apple. Currently always 0.
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note Method Signature: `AppleIntelFramebufferController::advReadI2COverAUX(framebuffer:displayPath:address:offset:length:buffer:flags:)`
		 *  @note Built upon Apple's original `ReadI2COverAUX()` and `WriteI2COverAUX()` APIs.
		 */
		static IOReturn advReadI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint16_t length, uint8_t *buffer, uint8_t flags);

		/**
		 *  [Advanced] Write to an I2C slave via the AUX channel
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @param framebuffer A framebuffer instance
		 *  @param displayPath A display path instance
		 *  @param address The 7-bit address of an I2C slave
		 *  @param offset Address of the first register to write to
		 *  @param length The number of bytes requested to write starting from `offset`
		 *  @param buffer A non-null buffer containing the bytes to write
		 *  @param flags A flag reserved by Apple. Currently always 0.
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note Method Signature: `AppleIntelFramebufferController::advWriteI2COverAUX(framebuffer:displayPath:address:offset:length:buffer:flags:)`
		 *  @note Built upon Apple's original `ReadI2COverAUX()` and `WriteI2COverAUX()` APIs.
		 */
		static IOReturn advWriteI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint16_t length, uint8_t *buffer, uint8_t flags);
		
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modAdvancedI2COverAUXSupport;
	
	/**
	 *  A submodule that adds driver support for the onboard LSPCON chip to enable HDMI 2.0 output on SKL, KBL and CFL platforms
	 */
	class LSPCONDriverSupport: public PatchSubmodule {
	public:
		/**
		 *  Original AppleIntelFramebufferController::ReadAUX function
		 */
		IOReturn (*orgReadAUX)(void *, IORegistryEntry *, uint32_t, uint16_t, void *, void *) {nullptr};
		
	private:
		/**
		 *  Original AppleIntelFramebufferController::GetDPCDInfo function
		 *
		 *  @seealso Refer to the document of `wrapGetDPCDInfo()` below.
		 */
		IOReturn (*orgGetDPCDInfo)(void *, IORegistryEntry *, void *) {nullptr};
		
		/**
		 *  User-defined LSPCON chip info for all possible framebuffers
		 */
		FramebufferLSPCON lspcons[MaxFramebufferConnectorCount];
		
		/// MARK: Manage user-defined LSPCON chip info for all framebuffers

		/**
		 *  Setup the LSPCON driver for the given framebuffer
		 *
		 *  @param that The opaque framebuffer controller instance
		 *  @param framebuffer The framebuffer that owns this LSPCON chip
		 *  @param displayPath The corresponding opaque display path instance
		 *  @note This method will update fields in `lspcons` accordingly on success.
		 */
		void setupLSPCON(void *that, IORegistryEntry *framebuffer, void *displayPath);

		/**
		 *  [Convenient] Check whether the given framebuffer has an onboard LSPCON chip
		 *
		 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
		 *  @return `true` if the framebuffer has an onboard LSPCON chip, `false` otherwise.
		 */
		bool hasLSPCON(uint32_t index) {
			return lspcons[index].hasLSPCON;
		}

		/**
		 *  [Convenient] Check whether the given framebuffer already has LSPCON driver initialized
		 *
		 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
		 *  @return `true` if the LSPCON driver has already been initialized for this framebuffer, `false` otherwise.
		 */
		bool hasLSPCONInitialized(uint32_t index) {
			return lspcons[index].lspcon != nullptr;
		}

		/**
		 *  [Convenient] Get the non-null LSPCON driver associated with the given framebuffer
		 *
		 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
		 *  @return The LSPCON driver instance.
		 */
		LSPCON *getLSPCON(uint32_t index) {
			return lspcons[index].lspcon;
		}

		/**
		 *  [Convenient] Set the non-null LSPCON driver for the given framebuffer
		 *
		 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
		 *  @param lspcon A non-null LSPCON driver instance associated with the given framebuffer
		 */
		void setLSPCON(uint32_t index, LSPCON *lspcon) {
			lspcons[index].lspcon = lspcon;
		}

		/**
		 *  [Convenient] Get the preferred LSPCON mode for the given framebuffer
		 *
		 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
		 *  @return The preferred adapter mode.
		 */
		LSPCON::Mode getLSPCONPreferredMode(uint32_t index) {
			return lspcons[index].preferredMode;
		}
		
		/**
		 *  [Wrapper] Retrieve the DPCD info for a given framebuffer port
		 *
		 *  @param that The hidden implicit `this` pointer
		 *  @param framebuffer A framebuffer instance
		 *  @param displayPath A display path instance
		 *  @return `kIOReturnSuccess` on success, other values otherwise.
		 *  @note This is a wrapper for Apple's original `AppleIntelFramebufferController::GetDPCDInfo()` method.
		 *        Used to inject code to initialize the driver for the onboard LSPCON chip.
		 */
		static IOReturn wrapGetDPCDInfo(void *that, IORegistryEntry *framebuffer, void *displayPath);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void deinit() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modLSPCONDriverSupport;
	
	/**
	 *  The LSPCON driver requires access to I2C-over-AUX transaction APIs
	 */
	friend class LSPCON;
	
	/**
	 *	A collection of submodules
	 */
	PatchSubmodule *submodules[6] = { &modDVMTCalcFix, &modDPCDMaxLinkRateFix, &modCoreDisplayClockFix, &modHDMIDividersCalcFix, &modLSPCONDriverSupport, &modAdvancedI2COverAUXSupport };
	
	/**
	 * Ensure each modeset is a complete modeset.
	 */
	FramebufferModifer forceCompleteModeset;

	/**
	 * Ensure each display is online.
	 */
	FramebufferModifer forceOnlineDisplay;
	
	/**
	 * Prevent IntelAccelerator from starting.
	 */
	bool disableAccel {false};

	/**
	 * Disable Type C framebuffer check.
	 */
	bool disableTypeCCheck {false};
	
	/**
	 *  Perform platform table dump to ioreg
	 */
	bool dumpPlatformTable {false};

	/**
	 *  Perform automatic DP -> HDMI replacement
	 */
	bool hdmiAutopatch {false};

	/**
	 *  Supports GuC firmware
	 */
	bool supportsGuCFirmware {false};

	/**
	 *  Currently loading GuC firmware
	 */
	bool performingFirmwareLoad {false};

	/**
	 *  Framebuffer address space start
	 */
	uint8_t *framebufferStart {nullptr};

	/**
	 *  Framebuffer address space size
	 */
	size_t framebufferSize {0};

	/**
	 *  Pointer to the original GuC firmware
	 */
	uint8_t *gKmGen9GuCBinary {nullptr};

	/**
	 *  Pointer to the original GuC firmware signature
	 */
	uint8_t *signaturePointer {nullptr};

	/**
	 *  Pointer to GuC firmware upload size assignment
	 */
	uint32_t *firmwareSizePointer {nullptr};

	/**
	 *  Dummy firmware buffer to store unused old firmware in
	 */
	uint8_t *dummyFirmwareBuffer {nullptr};

	/**
	 *  Actual firmware buffer we store our new firmware in
	 */
	uint8_t *realFirmwareBuffer {nullptr};

	/**
	 *  Actual intercepted binary sizes
	 */
	uint32_t realBinarySize {};

	/**
	 *  Store backlight level
	 */
	uint32_t backlightLevel {};

	/**
	 *  Fallback user-requested backlight frequency in case 0 was initially written to the register.
	 */
	static constexpr uint32_t FallbackTargetBacklightFrequency {120000};

	/**
	 *  User-requested backlight frequency obtained from BXT_BLC_PWM_FREQ1 at system start.
	 *  Can be specified via max-backlight-freq property.
	 */
	uint32_t targetBacklightFrequency {};

	/**
	 *  User-requested pwm control value obtained from BXT_BLC_PWM_CTL1.
	 */
	uint32_t targetPwmControl {};

	/**
	 *  Driver-requested backlight frequency obtained from BXT_BLC_PWM_FREQ1 write attempt at system start.
	 */
	uint32_t driverBacklightFrequency {};

	/**
	 * See function definition for explanation
	 */
	static bool wrapHwRegsNeedUpdate(void *controller, IOService *framebuffer, void *displayPath, void *crtParams, void *detailedInfo);

	/**
	 *  Explore the framebuffer structure in Apple's Intel graphics driver
	 */
	struct AppleIntelFramebufferExplorer {
		/**
		 *  [Convenient] Retrieve the framebuffer index
		 *
		 *  @param framebuffer An `AppleIntelFramebuffer` instance
		 *  @param index The framebuffer index on return
		 *  @return `true` on success, `false` if the framebuffer is NULL or the index does not exist.
		 */
		static bool getIndex(IORegistryEntry *framebuffer, uint32_t &index) {
			if (framebuffer == nullptr)
				return false;
			
			auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
			if (idxnum != nullptr) {
				index = idxnum->unsigned32BitValue();
				return true;
			}
			return false;
		}
	};
	
	/**
	 *  PAVP session callback wrapper used to prevent freezes on incompatible PAVP certificates
	 */
	static IOReturn wrapPavpSessionCallback(void *intelAccelerator, int32_t sessionCommand, uint32_t sessionAppId, uint32_t *a4, bool flag);

	/**
	 *  Global page table read wrapper for Kaby Lake.
	 */
	static bool globalPageTableRead(void *hardwareGlobalPageTable, uint64_t a1, uint64_t &a2, uint64_t &a3);

	/**
	 *  DP ComputeLaneCount wrapper to report success on non-DP screens to avoid black screen
	 */
	static bool wrapComputeLaneCount(void *that, void *timing, uint32_t bpp, int32_t availableLanes, int32_t *laneCount);

	/**
	 *  DP ComputeLaneCount wrapper to report success on non-DP screens to avoid black screen (10.14.1+ KBL/CFL version)
	 */
	static bool wrapComputeLaneCountNouveau(void *that, void *timing, int32_t availableLanes, int32_t *laneCount);

	/**
	 *  copyExistingServices wrapper used to rename Gen6Accelerator from userspace calls
	 */
	static OSObject *wrapCopyExistingServices(OSDictionary *matching, IOOptionBits inState, IOOptionBits options);

	/**
	 *  IntelAccelerator::start wrapper to support vesa mode, force OpenGL, prevent fw loading, etc.
	 */
	static bool wrapAcceleratorStart(IOService *that, IOService *provider);

	/**
	 *  Wrapped AppleIntelFramebufferController::WriteRegister32 function
	 */
	static void wrapCflWriteRegister32(void *that, uint32_t reg, uint32_t value);
	static void wrapKblWriteRegister32(void *that, uint32_t reg, uint32_t value);

	/**
	 *  AppleIntelFramebufferController::getOSInformation wrapper to patch framebuffer data
	 */
	static bool wrapGetOSInformation(IOService *that);

	/**
	 *  IGHardwareGuC::loadGuCBinary wrapper to feed updated (compatible GuC)
	 */
	static bool wrapLoadGuCBinary(void *that, bool flag);

	/**
	 *  Actual firmware loader
	 *
	 *  @param that  IGScheduler4 instance
	 *
	 *  @return true on success
	 */
	static bool wrapLoadFirmware(IOService *that);

	/**
	 *  Handle sleep event
	 *
	 *  @param that  IGScheduler4 instance
	 */
	static void wrapSystemWillSleep(IOService *that);

	/**
	 *  Handle wake event
	 *
	 *  @param that  IGScheduler4 instance
	 */
	static void wrapSystemDidWake(IOService *that);

	/**
	 *  IGHardwareGuC::initSchedControl wrapper to avoid incompatibilities during GuC load
	 */
	static bool wrapInitSchedControl(void *that, void *ctrl);

	/**
	 *  IGSharedMappedBuffer::withOptions wrapper to prepare for GuC firmware loading
	 */
	static void *wrapIgBufferWithOptions(void *accelTask, unsigned long size, unsigned int type, unsigned int flags);

	/**
	 *  IGMappedBuffer::getGPUVirtualAddress wrapper to trick GuC firmware virtual addresses
	 */
	static uint64_t wrapIgBufferGetGpuVirtualAddress(void *that);

	/**
	 *  IntelFBClientControl::doAttribute wrapper to filter attributes like AGDC.
	 */
	static IOReturn wrapFBClientDoAttribute(void *fbclient, uint32_t attribute, unsigned long *unk1, unsigned long unk2, unsigned long *unk3, unsigned long *unk4, void *externalMethodArguments);

	/**
	 *  AppleIntelFramebuffer::getDisplayStatus to force display status on configured screens.
	 */
	static uint32_t wrapGetDisplayStatus(IOService *framebuffer, void *displayPath);
	
	static uint64_t wrapIsTypeCOnlySystem(void*);

	/**
	 *  Load GuC-specific patches and hooks
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle for graphics driver
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void loadIGScheduler4Patches(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  Enable framebuffer debugging
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void loadFramebufferDebug(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

	/**
	 *  Load user-specified arguments from IGPU device
	 *
	 *  @param igpu                IGPU device handle
	 *  @param currentFramebuffer  current framebuffer id number
	 *
	 *  @return true if there is anything to do
	 */
	bool loadPatchesFromDevice(IORegistryEntry *igpu, uint32_t currentFramebuffer);

	/**
	 *  Find the framebuffer id in data
	 *
	 *  @param framebufferId    Framebuffer id to search
	 *  @param startingAddress  Start address of data to search
	 *  @param maxSize          Maximum size of data to search
	 *
	 *  @return pointer to address in data or nullptr
	 */
	uint8_t *findFramebufferId(uint32_t framebufferId, uint8_t *startingAddress, size_t maxSize);

#ifdef DEBUG
	/**
	 * Calculate total size of platform table list, including termination entry (FFFFFFFF 00000000)
	 *
	 * @param maxSize			Maximum size of data to search
	 *
	 * @return size of data
	 */
	size_t calculatePlatformListSize(size_t maxSize);

	/**
	 * Write platform table data to ioreg
	 *
	 * @param subKeyName		ioreg subkey (under IOService://IOResources/WhateverGreen)
	 */
	void writePlatformListData(const char *subKeyName);
#endif

	/**
	 *  Patch data without changing kernel protection
	 *
	 *  @param patch            KernelPatcher instance
	 *  @param startingAddress  Start address of data to search
	 *  @param maxSize          Maximum size of data to search
	 *
	 *  @return true if patched anything
	 */
	bool applyPatch(const KernelPatcher::LookupPatch &patch, uint8_t *startingAddress, size_t maxSize);
	
	/**
	 *  Add int to dictionary.
	 *
	 *  @param dict		OSDictionary instance.
	 *  @param key		Key to add.
	 *  @param value 	Value to add.
	 *
	 *  @return true if added
	 */
	bool setDictUInt32(OSDictionary *dict, const char *key, UInt32 value);

	/**
	 *  Patch platformInformationList
	 *
	 *  @param framebufferId               Framebuffer id
	 *  @param platformInformationList     PlatformInformationList pointer
	 *
	 *  @return true if patched anything
	 */
	template <typename T>
	bool applyPlatformInformationListPatch(uint32_t framebufferId, T *platformInformationList);

	/**
	 *  Extended patching called from applyPlatformInformationListPatch
	 *
	 *  @param frame               pointer to Framebuffer data
	 *
	 */
	template <typename T>
	void applyPlatformInformationPatchEx(T* frame);

	/**
	 *  Apply framebuffer patches
	 */
	void applyFramebufferPatches();

	/**
	 *  Patch platformInformationList with DP to HDMI connector type replacements
	 *
	 *  @param framebufferId               Framebuffer id
	 *  @param platformInformationList     PlatformInformationList pointer
	 *
	 *  @return true if patched anything
	 */
	template <typename T>
	bool applyDPtoHDMIPatch(uint32_t framebufferId, T *platformInformationList);

	/**
	 *  Apply DP to HDMI automatic connector type changes
	 */
	void applyHdmiAutopatch();
	
	/**
	 *	Apply patches for first generation framebuffer.
	 */
	void applyWestmerePatches(KernelPatcher &patcher);
	
	/**
	 *	Apply I/O registry FeatureControl and FBCControl patches for first generation framebuffer.
	 */
	void applyWestmereFeaturePatches(IOService *framebuffer);
};

#endif /* kern_igfx_hpp */
