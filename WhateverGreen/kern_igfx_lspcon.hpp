//
//  kern_igfx_lspcon.hpp
//  WhateverGreen
//
//  Created by FireWolf on 9/21/20.
//  Copyright © 2020 vit9696. All rights reserved.
//

#ifndef kern_igfx_lspcon_hpp
#define kern_igfx_lspcon_hpp

#include <mach/mach_types.h>
#include <IOKit/IOService.h>
#include <Headers/kern_util.hpp>

/**
 *  Represents the register layouts of DisplayPort++ adapter at I2C address 0x40
 *
 *  Specific to LSPCON DisplayPort 1.2 to HDMI 2.0 Adapter
 */
struct DisplayPortDualModeAdapterInfo { // first 128 bytes
	/// [0x00] HDMI ID
	///
	/// Fixed Value: "DP-HDMI ADAPTOR\x04"
	uint8_t hdmiID[16] {};

	/// [0x10] Adapter ID
	///
	/// Bit Masks:
	/// - isType2 = 0xA0
	/// - hasDPCD = 0x08
	///
	/// Sample Values: 0xA8 = Type 2 Adapter with DPCD
	uint8_t adapterID {};

	/// [0x11] IEEE OUI
	///
	/// Sample Value: 0x001CF8 [Parade]
	/// Reference: http://standards-oui.ieee.org/oui.txt
	uint8_t oui[3] {};

	/// [0x14] Device ID
	///
	/// Sample Value: 0x505331373530 = "PS1750"
	uint8_t deviceID[6] {};

	/// [0x1A] Hardware Revision Number
	///
	/// Sample Value: 0xB2 (B2 version)
	uint8_t revision {};

	/// [0x1B] Firmware Major Revision
	uint8_t firmwareMajor {};

	/// [0x1C] Firmware Minor Revision
	uint8_t firmwareMinor {};

	/// [0x1D] Maximum TMDS Clock
	uint8_t maxTMDSClock {};

	/// [0x1E] I2C Speed Capability
	uint8_t i2cSpeedCap {};

	/// [0x1F] Unused/Reserved Field???
	uint8_t reserved0 {};

	/// [0x20] TMDS Output Buffer State
	///
	/// Bit Masks:
	/// - Disabled = 0x01
	///
	/// Sample Value:
	/// 0x00 = Enabled
	uint8_t tmdsOutputBufferState {};

	/// [0x21] HDMI PIN CONTROL
	uint8_t hdmiPinCtrl {};

	/// [0x22] I2C Speed Control
	uint8_t i2cSpeedCtrl {};

	/// [0x23 - 0x3F] Unused/Reserved Fields
	uint8_t reserved1[29] {};

	/// [0x40] [W] Set the new LSPCON mode
	uint8_t lspconChangeMode {};

	/// [0x41] [R] Get the current LSPCON mode
	///
	/// Bit Masks:
	/// - PCON = 0x01
	///
	/// Sample Value:
	/// 0x00 = LS
	/// 0x01 = PCON
	uint8_t lspconCurrentMode {};

	/// [0x42 - 0x7F] Rest Unused/Reserved Fields
	uint8_t reserved2[62] {};
};

/**
 *  Represents the onboard Level Shifter and Protocol Converter
 */
class LSPCON {
public:
	/**
	 *  Defines all possible adapter modes and convenient helpers to interpret a mode
	 *  A wrapper to support member functions on an enumeration case
	 */
	struct Mode {
	public:
		/**
		 *  Enumerates all possible values
		 */
		enum class Value : uint8_t {
			/// Level Shifter Mode 		(DP++ to HDMI 1.4)
			LevelShifter,

			/// Protocol Converter Mode (DP++ to HDMI 2.0)
			ProtocolConverter,

			/// Invalid Mode
			Invalid
		};
		
		/**
		 *  Default constructor
		 */
		Mode(Value value = Value::Invalid) : value(value) {}
		
		/**
		 *  [Mode Helper] Parse the adapter mode from the raw register value
		 *
		 *  @param mode A raw register value read from the adapter.
		 *  @return The corresponding adapter mode on success, `invalid` otherwise.
		 */
		static inline Mode parse(uint8_t mode) {
			switch (mode & DP_DUAL_MODE_LSPCON_MODE_PCON) {
				case DP_DUAL_MODE_LSPCON_MODE_LS:
					return { Value::LevelShifter };

				case DP_DUAL_MODE_LSPCON_MODE_PCON:
					return { Value::ProtocolConverter };

				default:
					return { Value::Invalid };
			}
		}
		
		/**
		 *  Get the raw value of this adapter mode
		 *
		 *  @param mode A valid adapter mode
		 *  @return The corresponding register value on success.
		 *          If the given mode is `Invalid`, the raw value of `LS` mode will be returned.
		 */
		inline uint8_t getRawValue()
		{
			switch (value) {
				case Value::LevelShifter:
					return DP_DUAL_MODE_LSPCON_MODE_LS;
					
				case Value::ProtocolConverter:
					return DP_DUAL_MODE_LSPCON_MODE_PCON;
				
				default:
					return DP_DUAL_MODE_LSPCON_MODE_LS;
			}
		}
		
		/**
		 *  Get the string representation of this adapter mode
		 */
		inline const char *getDescription() {
			switch (value) {
				case Value::LevelShifter:
					return "Level Shifter (DP++ to HDMI 1.4)";

				case Value::ProtocolConverter:
					return "Protocol Converter (DP++ to HDMI 2.0)";

				default:
					return "Invalid";
			}
		}
		
