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
			uint8_t FPFCameliaVersion           :1;
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
	 *  Framebuffer find / replace patches
	 */
	FramebufferPatch framebufferPatches[MaxFramebufferPatchCount] {};

	/**
	 *  External global variables
	 */
	void *gPlatformInformationList {nullptr};
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
	 *  Detected CPU generation of the host system
	 */
	CPUInfo::CpuGeneration cpuGeneration {};

	/**
	 *  Set to true if a black screen ComputeLaneCount patch is required
	 */
	bool blackScreenPatch {false};

	/**
	 *  Set to true if PAVP code should be disabled
	 */
	bool pavpDisablePatch {false};

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
	bool dumpPlatformTable {true};

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
	 *  PAVP session callback wrapper used to prevent freezes on incompatible PAVP certificates
	 */
	static IOReturn wrapPavpSessionCallback(void *intelAccelerator, int32_t sessionCommand, uint32_t sessionAppId, uint32_t *a4, bool flag);

	/**
	 *  DP ComputeLaneCount wrapper to report success on non-DP screens to avoid black screen
	 */
	static bool wrapComputeLaneCount(void *that, void *timing, uint32_t bpp, int32_t availableLanes, int32_t *laneCount);

	/**
	 *  copyExistingServices wrapper used to rename Gen6Accelerator from userspace calls
	 */
	static OSObject *wrapCopyExistingServices(OSDictionary *matching, IOOptionBits inState, IOOptionBits options);

	/**
	 *  IntelAccelerator::start wrapper to support vesa mode, force OpenGL, prevent fw loading, etc.
	 */
	static bool wrapAcceleratorStart(IOService *that, IOService *provider);

	/**
	 *  AppleIntelFramebufferController::getOSInformation wrapper to patch framebuffer data
	 */
	static uint64_t wrapGetOSInformation(void *that);

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
	 *
	 */
	void writePlatformListData(const char* subKeyName);

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
	void applyPlatformInformationPatchEx(T* frame) { }

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
