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
#include "kern_igfx_backlight.hpp"

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
	uint32_t fPatchCursorMemorySize {};

	/**
	 *  Maximum find / replace patches
	 */
	static constexpr size_t MaxFramebufferPatchCount = 10;

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
	 *	First generation registers (Westmere).
	 */
	static constexpr uint32_t WESTMERE_TXA_CTL 					= 0x60100;
	static constexpr uint32_t WESTMERE_RXA_CTL 					= 0xF000C;
	static constexpr uint32_t WESTMERE_LINK_WIDTH_MASK	= 0xFFC7FFFF;
	static constexpr uint32_t WESTMERE_LINK_WIDTH_SHIFT = 19;

	/**
	 *  Framebuffer list, imported from the framebuffer kext
	 */
	void *gPlatformInformationList {nullptr};

	/**
	 *  Framebuffer list is in Sandy Bridge format
	 */
	bool gPlatformListIsSNB {false};

	/**
	 *  IGPU support
	 */
	bool gPlatformGraphicsSupported {true};

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
	 *  Original AppleIntelHDGraphicsFB::TrainFDI function
	 */
	mach_vm_address_t orgTrainFDI {};

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

	// The opaque framebuffer controller type on BDW+
	class AppleIntelFramebufferController;
	
	// Available on ICL+
	// Apple has refactored quite a large amount of code into a new class `AppleIntelPort` in the ICL graphics driver,
	// and the framebuffer controller now maintains an array of `ports`.
	class AppleIntelPort;
	
	/**
	 *  Get the real framebuffer in use from the given bundle index
	 */
	KernelPatcher::KextInfo *getRealFramebuffer(size_t index) {
		return (currentFramebuffer && currentFramebuffer->loadIndex == index) ? currentFramebuffer : currentFramebufferOpt;
	}
	
	//
	// MARK: - Patch Submodule & Injection Kits
	//
	
	/**
	 *  Describes how to inject code into a shared submodule
	 *
	 *  @tparam T Specify the type of the trigger
	 *  @tparam I Specify the type of the function to inject code
	 *  @example The trigger type can be an integer type to inject code based on a register address.
	 */
	template <typename T, typename I>
	struct InjectionDescriptor {
		/**
		 *  The trigger value to be monitored by the coordinator
		 */
		T trigger {};

		/**
		 *  A function to invoke when the trigger value is observed
		 *
		 *  @example One may monitor a specific register address and modify its value in the injector function.
		 */
		I injector {};

		/**
		 *  A pointer to the next descriptor in a linked list
		 */
		InjectionDescriptor *next {nullptr};

		/**
		 *  Create an injection descriptor conveniently
		 */
		InjectionDescriptor(T t, I i) :
			trigger(t), injector(i), next(nullptr) { }
		
		/**
		 *  Member type `Trigger` is required by the coordinator
		 */
		using Trigger = T;
		
		/**
		 *  Member type `Injector` is required by the coordinator
		 */
		using Injector = I;
	};
	
	/**
	 *  Represents a list of injection descriptors
	 *
	 *  @tparam D Specify the concrete type of the descriptor
	 */
	template <typename D>
	struct InjectionDescriptorList {
	private:
		/**
		 *  Head of the list
		 */
		D *head {nullptr};
		
		/**
		 *  Tail of the list
		 */
		D *tail {nullptr};
		
	public:
		/**
		 *  Get the injector function associated with the given trigger
		 *
		 *  @param trigger The trigger value
		 *  @return The injector function on success, `nullptr` if the given trigger is not in the list.
		 */
		typename D::Injector getInjector(typename D::Trigger trigger) {
			for (auto current = head; current != nullptr; current = current->next)
				if (current->trigger == trigger)
					return current->injector;
			
			return nullptr;
		}
		
		/**
		 *  Add an injection descriptor to the list
		 *
		 *  @param descriptor A non-null descriptor that specifies the trigger and the injector function
		 *  @warning Patch developers must ensure that triggers are unique.
		 *           The coordinator registers injections on a first come, first served basis.
		 *           i.e. The latter descriptor will NOT overwrite the injection function of the existing one.
		 *  @note This function assumes that the trigger value of the given descriptor has not been registered yet.
		 */
		void add(D *descriptor NONNULL) {
			// Sanitize garbage value
			descriptor->next = nullptr;
			
			// Append the new descriptor
			if (head == nullptr)
				// Empty list
				head = descriptor;
			else
				// Non-empty list
				tail->next = descriptor;
			
			// Update the tail
			tail = descriptor;
		}
	};
	
	/**
	 *  An injection coordinator is capable of coordinating multiple requests of injection to a shared function
	 *
	 *  @tparam P Specify the type of the prologue injection descriptor that defines how the coordinator injects code before it calls the original function
	 *  @tparam R Specify the type of the replacer injection descriptor that defines how the coordinator replaces the original function implementation
	 *  @tparam E Specify the type of the epilogue injection descriptor that defines how the coordinator injects code after it calls the original function
	 *  @note Patch submodules inherited from this class get coordination support automatically.
	 *  @note Patch submodules invoke the `add()` method of a list to register injections.
	 */
	template <typename P, typename R, typename E>
	class InjectionCoordinator {
	public:
		/**
		 *  Virtual destructor
		 */
		virtual ~InjectionCoordinator() = default;
		
		/**
		 *  A list of prologue injection descriptors
		 */
		InjectionDescriptorList<P> prologueList {};
		
		/**
		 *  A list of replacer injection descriptors
		 */
		InjectionDescriptorList<R> replacerList {};
		
		/**
		 *  A list of epilogue injection descriptors
		 */
		InjectionDescriptorList<E> epilogueList {};
	};
	
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
		 *  Set to `true` if this submodule should be enabled
		 */
		bool enabled {false};
		
		/**
		 *  Set to `true` if this submodule requires patching the framebuffer driver
		 */
		bool requiresPatchingFramebuffer {false};
		
		/**
		 *  Set to `true` if this submodule requires patching the graphics acceleration driver
		 */
		bool requiresPatchingGraphics {false};
		
		/**
		 *  Set to `true` if this submodule requires accessing global framebuffer controllers
		 */
		bool requiresGlobalFramebufferControllersAccess {false};
		
		/**
		 *  Set to `true` if this submodules requires read access to MMIO registers
		 */
		bool requiresMMIORegistersReadAccess {false};
		
		/**
		 *  Set to `true` if this submodules requires write access to MMIO registers
		 */
		bool requiresMMIORegistersWriteAccess {false};
		
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
		
		/**
		 *  Disable submodules that depend on this submodules
		 */
		virtual void disableDependentSubmodules() {}
	};
	
	//
	// MARK: - Shared Submodules
	//
	
	/**
	 *  A submodule to provide shared access to global framebuffer controllers
	 */
	class FramebufferControllerAccessSupport: public PatchSubmodule {
		/**
		 *  An array of framebuffer controllers populated by `AppleIntelFramebufferController::start()`
		 */
		AppleIntelFramebufferController **controllers {};
		
	public:
		/**
		 *  Get the framebuffer controller at the given index
		 */
		AppleIntelFramebufferController *getController(size_t index) { return controllers[index]; }
		
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
		void disableDependentSubmodules() override;
	} modFramebufferControllerAccessSupport;
	
	/**
	 *  Defines the prologue injection descriptor for `AppleIntelFramebufferController::ReadRegister32()`
	 *
	 *  @note The trigger is the register address.
	 *  @note The injector function takes the controller along with the register address and returns void.
	 *  @note The injection is performed before the original function is invoked.
	 */
	using MMIOReadPrologue = InjectionDescriptor<uint32_t, void (*)(void *, uint32_t)>;
	
	/**
	 *  Defines the replacer injection descriptor for `AppleIntelFramebufferController::ReadRegister32()`
	 *
	 *  @note The trigger is the register address.
	 *  @note The injector function takes the controller along with the register address and returns the register value.
	 *  @note The injection replaced the original function call.
	 */
	using MMIOReadReplacer = InjectionDescriptor<uint32_t, uint32_t (*)(void *, uint32_t)>;
	
	/**
	 *  Defines the epilogue injection descriptor for `AppleIntelFramebufferController::ReadRegister32()`
	 *
	 *  @note The trigger is the register address.
	 *  @note The injector function takes the controller along with the register address and its value, and returns the new value.
	 *  @note The injection is performed after the original function is invoked.
	 */
	using MMIOReadEpilogue = InjectionDescriptor<uint32_t, uint32_t (*)(void *, uint32_t, uint32_t)>;
	
	/**
	 *  A submodule that provides read access to MMIO registers and coordinates injections to the read function
	 */
	class MMIORegistersReadSupport: public PatchSubmodule, public InjectionCoordinator<MMIOReadPrologue, MMIOReadReplacer, MMIOReadEpilogue> {
		/**
		 *  Set to `true` to print detailed register access information
		 */
		bool verbose {false};
		
	public:
		/**
		 *  Original AppleIntelFramebufferController::ReadRegister32 function
		 *
		 *  @note Other submodules may use this function pointer to skip injected code.
		 */
		uint32_t (*orgReadRegister32)(void *, uint32_t) {nullptr};
		
		/**
		 *  Wrapper for the AppleIntelFramebufferController::ReadRegister32 function
		 *
		 *  @param controller The implicit controller instance
		 *  @param address The register address
		 *  @return The register value.
		 *  @note This wrapper function monitors the register address and invokes registered injectors.
		 */
		static uint32_t wrapReadRegister32(void *controller, uint32_t address);
		
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
		void disableDependentSubmodules() override;
	} modMMIORegistersReadSupport;
	
	/**
	 *  Defines the injection descriptor for `AppleIntelFramebufferController::WriteRegister32()`
	 *
	 *  @note The trigger is the register address.
	 *  @note The injector function takes the controller along with the register address and its new value, and returns void.
	 *  @note This type is shared by all three kinds of injection descriptors.
	 */
	using MMIOWriteInjectionDescriptor = InjectionDescriptor<uint32_t, void (*)(void *, uint32_t, uint32_t)>;
	
	/**
	 *  A submodule that provides write access to MMIO registers and coordinates injections to the write function
	 */
	class MMIORegistersWriteSupport: public PatchSubmodule, public InjectionCoordinator<MMIOWriteInjectionDescriptor, MMIOWriteInjectionDescriptor, MMIOWriteInjectionDescriptor> {
		/**
		 *  Set to `true` to print detailed register access information
		 */
		bool verbose {false};
		
	public:
		/**
		 *  Original AppleIntelFramebufferController::WriteRegister32 function
		 *
		 *  @note Other submodules may use this function pointer to skip injected code.
		 */
		void (*orgWriteRegister32)(void *, uint32_t, uint32_t) {nullptr};
		
		/**
		 *  Wrapper for the AppleIntelFramebufferController::WriteRegister32 function
		 *
		 *  @param controller The implicit controller instance
		 *  @param address The register address
		 *  @param value The new register value
		 *  @note This wrapper function monitors the register address and invokes registered injectors.
		 */
		static void wrapWriteRegister32(void *controller, uint32_t address, uint32_t value);
		
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
		void disableDependentSubmodules() override;
	} modMMIORegistersWriteSupport;
	
	/**
	 *  [Convenient] Get the default framebuffer controller
	 */
	AppleIntelFramebufferController *defaultController() {
		return modFramebufferControllerAccessSupport.getController(0);
	}
	
	/**
	 *  [Convenient] Invoke the original AppleIntelFramebufferController::ReadRegister32 function
	 *
	 *  @param controller The framebuffer controller instance
	 *  @param address The register address
	 *  @return The register value.
	 */
	uint32_t readRegister32(void *controller, uint32_t address) {
		return modMMIORegistersReadSupport.orgReadRegister32(controller, address);
	}
	
	/**
	 *  [Convenient] Invoke the original AppleIntelFramebufferController::WriteRegister32 function
	 *
	 *  @param controller The framebuffer controller instance
	 *  @param address The register address
	 *  @param value The new register value
	 */
	void writeRegister32(void *controller, uint32_t address, uint32_t value) {
		modMMIORegistersWriteSupport.orgWriteRegister32(controller, address, value);
	}
	
	//
	// MARK: - Individual Fixes
	//
	
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
			uint64_t minDeviation {0};

			/// The current chosen central frequency
			uint64_t central {0};

			/// The current DCO frequency
			uint64_t frequency {0};

			/// The current selected divider
			uint32_t divider {0};

			/// The corresponding pdiv value [P0]
			uint32_t pdiv {0};

			/// The corresponding qdiv value [P1]
			uint32_t qdiv {0};

			/// The corresponding kqiv value [P2]
			uint32_t kdiv {0};
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
		FramebufferLSPCON lspcons[MaxFramebufferConnectorCount] {};
		
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
	 *  A submodule that patches RPS control for all command streamers
	 */
	class RPSControlPatch: public PatchSubmodule {
		uint32_t freq_max {0};
		bool patchRCSCheck(mach_vm_address_t& start);
		int (*orgPmNotifyWrapper)(unsigned int, unsigned int, unsigned long long *, unsigned int *) {nullptr};
		static int wrapPmNotifyWrapper(unsigned int, unsigned int, unsigned long long *, unsigned int *);
		
	public:
		/**
		 *  True if this fix is available for the current Intel platform
		 */
		bool available {false};
		
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
		void processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modRPSControlPatch;
	
	/**
	 *  A submodule that fixes the kernel panic due to a rare force wake timeout on KBL and CFL platforms.
	 */
	class ForceWakeWorkaround: public PatchSubmodule {
		static bool pollRegister(uint32_t, uint32_t, uint32_t, uint32_t);
		static bool forceWakeWaitAckFallback(uint32_t, uint32_t, uint32_t);
		static void forceWake(void *, uint8_t set, uint32_t dom, uint32_t);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modForceWakeWorkaround;
	
	/**
	 *  A submodule that applies patches based on the framebuffer index
	 */
	class FramebufferModifer: public PatchSubmodule {
	protected:
		/**
		 *  Indices of framebuffers to be patched
		 */
		uint8_t fbs[sizeof(uint64_t)] {};

		/**
		 *  Check whether the given framebuffer is in the list
		 */
		bool inList(IORegistryEntry* fb) {
			uint32_t idx;
			if (AppleIntelFramebufferExplorer::getIndex(fb, idx))
				for (auto i : fbs)
					if (i == idx)
						return true;
			return false;
		}
		
	public:
		/**
		 *  `True` if this patch is supported on the current platform
		 */
		bool supported {false};
		
		/**
		 *  `True` if the current platform is Skylake
		 */
		bool legacy {false};
		
		/**
		 *  `True` if patch behavior should be overridden
		 */
		bool customised {false};
	};
	
	/**
	 *  A submodule to ensure that each modeset operation is a complete one
	 */
	class ForceCompleteModeset: public FramebufferModifer {
		/**
		 *  Original AppleIntelFramebufferController::hwRegsNeedUpdate function
		 */
		bool (*orgHwRegsNeedUpdate)(void *, IORegistryEntry *, void *, void *, void *) {nullptr};
		
		/**
		 *  Wrapper to force a complete modeset
		 */
		static bool wrapHwRegsNeedUpdate(void *controller, IORegistryEntry *framebuffer, void *displayPath, void *crtParams, void *detailedInfo);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modForceCompleteModeset;
	
	/**
	 *  A submodule to ensure that each display is online
	 */
	class ForceOnlineDisplay: public FramebufferModifer {
		/**
		 *  Original AppleIntelFramebuffer::getDisplayStatus function
		 */
		uint32_t (*orgGetDisplayStatus)(IORegistryEntry *, void *) {nullptr};
		
		/**
		 *  Wrapper to report that a display is online
		 */
		static uint32_t wrapGetDisplayStatus(IORegistryEntry *framebuffer, void *displayPath);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modForceOnlineDisplay;
	
	/**
	 *  A submodule that disables Apple's Graphics Device Control (AGDC)
	 */
	class AGDCDisabler: public PatchSubmodule {
		/**
		 *  Original IntelFBClientControl::doAttribute function
		 */
		IOReturn (*orgFBClientDoAttribute)(void *, uint32_t, unsigned long *, unsigned long, unsigned long *, unsigned long *, void *) {nullptr};
		
		/**
		 *  A wrapper to ignore AGDC registration request
		 */
		static IOReturn wrapFBClientDoAttribute(void *fbclient, uint32_t attribute, unsigned long *unk1, unsigned long unk2, unsigned long *unk3, unsigned long *unk4, void *externalMethodArguments);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modAGDCDisabler;
	
	/**
	 *  A submodule that disables the check of Type-C platforms
	 */
	class TypeCCheckDisabler: public PatchSubmodule {
		/**
		 *  A wrapper to always report that this is not a Type-C platform
		 *
		 *  @note Apparently, platforms with (ig-platform-id & 0xf != 0) have only Type C connectivity.
		 *        Framebuffer kext uses this fact to sanitise connector type, forcing it to DP.
		 *        This breaks many systems, so we undo this check.
		 *        Affected drivers: KBL and newer?
		 */
		static bool wrapIsTypeCOnlySystem(void *controller);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modTypeCCheckDisabler;
	
	/**
	 *  A submodule to fix the black screen on external HDMI/DVI displays on SKL/KBL/CFL platforms
	 */
	class BlackScreenFix: public PatchSubmodule {
		/**
		 *  [Legacy] Original AppleIntelFramebufferController::ComputeLaneCount function
		 *
		 *  @note This function is used for DP lane count calculation.
		 */
		bool (*orgComputeLaneCount)(void *, void *, uint32_t, int, int *) {nullptr};
		
		/**
		 *  [Nouveau] Original AppleIntelFramebufferController::ComputeLaneCount function
		 *
		 *  @note This function is used for DP lane count calculation.
		 *  @note Available on KBL+ and as of macOS 10.14.1.
		 */
		bool (*orgComputeLaneCountNouveau)(void *, void *, int, int *) {nullptr};
		
		/**
		 *  [Legacy] A wrapper to report a working lane count for HDMI/DVI connections
		 */
		static bool wrapComputeLaneCount(void *controller, void *detailedTiming, uint32_t bpp, int availableLanes, int *laneCount);
		
		/**
		 *  [Nouveau] A wrapper to report a working lane count for HDMI/DVI connections
		 *
		 *  @note Available on KBL+ and as of macOS 10.14.1.
		 */
		static bool wrapComputeLaneCountNouveau(void *controller, void *detailedTiming, int availableLanes, int *laneCount);
		
	public:
		/**
		 *  True if the current platform is supported
		 */
		bool available {false};
		
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modBlackScreenFix;
	
	/**
	 *  A submodule to disable PAVP code and thus prevent freezes
	 */
	class PAVPDisabler: public PatchSubmodule {
		/**
		 *  Original PAVP session callback function used for PAVP command handling
		 */
		IOReturn (*orgPavpSessionCallback)(void *, int32_t, uint32_t, uint32_t *, bool) {nullptr};
		
		/**
		 *  PAVP session callback wrapper used to prevent freezes on incompatible PAVP certificates
		 */
		static IOReturn wrapPavpSessionCallback(void *intelAccelerator, int32_t sessionCommand, uint32_t sessionAppId, uint32_t *a4, bool flag);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modPAVPDisabler;
	
	/**
	 *  A submodule to patch read descriptors and thus avoid random kernel panics on SKL+
	 */
	class ReadDescriptorPatch: public PatchSubmodule {
		/**
		 *  Global page table read wrapper for Kaby Lake.
		 */
		static bool globalPageTableRead(void *hardwareGlobalPageTable, uint64_t a1, uint64_t &a2, uint64_t &a3);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modReadDescriptorPatch;
	
	/**
	 *  A submodule to patch backlight register values and thus avoid 3-minute black screen on KBL+
	 *
	 *  @note Supported Platforms: KBL, CFL, ICL.
	 */
	class BacklightRegistersFix: public PatchSubmodule {		
		/**
		 *  Fallback user-requested backlight frequency in case 0 was initially written to the register.
		 */
		static constexpr uint32_t FallbackTargetBacklightFrequency = 120000;
		
		/**
		 *  [COMM] User-requested backlight frequency obtained from BXT_BLC_PWM_FREQ1 at system start.
		 *
		 *  @note Its value can be specified via max-backlight-freq property.
		 */
		uint32_t targetBacklightFrequency {};
		
		/**
		 *  [KBL ] User-requested pwm control value obtained from BXT_BLC_PWM_CTL1.
		 */
		uint32_t targetPwmControl {};
		
		/**
		 *  [CFL+] Driver-requested backlight frequency obtained from BXT_BLC_PWM_FREQ1 write attempt at system start.
		 */
		uint32_t driverBacklightFrequency {};
		
		/**
		 *  [KBL ] Wrapper to fix the value of BXT_BLC_PWM_FREQ1
		 *
		 *  @note When this function is called, `reg` is guaranteed to be `BXT_BLC_PWM_FREQ1`.
		 */
		static void wrapKBLWriteRegisterPWMFreq1(void *controller, uint32_t reg, uint32_t value);
		
		/**
		 *  [KBL ] Wrapper to fix the value of BXT_BLC_PWM_CTL1
		 *
		 *  @note When this function is called, `reg` is guaranteed to be `BXT_BLC_PWM_CTL1`.
		 */
		static void wrapKBLWriteRegisterPWMCtrl1(void *controller, uint32_t reg, uint32_t value);
		
		/**
		 *  [CFL+] Wrapper to fix the value of BXT_BLC_PWM_FREQ1
		 *
		 *  @note When this function is called, `reg` is guaranteed to be `BXT_BLC_PWM_FREQ1`.
		 */
		static void wrapCFLWriteRegisterPWMFreq1(void *controller, uint32_t reg, uint32_t value);
		
		/**
		 *  [CFL+] Wrapper to fix the value of BXT_BLC_PWM_DUTY1
		 *
		 *  @note When this function is called, `reg` is guaranteed to be `BXT_BLC_PWM_DUTY1`.
		 */
		static void wrapCFLWriteRegisterPWMDuty1(void *controller, uint32_t reg, uint32_t value);
		
		/**
		 *  [KBL ] A replacer descriptor that injects code when the register of interest is BXT_BLC_PWM_FREQ1
		 */
		MMIOWriteInjectionDescriptor dKBLPWMFreq1 {BXT_BLC_PWM_FREQ1, wrapKBLWriteRegisterPWMFreq1};
		
		/**
		 *  [KBL ] A replacer descriptor that injects code when the register of interest is BXT_BLC_PWM_CTL1
		 */
		MMIOWriteInjectionDescriptor dKBLPWMCtrl1 {BXT_BLC_PWM_CTL1 , wrapKBLWriteRegisterPWMCtrl1};
		
		/**
		 *  [CFL+] A replacer descriptor that injects code when the register of interest is BXT_BLC_PWM_FREQ1
		 */
		MMIOWriteInjectionDescriptor dCFLPWMFreq1 {BXT_BLC_PWM_FREQ1, wrapCFLWriteRegisterPWMFreq1};
		
		/**
		 *  [CFL+] A replacer descriptor that injects code when the register of interest is BXT_BLC_PWM_DUTY1
		 */
		MMIOWriteInjectionDescriptor dCFLPWMDuty1 {BXT_BLC_PWM_DUTY1, wrapCFLWriteRegisterPWMDuty1};
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modBacklightRegistersFix;
	
	/**
	 *  Brightness request event source needs access to the original WriteRegister32 function
	 */
	friend class BrightnessRequestEventSource;

	/**
	 *  A submodule to make brightness transitions smoother
	 *
	 *  @note This submodule must be placed AFTER the backlight registers fix (BLR) module.
	 *        If BLR is enabled, it rescales the value to be written to `BXT_BLC_PWM_DUTY1`
	 *        and then passes the rescaled value to `smoothCFLWriteRegisterPWMDuty1`.
	 */
	class BacklightSmoother: public PatchSubmodule {
		/**
		 *  Backlight registers fix submodule needs access to the smoother version of `WriteRegister32()`
		 */
		friend class BacklightRegistersFix;

		/**
		 *  Brightness request event source needs access to the queue and config parameters
		 */
		friend class BrightnessRequestEventSource;

		/**
		 *  Default number of steps to reach the target duty value
		 */
		static constexpr uint32_t kDefaultSteps = 35;

		/**
		 *  Default interval in milliseconds between each step
		 */
		static constexpr uint32_t kDefaultInterval = 7;

		/**
		 *  Default threshold value of skipping the smoother
		 */
		static constexpr uint32_t kDefaultThreshold = 0;

		/**
		 *  Default length of the brightness request queue
		 */
		static constexpr uint32_t kDefaultQueueSize = 64;
		
		/**
		 *  Minimum length of the brightness request queue
		 */
		static constexpr uint32_t kMinimumQueueSize = 32;
		
		/**
		 *  The total number of steps to reach the target duty value
		 */
		uint32_t steps {kDefaultSteps};

		/**
		 *  Interval in milliseconds between each step
		 */
		uint32_t interval {kDefaultInterval};

		/**
		 *  Skip the smoother if the distance to the target duty value falls below the threshold
		 */
		uint32_t threshold {kDefaultThreshold};
		
		/**
		 *  The size of the brightness request queue
		 */
		uint32_t queueSize {kDefaultQueueSize};
		
		/**
		 *  The range of the brightness level (represented as register values)
		 */
		ppair<uint32_t, uint32_t> brightnessRange {0, UINT32_MAX};
		
		/**
		 *  Owner of the event source
		 */
		OSObject *owner {nullptr};

		/**
		 *  A list of pending brightness adjustment requests
		 */
		BrightnessRequest request;

		/**
		 *  A workloop that provides a kernel thread to adjust the brightness
		 */
		IOWorkLoop *workloop {nullptr};

		/**
		 *  A custom event source that adjusts the brightness smoothly
		 */
		BrightnessRequestEventSource *eventSource {nullptr};

		/**
		 *  [IVB ] Wrapper to write to BLC_PWM_CPU_CTL smoothly
		 *
		 *  @note When this function is called, `address` is guaranteed to be `BLC_PWM_CPU_CTL`.
		 */
		static void smoothIVBWriteRegisterPWMCCTRL(void *controller, uint32_t address, uint32_t value);

		/**
		 *  [HSW+] Wrapper to write to BXT_BLC_PWM_FREQ1 smoothly
		 *
		 *  @note When this function is called, `address` is guaranteed to be `BXT_BLC_PWM_FREQ1`.
		 */
		static void smoothHSWWriteRegisterPWMFreq1(void *controller, uint32_t address, uint32_t value);

		/**
		 *  [CFL+] Wrapper to write to BXT_BLC_PWM_DUTY1 smoothly
		 *
		 *  @note When this function is called, `address` is guaranteed to be `BXT_BLC_PWM_DUTY1`.
		 */
		static void smoothCFLWriteRegisterPWMDuty1(void *controller, uint32_t address, uint32_t value);

		/**
		 *  [IVB ] A replacer descriptor that injects code when the register of interest is BLC_PWM_CPU_CTL
		 */
		MMIOWriteInjectionDescriptor dIVBPWMCCTRL {BLC_PWM_CPU_CTL,   smoothIVBWriteRegisterPWMCCTRL};

		/**
		 *  [HSW+] A replacer descriptor that injects code when the register of interest is BXT_BLC_PWM_FREQ1
		 */
		MMIOWriteInjectionDescriptor dHSWPWMFreq1 {BXT_BLC_PWM_FREQ1, smoothHSWWriteRegisterPWMFreq1};

		/**
		 *  [CFL+] A replacer descriptor that injects code when the register of interest is BXT_BLC_PWM_DUTY1
		 */
		MMIOWriteInjectionDescriptor dCFLPWMDuty1 {BXT_BLC_PWM_DUTY1, smoothCFLWriteRegisterPWMDuty1};

	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void deinit() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modBacklightSmoother;
	
	/**
	 *  A submodule to provide support for debugging the framebuffer driver
	 */
	class FramebufferDebugSupport: public PatchSubmodule {
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modFramebufferDebugSupport;
	
	/**
	 *  A submodule to override the max pixel clock limit in the framebuffer driver.
	 */
	class MaxPixelClockOverride: public PatchSubmodule {
		/**
		 * Override for max pixel clock frequency (Hz).
		 */
		uint64_t maxPixelClockFrequency = 675000000;

		/**
		 *  Original AppleIntelFramebuffer::connectionProbe function
		 */
		IOReturn (*orgConnectionProbe)(IOService *, unsigned int, unsigned int) {nullptr};

		/**
		 *  AppleIntelFramebuffer::connectionProbe wrapper to override max pixel clock
		 */
		static IOReturn wrapConnectionProbe(IOService *that, unsigned int unk1, unsigned int unk2);

	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modMaxPixelClockOverride;
	
	/**
	 *  A submodule to optimize the display data buffer position earlier to solve the 10-second flicker issue on ICL+
	 */
	class DisplayDataBufferEarlyOptimizer: public PatchSubmodule {
		/**
		 *  The feature control key
		 */
		static constexpr const char* kFeatureControl = "FeatureControl";
		
		/**
		 *  The display data buffer optimizer delay key
		 */
		static constexpr const char* kOptimizerTime = "DBUFOptimizeTime";
		
		/**
		 *  The default optimizer delay is 1 second
		 *
		 *  @note The community reports that the previous default delay (0 second) used v1.5.4
		 *        may result in both internal and external displays flickering on some laptops.
		 *        Their experiment suggests that a delay of 1 to 3 seconds seems to be safe and solves
		 *        the underrun issue on the builtin display without having impacts on the external ones.
		 *  @note Thanks @m0d16l14n1 for the suggestion and collecting results from other testers.
		 */
		static constexpr uint32_t kDefaultOptimizerTime = 1;
		
		/**
		 *  Specify the amount of time in seconds to delay the execution of optimizing the display data buffer allocation
		 */
		uint32_t optimizerTime {kDefaultOptimizerTime};
		
		/**
		 *  Original AppleIntelFramebufferController::getFeatureControl function
		 */
		void (*orgGetFeatureControl)(IOService *controller) {nullptr};
		
		/**
		 *  Fetch and load the featuer control information
		 *
		 *  @param controller The hidden implicit `this` pointer
		 */
		static void wrapGetFeatureControl(IOService *controller);
		
	public:
		// MARK: Patch Submodule IMP
		void init() override;
		void processKernel(KernelPatcher &patcher, DeviceInfo *info) override;
		void processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) override;
	} modDisplayDataBufferEarlyOptimizer;
	
	/**
	 *  A collection of shared submodules
	 */
	PatchSubmodule *sharedSubmodules[3] = {
		&modFramebufferControllerAccessSupport,
		&modMMIORegistersReadSupport,
		&modMMIORegistersWriteSupport
	};
	
	/**
	 *  A collection of submodules
	 */
	PatchSubmodule *submodules[20] = {
		&modDVMTCalcFix,
		&modDPCDMaxLinkRateFix,
		&modCoreDisplayClockFix,
		&modHDMIDividersCalcFix,
		&modLSPCONDriverSupport,
		&modAdvancedI2COverAUXSupport,
		&modRPSControlPatch,
		&modForceWakeWorkaround,
		&modForceCompleteModeset,
		&modForceOnlineDisplay,
		&modAGDCDisabler,
		&modTypeCCheckDisabler,
		&modBlackScreenFix,
		&modPAVPDisabler,
		&modReadDescriptorPatch,
		&modBacklightRegistersFix,
		&modBacklightSmoother,
		&modFramebufferDebugSupport,
		&modMaxPixelClockOverride,
		&modDisplayDataBufferEarlyOptimizer,
	};
	
	/**
	 * Prevent IntelAccelerator from starting.
	 */
	bool disableAccel {false};
	
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
	static bool wrapGetOSInformation(IOService *that);

	/**
	 *  IGHardwareGuC::loadGuCBinary wrapper to feed updated (compatible GuC)
	 */
	static bool wrapLoadGuCBinary(void *that, bool flag);
	
	/**
	 *	AppleIntelHDGraphicsFB::TrainFDI wrapper to set correct link width settings on device
	 */
	static void wrapTrainFDI(IOService *that, int32_t value, void *params);

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
