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
	FramebufferCFL framebufferPatch {};

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
	int (*orgReadAUX)(void *, void *, uint32_t, uint16_t, void *, void *) {nullptr};

	/**
	 *  Detected CPU generation of the host system
	 */
	CPUInfo::CpuGeneration cpuGeneration {};

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
	 *	- IGPU property enable-cfl-backlight-fix turns patch on
	 *  - laptop with CFL CPU and CFL IGPU drivers turns patch on
	 */
	CoffeeBacklightPatch cflBacklightPatch {CoffeeBacklightPatch::Off};

	/**
	 *  Patch the maximum link rate in the DPCD buffer read from the built-in display
	 */
	bool maxLinkRatePatch {false};
	
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
	 *  Set to true if Sandy Bridge Gen6Accelerator should be renamed
	 */
	bool moderniseAccelerator {false};

	/**
	 *  Set to true to avoid incompatible GPU firmware loading
	 */
	bool avoidFirmwareLoading {false};

	/**
	 *  Requires framebuffer modifications
	 */
	bool applyFramebufferPatch {false};

	/**
	 *  Perform framebuffer dump to /AppleIntelFramebufferNUM
	 */
	bool dumpFramebufferToDisk {false};

	/**
	 *  Perform platform table dump to ioreg
	 */
	bool dumpPlatformTable {false};

	/**
	 *  Perform automatic DP -> HDMI replacement
	 */
	bool hdmiAutopatch {false};

	/**
	 *  Load GuC firmware
	 */
	bool loadGuCFirmware {false};

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
	 *  Represents the first 16 fields of the receiver capabilities defined in DPCD
	 *
	 *  Main Reference:
	 *  - DisplayPort Specification Version 1.2
	 *
	 *  Side Reference:
	 *  - struct intel_dp @ line 1073 in intel_drv.h (Linux 4.19 Kernel)
	 *  - DP_RECEIVER_CAP_SIZE @ line 964 in drm_dp_helper.h
	 */
	struct DPCDCap16 // 16 bytes
	{
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
	uint8_t maxLinkRate {0x14};
	
	/**
	 *  ReadAUX wrapper to modify the maximum link rate valud in the DPCD buffer
	 */
	static int wrapReadAUX(void *that, IORegistryEntry *framebuffer, uint32_t address, uint16_t length, void *buffer, void *displayPath);

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
	static bool wrapGetOSInformation(void *that);

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
	 *  Load GuC-specific patches and hooks
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle for graphics driver
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void loadIGScheduler4Patches(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

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
};

#endif /* kern_igfx_hpp */