		/**
		 *  Check whether this is an invalid adapter mode
		 */
		inline bool isInvalid() {
			return value == Value::Invalid;
		}
		
		/**
		 *  Check whether this mode supports HDMI 2.0 output
		 */
		inline bool supportsHDMI20() {
			return value == Value::ProtocolConverter;
		}
		
		/**
		 *  Check whether two modes are identical
		 */
		friend bool operator==(const Mode& lhs, const Mode& rhs) { return lhs.value == rhs.value; }
		
	private:
		/**
		 *  Register value when the adapter is in **Level Shifter** mode
		 */
		static constexpr uint8_t DP_DUAL_MODE_LSPCON_MODE_LS = 0x00;

		/**
		 *  Register value when the adapter is in **Protocol Converter** mode
		 */
		static constexpr uint8_t DP_DUAL_MODE_LSPCON_MODE_PCON = 0x01;
		
		/**
		 *  The underlying mode value
		 */
		Value value {Value::Invalid};
	};
	
	/**
	 *  Defines all possible chip vendors and convenient helpers to parse and interpret a vendor
	 */
	struct Vendor {
	public:
		/**
		 *  Enumerates all possible values
		 */
		enum class Value : uint32_t {
			MegaChips,
			Parade,
			Unknown
		};
		
		/**
		 *  Default constructor
		 */
		Vendor(Value value = Value::Unknown) : value(value) {}
		
		/**
		 *  [Vendor Helper] Parse the adapter vendor from the adapter info
		 *
		 *  @param info A non-null DP++ adapter info instance
		 *  @return The vendor on success, `Unknown` otherwise.
		 */
		static inline Vendor parse(DisplayPortDualModeAdapterInfo *info) {
			uint32_t oui = info->oui[0] << 16 | info->oui[1] << 8 | info->oui[2];
			switch (oui) {
				case DP_DUAL_MODE_LSPCON_VENDOR_PARADE:
					return { Value::Parade };

				case DP_DUAL_MODE_LSPCON_VENDOR_MEGACHIPS:
					return { Value::MegaChips };

				default:
					return { Value::Unknown };
			}
		}

		/**
		 *  [Vendor Helper] Get the string representation of the adapter vendor
		 */
		inline const char *getDescription() {
			switch (value) {
				case Value::Parade:
					return "Parade";

				case Value::MegaChips:
					return "MegaChips";

				default:
					return "Unknown";
			}
		}
		
	private:
		/**
		 *  IEEE OUI of Parade Technologies
		 */
		static constexpr uint32_t DP_DUAL_MODE_LSPCON_VENDOR_PARADE = 0x001CF8;

		/**
		 *  IEEE OUI of MegaChips Corporation
		 */
		static constexpr uint32_t DP_DUAL_MODE_LSPCON_VENDOR_MEGACHIPS = 0x0060AD;
		
		/**
		 *  The underlying vendor value
		 */
		Value value {Value::Unknown};
	};

	/**
	 *  [Convenient] Create the LSPCON driver for the given framebuffer
	 *
	 *  @param controller The opaque `AppleIntelFramebufferController` instance
	 *  @param framebuffer The framebuffer that owns this LSPCON chip
	 *  @param displayPath The corresponding opaque display path instance
	 *  @return A driver instance on success, `nullptr` otherwise.
	 *  @note This convenient initializer returns `nullptr` if it could not retrieve the framebuffer index.
	 */
	static LSPCON *create(void *controller, IORegistryEntry *framebuffer, void *displayPath);

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
	IOReturn getMode(Mode &mode);

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
	bool isRunningInMode(Mode mode);

private:
	/// The 7-bit I2C slave address of the DisplayPort dual mode adapter
	static constexpr uint32_t DP_DUAL_MODE_ADAPTER_I2C_ADDR = 0x40;

	/// Register address to change the adapter mode
	static constexpr uint8_t DP_DUAL_MODE_LSPCON_CHANGE_MODE = 0x40;

	/// Register address to read the current adapter mode
	static constexpr uint8_t DP_DUAL_MODE_LSPCON_CURRENT_MODE = 0x41;

	/// Bit mask indicating that the DisplayPort dual mode adapter is of type 2
	static constexpr uint8_t DP_DUAL_MODE_TYPE_IS_TYPE2 = 0xA0;

	/// Bit mask indicating that the DisplayPort dual mode adapter has DPCD (LSPCON case)
	static constexpr uint8_t DP_DUAL_MODE_TYPE_HAS_DPCD = 0x08;
	
	/// The opaque framebuffer controller instance
	void *controller {nullptr};

	/// The framebuffer that owns this LSPCON chip
	IORegistryEntry *framebuffer {nullptr};

	/// The corresponding opaque display path instance
	void *displayPath {nullptr};

	/// The framebuffer index (for debugging purposes)
	uint32_t index {0};

	/**
	 *  Initialize the LSPCON chip for the given framebuffer
	 *
	 *  @param controller The opaque `AppleIntelFramebufferController` instance
	 *  @param framebuffer The framebuffer that owns this LSPCON chip
	 *  @param displayPath The corresponding opaque display path instance
	 *  @param index The framebuffer index (only for debugging purposes)
	 *  @seealso LSPCON::create(controller:framebuffer:displayPath:) to create the driver instance.
	 */
	LSPCON(void *controller, IORegistryEntry *framebuffer, void *displayPath, uint32_t index);

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
	LSPCON::Mode preferredMode { LSPCON::Mode::Value::ProtocolConverter };

	/**
	 *  The corresponding LSPCON driver; `NULL` if no onboard chip
	 */
	LSPCON *lspcon {nullptr};
};

#endif /* kern_igfx_lspcon_hpp */
