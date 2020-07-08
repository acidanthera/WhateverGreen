//
//  kern_igfx.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_igfx_hpp
#define kern_igfx_hpp

#include "kern_fb.hpp"

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Library/LegacyIOService.h>
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
	 *  Original AppleIntelFramebufferController::ReadAUX function
	 */
	IOReturn (*orgReadAUX)(void *, void *, uint32_t, uint16_t, void *, void *) {nullptr};

	/**
	 *  Original AppleIntelFramebufferController::ReadI2COverAUX function
	 */
	IOReturn (*orgReadI2COverAUX)(void *, IORegistryEntry *, void *, uint32_t, uint16_t, uint8_t *, bool, uint8_t) {nullptr};

	/**
	 *  Original AppleIntelFramebufferController::WriteI2COverAUX function
	 */
	IOReturn (*orgWriteI2COverAUX)(void *, IORegistryEntry *, void *, uint32_t, uint16_t, uint8_t *, bool) {nullptr};

	/**
	 *  Original AppleIntelFramebufferController::GetDPCDInfo function
	 */
	IOReturn (*orgGetDPCDInfo)(void *, IORegistryEntry *, void *);

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
	 *  Patch the maximum link rate in the DPCD buffer read from the built-in display
	 */
	bool maxLinkRatePatch {false};

	/**
	 *  Set to true to enable LSPCON driver support
	 */
	bool supportLSPCON {false};

	/**
	 *  Set to true to enable verbose output in I2C-over-AUX transactions
	 */
	bool verboseI2C {false};

	/**
	 *  Set to true to fix the infinite loop issue when computing dividers for HDMI connections
	 */
	bool hdmiP0P1P2Patch {false};

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

	struct RPSControl {
		bool enabled {false};
		uint32_t freq_max {0};
		
		void initFB(IGFX&,KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
		void initGraphics(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

		static int pmNotifyWrapper(unsigned int,unsigned int,unsigned long long *,unsigned int *);
		mach_vm_address_t orgPmNotifyWrapper;
	} RPSControl;
	
	struct ForceWakeWorkaround {
		bool enabled {false};
		IOLock* lck {};
		bool didInit {false};

		void initGraphics(IGFX&,KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
		
		static bool pollRegister(uint32_t, uint32_t, uint32_t, uint32_t);
		static bool forceWakeWaitAckFallback(uint32_t, uint32_t, uint32_t);
		static void forceWake(void*, uint8_t set, uint32_t dom, uint32_t);
	} ForceWakeWorkaround;

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
	 *  The default DPCD address
	 */
	static constexpr uint32_t DPCD_DEFAULT_ADDRESS = 0x0000;

	/**
	 *  The extended DPCD address
	 */
	static constexpr uint32_t DPCD_EXTENDED_ADDRESS = 0x2200;

	/**
	 *  Represents the first 16 fields of the receiver capabilities defined in DPCD
	 *
	 *  Main Reference:
	 *  - DisplayPort Specification Version 1.2
	 *
	 *  Side Reference:
	 *  - struct intel_dp @ line 1073 in intel_drv.h (Linux 4.19 Kernel)
	 *  - DP_RECEIVER_CAP_SIZE @ line 964 in drm_dp_helper.h
	 */
	struct DPCDCap16 { // 16 bytes
		// DPCD Revision (DP Config Version)
		// Value: 0x10, 0x11, 0x12, 0x13, 0x14
		uint8_t revision;

		// Maximum Link Rate
		// Value: 0x1E (HBR3) 8.1 Gbps
		//        0x14 (HBR2) 5.4 Gbps
		//        0x0C (3_24) 3.24 Gbps
		//        0x0A (HBR)  2.7 Gbps
		//        0x06 (RBR)  1.62 Gbps
		// Reference: 0x0C is used by Apple internally.
		uint8_t maxLinkRate;

		// Maximum Number of Lanes
		// Value: 0x1 (HBR2)
		//        0x2 (HBR)
		//        0x4 (RBR)
		// Side Notes:
		// (1) Bit 7 is used to indicate whether the link is capable of enhanced framing.
		// (2) Bit 6 is used to indicate whether TPS3 is supported.
		uint8_t maxLaneCount;

		// Maximum Downspread
		uint8_t maxDownspread;

		// Other fields omitted in this struct
		// Detailed information can be found in the specification
		uint8_t others[12];
	};

	/**
	 *  User-specified maximum link rate value in the DPCD buffer
	 *
	 *  Default value is 0x14 (5.4 Gbps, HBR2) for 4K laptop display
	 */
	uint32_t maxLinkRate {0x14};

	/**
	 *  ReadAUX wrapper to modify the maximum link rate value in the DPCD buffer
	 */
	static IOReturn wrapReadAUX(void *that, IORegistryEntry *framebuffer, uint32_t address, uint16_t length, void *buffer, void *displayPath);

	/**
	 * See function definition for explanation
	 */
	static bool wrapHwRegsNeedUpdate(void *controller, IOService *framebuffer, void *displayPath, void *crtParams, void *detailedInfo);

	/**
	 *  Reflect the `AppleIntelFramebufferController::CRTCParams` struct
	 *
	 *  @note Unlike the Intel Linux Graphics Driver,
	 *  - Apple does not transform the `pdiv`, `qdiv` and `kdiv` fields.
	 *  - Apple records the final central frequency divided by 15625.
	 *  @ref static void skl_wrpll_params_populate(params:afe_clock:central_freq:p0:p1:p2:)
	 *  @seealso https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/i915/intel_dpll_mgr.c?h=v5.1.13#n1171
	 */
	struct CRTCParams {
		/// Uninvestigated fields
		uint8_t uninvestigated[32];

		/// P0                          [`CRTCParams` field offset 0x20]
		uint32_t pdiv;

		/// P1                          [`CRTCParams` field offset 0x24]
		uint32_t qdiv;

		/// P2                          [`CRTCParams` field offset 0x28]
		uint32_t kdiv;

		/// Difference in Hz            [`CRTCParams` field offset 0x2C]
		uint32_t fraction;

		/// Multiplier of 24 MHz        [`CRTCParams` field offset 0x30]
		uint32_t multiplier;

		/// Central Frequency / 15625   [`CRTCParams` field offset 0x34]
		uint32_t cf15625;

		/// The rest fields are not of interest
	};

	static_assert(offsetof(CRTCParams, pdiv) == 0x20, "Invalid pdiv offset, please check your compiler.");
	static_assert(sizeof(CRTCParams) == 56, "Invalid size of CRTCParams struct, please check your compiler.");

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
	 *  The maximum positive deviation from the DCO central frequency
	 *
	 *  @note DCO frequency must be within +1% of the DCO central frequency.
	 *  @warning This is a hardware requirement.
	 *           See "Intel Graphics Programmer Reference Manual for Kaby Lake platform"
	 *           Volume 12 Display, Page 134, Formula for HDMI and DVI DPLL Programming
	 *  @link https://01.org/sites/default/files/documentation/intel-gfx-prm-osrc-kbl-vol12-display.pdf
	 *  @note This value is appropriate for graphics on Skylake, Kaby Lake and Coffee Lake platforms.
	 *  @seealso Intel Linux Graphics Driver
	 *  https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/gpu/drm/i915/intel_dpll_mgr.c?h=v5.1.13#n1080
	 */
	static constexpr uint64_t SKL_DCO_MAX_POS_DEVIATION = 100;

	/**
	 *  The maximum negative deviation from the DCO central frequency
	 *
	 *  @note DCO frequency must be within -6% of the DCO central frequency.
	 *  @seealso See `SKL_DCO_MAX_POS_DEVIATION` above for details.
	 */
	static constexpr uint64_t SKL_DCO_MAX_NEG_DEVIATION = 600;

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
	static void populateP0P1P2(struct ProbeContext* context);

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
	static int wrapComputeHdmiP0P1P2(void *that, uint32_t pixelClock, void *displayPath, void *parameters);

	/**
	 *  Explore the framebuffer structure in Apple's Intel graphics driver
	 */
	struct AppleIntelFramebufferExplorer {
		/**
		 *  [Convenient] Retrieve the framebuffer index
		 *
		 *  @param framebuffer An `AppleIntelFramebuffer` instance
		 *  @param index The framebuffer index on return
		 *  @return `true` on success, `false` if the index does not exist.
		 */
		static bool getIndex(IORegistryEntry *framebuffer, uint32_t &index) {
			auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
			if (idxnum != nullptr) {
				index = idxnum->unsigned32BitValue();
				return true;
			}
			return false;
		}
	};

	/**
	 *  Represents the register layouts of DisplayPort++ adapter at I2C address 0x40
	 *
	 *  Specific to LSPCON DisplayPort 1.2 to HDMI 2.0 Adapter
	 */
	struct DisplayPortDualModeAdapterInfo { // first 128 bytes
		/// [0x00] HDMI ID
		///
		/// Fixed Value: "DP-HDMI ADAPTOR\x04"
		uint8_t hdmiID[16];

		/// [0x10] Adapter ID
		///
		/// Bit Masks:
		/// - isType2 = 0xA0
		/// - hasDPCD = 0x08
		///
		/// Sample Values: 0xA8 = Type 2 Adapter with DPCD
		uint8_t adapterID;

		/// [0x11] IEEE OUI
		///
		/// Sample Value: 0x001CF8 [Parade]
		/// Reference: http://standards-oui.ieee.org/oui.txt
		uint8_t oui[3];

		/// [0x14] Device ID
		///
		/// Sample Value: 0x505331373530 = "PS1750"
		uint8_t deviceID[6];

		/// [0x1A] Hardware Revision Number
		///
		/// Sample Value: 0xB2 (B2 version)
		uint8_t revision;

		/// [0x1B] Firmware Major Revision
		uint8_t firmwareMajor;

		/// [0x1C] Firmware Minor Revision
		uint8_t firmwareMinor;

		/// [0x1D] Maximum TMDS Clock
		uint8_t maxTMDSClock;

		/// [0x1E] I2C Speed Capability
		uint8_t i2cSpeedCap;

		/// [0x1F] Unused/Reserved Field???
		uint8_t reserved0;

		/// [0x20] TMDS Output Buffer State
		///
		/// Bit Masks:
		/// - Disabled = 0x01
		///
		/// Sample Value:
		/// 0x00 = Enabled
		uint8_t tmdsOutputBufferState;

		/// [0x21] HDMI PIN CONTROL
		uint8_t hdmiPinCtrl;

		/// [0x22] I2C Speed Control
		uint8_t i2cSpeedCtrl;

		/// [0x23 - 0x3F] Unused/Reserved Fields
		uint8_t reserved1[29];

		/// [0x40] [W] Set the new LSPCON mode
		uint8_t lspconChangeMode;

		/// [0x41] [R] Get the current LSPCON mode
		///
		/// Bit Masks:
		/// - PCON = 0x01
		///
		/// Sample Value:
		/// 0x00 = LS
		/// 0x01 = PCON
		uint8_t lspconCurrentMode;

		/// [0x42 - 0x7F] Rest Unused/Reserved Fields
		uint8_t reserved2[62];
	};

	/**
	 *  Represents the onboard Level Shifter and Protocol Converter
	 */
	class LSPCON {
	public:
		/**
		 *  Represents all possible adapter modes
		 */
		enum class Mode: uint32_t {

			/// Level Shifter Mode 		(DP++ to HDMI 1.4)
			LevelShifter = 0x00,

			/// Protocol Converter Mode (DP++ to HDMI 2.0)
			ProtocolConverter = 0x01,

			/// Invalid Mode
			Invalid
		};

		/**
		 *  [Mode Helper] Parse the adapter mode from the raw register value
		 *
		 *  @param mode A raw register value read from the adapter.
		 *  @return The corresponding adapter mode on success, `invalid` otherwise.
		 */
		static inline Mode parseMode(uint8_t mode) {
			switch (mode & DP_DUAL_MODE_LSPCON_MODE_PCON) {
				case DP_DUAL_MODE_LSPCON_MODE_LS:
					return Mode::LevelShifter;

				case DP_DUAL_MODE_LSPCON_MODE_PCON:
					return Mode::ProtocolConverter;

				default:
					return Mode::Invalid;
			}
		}

		/**
		 *  [Mode Helper] Get the raw value of the given adapter mode
		 *
		 *  @param mode A valid adapter mode
		 *  @return The corresponding register value on success.
		 *          If the given mode is `Invalid`, the raw value of `LS` mode will be returned.
		 */
		static inline uint8_t getModeValue(Mode mode) {
			switch (mode) {
				case Mode::LevelShifter:
					return DP_DUAL_MODE_LSPCON_MODE_LS;

				case Mode::ProtocolConverter:
					return DP_DUAL_MODE_LSPCON_MODE_PCON;

				default:
					return DP_DUAL_MODE_LSPCON_MODE_LS;
			}
		}

		/**
		 *  [Mode Helper] Get the string representation of the adapter mode
		 */
		static inline const char *getModeString(Mode mode) {
			switch (mode) {
				case Mode::LevelShifter:
					return "Level Shifter (DP++ to HDMI 1.4)";

				case Mode::ProtocolConverter:
					return "Protocol Converter (DP++ to HDMI 2.0)";

				default:
					return "Invalid";
			}
		}

		/**
		 *  [Convenient] Create the LSPCON driver for the given framebuffer
		 *
		 *  @param controller The opaque `AppleIntelFramebufferController` instance
		 *  @param framebuffer The framebuffer that owns this LSPCON chip
		 *  @param displayPath The corresponding opaque display path instance
		 *  @return A driver instance on success, `nullptr` otherwise.
		 *  @note This convenient initializer returns `nullptr` if it could not retrieve the framebuffer index.
		 */
		static LSPCON *create(void *controller, IORegistryEntry *framebuffer, void *displayPath) {
			// Guard: Framebuffer index should exist
			uint32_t index;
			if (!AppleIntelFramebufferExplorer::getIndex(framebuffer, index))
				return nullptr;

			// Call the private constructor
			return new LSPCON(controller, framebuffer, displayPath, index);
		}

		/**
		 *  [Convenient] Destroy the LSPCON driver safely
		 *
		 *  @param instance A nullable LSPCON driver instance
		 */
		static void deleter(LSPCON *instance NONNULL) { delete instance; }

		/**
		 *  Probe the onboard LSPCON chip
		 *
		 *  @return `kIOReturnSuccess` on success, errors otherwise
		 */
		IOReturn probe();

		/**
		 *  Get the current adapter mode
		 *
		 *  @param mode The current adapter mode on return.
		 *  @return `kIOReturnSuccess` on success, errors otherwise.
		 */
		IOReturn getMode(Mode *mode);

		/**
		 *  Change the adapter mode
		 *
		 *  @param newMode The new adapter mode
		 *  @return `kIOReturnSuccess` on success, errors otherwise.
		 *  @note This method will not return until `newMode` is effective.
		 *  @warning This method will return the result of the last attempt if timed out on waiting for `newMode` to be effective.
		 */
		IOReturn setMode(Mode newMode);

		/**
		 *  Change the adapter mode if necessary
		 *
		 *  @param newMode The new adapter mode
		 *  @return `kIOReturnSuccess` on success, errors otherwise.
		 *  @note This method is a wrapper of `setMode` and will only set the new mode if `newMode` is not currently effective.
		 *  @seealso `setMode(newMode:)`
		 */
		IOReturn setModeIfNecessary(Mode newMode);

		/**
		 *  Wake up the native DisplayPort AUX channel for this adapter
		 *
		 *  @return `kIOReturnSuccess` on success, other errors otherwise.
		 */
		IOReturn wakeUpNativeAUX();

		/**
		 *  Return `true` if the adapter is running in the given mode
		 *
		 *  @param mode The expected mode; one of `LS` and `PCON`
		 */
		inline bool isRunningInMode(Mode mode) {
			Mode currentMode;
			if (getMode(&currentMode) != kIOReturnSuccess) {
				DBGLOG("igfx", "LSPCON::isRunningInMode() Error: [FB%d] Failed to get the current adapter mode.", index);
				return false;
			}
			return mode == currentMode;
		}

	private:
		/// The 7-bit I2C slave address of the DisplayPort dual mode adapter
		static constexpr uint32_t DP_DUAL_MODE_ADAPTER_I2C_ADDR = 0x40;

		/// Register address to change the adapter mode
		static constexpr uint8_t DP_DUAL_MODE_LSPCON_CHANGE_MODE = 0x40;

		/// Register address to read the current adapter mode
		static constexpr uint8_t DP_DUAL_MODE_LSPCON_CURRENT_MODE = 0x41;

		/// Register value when the adapter is in **Level Shifter** mode
		static constexpr uint8_t DP_DUAL_MODE_LSPCON_MODE_LS = 0x00;

		/// Register value when the adapter is in **Protocol Converter** mode
		static constexpr uint8_t DP_DUAL_MODE_LSPCON_MODE_PCON = 0x01;

		/// IEEE OUI of Parade Technologies
		static constexpr uint32_t DP_DUAL_MODE_LSPCON_VENDOR_PARADE = 0x001CF8;

		/// IEEE OUI of MegaChips Corporation
		static constexpr uint32_t DP_DUAL_MODE_LSPCON_VENDOR_MEGACHIPS = 0x0060AD;

		/// Bit mask indicating that the DisplayPort dual mode adapter is of type 2
		static constexpr uint8_t DP_DUAL_MODE_TYPE_IS_TYPE2 = 0xA0;

		/// Bit mask indicating that the DisplayPort dual mode adapter has DPCD (LSPCON case)
		static constexpr uint8_t DP_DUAL_MODE_TYPE_HAS_DPCD = 0x08;

		/**
		 *  Represents all possible chip vendors
		 */
		enum class Vendor {
			MegaChips,
			Parade,
			Unknown
		};

		/// The opaque framebuffer controller instance
		void *controller;

		/// The framebuffer that owns this LSPCON chip
		IORegistryEntry *framebuffer;

		/// The corresponding opaque display path instance
		void *displayPath;

		/// The framebuffer index (for debugging purposes)
		uint32_t index;

		/**
		 *  Initialize the LSPCON chip for the given framebuffer
		 *
		 *  @param controller The opaque `AppleIntelFramebufferController` instance
		 *  @param framebuffer The framebuffer that owns this LSPCON chip
		 *  @param displayPath The corresponding opaque display path instance
		 *  @param index The framebuffer index (only for debugging purposes)
		 *  @seealso LSPCON::create(controller:framebuffer:displayPath:) to create the driver instance.
		 */
		LSPCON(void *controller, IORegistryEntry *framebuffer, void *displayPath, uint32_t index) {
			this->controller = controller;
			this->framebuffer = framebuffer;
			this->displayPath = displayPath;
			this->index = index;
		}

		/**
		 *  [Vendor Helper] Parse the adapter vendor from the adapter info
		 *
		 *  @param info A non-null DP++ adapter info instance
		 *  @return The vendor on success, `Unknown` otherwise.
		 */
		static inline Vendor parseVendor(DisplayPortDualModeAdapterInfo *info) {
			uint32_t oui = info->oui[0] << 16 | info->oui[1] << 8 | info->oui[2];
			switch (oui) {
				case DP_DUAL_MODE_LSPCON_VENDOR_PARADE:
					return Vendor::Parade;

				case DP_DUAL_MODE_LSPCON_VENDOR_MEGACHIPS:
					return Vendor::MegaChips;

				default:
					return Vendor::Unknown;
			}
		}

		/**
		 *  [Vendor Helper] Get the string representation of the adapter vendor
		 */
		static inline const char *getVendorString(Vendor vendor) {
			switch (vendor) {
				case Vendor::Parade:
					return "Parade";

				case Vendor::MegaChips:
					return "MegaChips";

				default:
					return "Unknown";
			}
		}

		/**
		 *  [DP++ Helper] Check whether this is a HDMI adapter based on the adapter info
		 *
		 *  @param info A non-null DP++ adapter info instance
		 *  @return `true` if this is a HDMI adapter, `false` otherwise.
		 */
		static inline bool isHDMIAdapter(DisplayPortDualModeAdapterInfo *info) {
			return memcmp(info->hdmiID, "DP-HDMI ADAPTOR\x04", 16) == 0;
		}

		/**
		 *  [DP++ Helper] Check whether this is a LSPCON adapter based on the adapter info
		 *
		 *  @param info A non-null DP++ adapter info instance
		 *  @return `true` if this is a LSPCON DP-HDMI adapter, `false` otherwise.
		 */
		static inline bool isLSPCONAdapter(DisplayPortDualModeAdapterInfo *info) {
			// Guard: Check whether it is a DP to HDMI adapter
			if (!isHDMIAdapter(info))
				return false;

			// Onboard LSPCON adapter must be of type 2 and have DPCD info
			return info->adapterID == (DP_DUAL_MODE_TYPE_IS_TYPE2 | DP_DUAL_MODE_TYPE_HAS_DPCD);
		}
	};

	/**
	 *  Represents the LSPCON chip info for a framebuffer
	 */
	struct FramebufferLSPCON {
		/**
		 *  Indicate whether this framebuffer has an onboard LSPCON chip
		 *
		 *  @note This value will be read from the IGPU property `framebuffer-conX-has-lspcon`.
		 *  @warning If not specified, assuming no onboard LSPCON chip for this framebuffer.
		 */
		uint32_t hasLSPCON {0};

		/**
		 *  User preferred LSPCON adapter mode
		 *
		 *  @note This value will be read from the IGPU property `framebuffer-conX-preferred-lspcon-mode`.
		 *  @warning If not specified, assuming `PCON` mode is preferred.
		 *  @warning If invalid mode value found, assuming `PCON` mode
		 */
		LSPCON::Mode preferredMode {LSPCON::Mode::ProtocolConverter};

		/**
		 *  The corresponding LSPCON driver; `NULL` if no onboard chip
		 */
		LSPCON *lspcon {nullptr};
	};

	/**
	 *  User-defined LSPCON chip info for all possible framebuffers
	 */
	FramebufferLSPCON lspcons[MaxFramebufferConnectorCount];

	/// MARK:- Manage user-defined LSPCON chip info for all framebuffers

	/**
	 *  Setup the LSPCON driver for the given framebuffer
	 *
	 *  @param that The opaque framebuffer controller instance
	 *  @param framebuffer The framebuffer that owns this LSPCON chip
	 *  @param displayPath The corresponding opaque display path instance
	 *  @note This method will update fields in `lspcons` accordingly on success.
	 */
	static void framebufferSetupLSPCON(void *that, IORegistryEntry *framebuffer, void *displayPath);

	/**
	 *  [Convenient] Check whether the given framebuffer has an onboard LSPCON chip
	 *
	 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
	 *  @return `true` if the framebuffer has an onboard LSPCON chip, `false` otherwise.
	 */
	static inline bool framebufferHasLSPCON(uint32_t index) {
		return callbackIGFX->lspcons[index].hasLSPCON;
	}

	/**
	 *  [Convenient] Check whether the given framebuffer already has LSPCON driver initialized
	 *
	 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
	 *  @return `true` if the LSPCON driver has already been initialized for this framebuffer, `false` otherwise.
	 */
	static inline bool framebufferHasLSPCONInitialized(uint32_t index) {
		return callbackIGFX->lspcons[index].lspcon != nullptr;
	}

	/**
	 *  [Convenient] Get the non-null LSPCON driver associated with the given framebuffer
	 *
	 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
	 *  @return The LSPCON driver instance.
	 */
	static inline LSPCON *framebufferGetLSPCON(uint32_t index) {
		return callbackIGFX->lspcons[index].lspcon;
	}

	/**
	 *  [Convenient] Set the non-null LSPCON driver for the given framebuffer
	 *
	 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
	 *  @param lspcon A non-null LSPCON driver instance associated with the given framebuffer
	 */
	static inline void framebufferSetLSPCON(uint32_t index, LSPCON *lspcon) {
		callbackIGFX->lspcons[index].lspcon = lspcon;
	}

	/**
	 *  [Convenient] Get the preferred LSPCON mode for the given framebuffer
	 *
	 *  @param index A **valid** framebuffer index; Must be less than `MaxFramebufferConnectorCount`
	 *  @return The preferred adapter mode.
	 */
	static inline LSPCON::Mode framebufferLSPCONGetPreferredMode(uint32_t index) {
		return callbackIGFX->lspcons[index].preferredMode;
	}

	/// MARK:- I2C-over-AUX Transaction APIs

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
	 *  @ref TODO: Add the link to the blog post. [Working In Progress]
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
	 *  @ref TODO: Add the link to the blog post. [Working In Progress]
	 */
	static IOReturn wrapWriteI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint16_t length, uint8_t *buffer, bool intermediate);

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
