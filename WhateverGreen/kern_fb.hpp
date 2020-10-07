//
//  kern_fb.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_fb_hpp
#define kern_fb_hpp

#include <Headers/kern_util.hpp>

static constexpr size_t MaxFramebufferConnectorCount = 6;

union FramebufferFlags {
	struct FramebufferFlagBits {
		/* Discovered in AppleIntelFBController::LinkTraining. Disables the use of FastLinkTraining.
		 * According to joevt with zero SKL link training happens at 450 MHz else at 540 MHz */
		uint8_t FBAvoidFastLinkTraining      :1;  /* 0x1 */
		uint8_t FBUnknownFlag_2              :1;  /* 0x2 */
		/* Discovered in AppleIntelFramebufferController::getFeatureControl.
		 * This is equivalent to setting FBC=1 in the plist FeatureControl section. */
		uint8_t FBFramebufferCompression     :1;  /* 0x4 */
		/* Discovered in AppleIntelFramebufferController::getFeatureControl.
		 * This is equivalent to setting SliceSDEnable=1, EUSDEnable=1, DynamicSliceSwitch=1 in the plist FeatureControl section.
		 */
		uint8_t FBEnableSliceFeatures        :1;  /* 0x8 */
		/* Discovered in AppleIntelFramebufferController::getFeatureControl.
		 * This is equivalent to setting DynamicFBCEnable=1 in the plist FeatureControl section.
		 */
		uint8_t FBDynamicFBCEnable           :1;  /* 0x10 */
		/* This sets fUseVideoTurbo=1 and loads GPU turbo frequency from the specific field.
		 * Defaults to 14, can be overridden by VideoTurboFreq in the plist FeatureControl section.
		 */
		uint8_t FBUseVideoTurbo              :1;  /* 0x20 */
		/* Discovered in AppleIntelFramebuffer::getDisplayStatus.
		 * Enforces display power reset even on always connected displays (see connector flags CNConnectorAlwaysConnected).
		 */
		uint8_t FBForcePowerAlwaysConnected  :1;  /* 0x40 */
		/* According to joevt this enforces High Bitrate mode 1, which limits DP bitrate to 8.64 Gbit/s instead of normal 17.28 Gbit/s (HBR2).
		 * I do not think this is used on Skylake any longer.
		 */
		uint8_t FBDisableHighBitrateMode2    :1;  /* 0x80 */
		/* This bit is not used on Broadwell and newer but set when fPortCount > 0, i.e. for all online framebuffers.
		 * On Haswell it is used by AppleIntelFramebuffer::GetOnlineInfo and is set only on 0x0D260007 (MacBookPro11,3) and 0x0D26000E, which are top models.
		 * This appears to boost pixel frequency limit (aka pixel clock) to 540000000 Hz (from the usual 216000000, 320000000, 360000000, 450000000).
		 */
		uint8_t FBBoostPixelFrequencyLimit   :1;  /* 0x100 */
		/* Discovered in AppleIntelFramebuffer::ValidateSourceSize.
		 * Limits source size to 4096x4096.
		 */
		uint8_t FBLimit4KSourceSize          :1;  /* 0x200 */
		/* Discovered in AppleIntelFramebufferController::start.
		 * These bits appear to be entirely equivalent and could be used interchangeably. Result in setting:
		 * - PCH_LP_PARTITION_LEVEL_DISABLE (1 << 12) bit in SOUTH_DSPCLK_GATE_D (0xc2020)
		 * - LPT_PWM_GRANULARITY (1 << 5) bit in SOUTH_CHICKEN2 (0xc2004)
		 * See Linux driver sources (lpt_init_clock_gating, lpt_enable_backlight).
		 * Since these bits are setting backlight pulse width modularity, there is no sense in setting them without a built-in display (i.e. on desktop).
		 */
		uint8_t FBAlternatePWMIncrement1     :1;  /* 0x400 */
		uint8_t FBAlternatePWMIncrement2     :1;  /* 0x800 */
		/* Discovered in Broadwell AppleIntelFBController::start / AppleIntelFBController::getFeatureControl.
		 * This is equivalent to setting DisableFeatureIPS=1 in the plist FeatureControl section.
		 * IPS stands for Intermediate Pixel Storage
		 */
		uint8_t FBDisableFeatureIPS          :1;  /* 0x1000 */
		uint8_t FBUnknownFlag_2000           :1;  /* 0x2000 */
		/* Discovered in Broadwell AppleIntelFBController::getOSInformation.
		 * Used by AppleIntelFramebufferController::LinkTraining for camellia version 2.
		 * Can be overridden by -notconrecover boot-arg, which effectively unsets this bit.
		 */
		uint8_t FBAllowConnectorRecover      :1;  /* 0x4000 */
		uint8_t FBUnknownFlag_8000           :1;  /* 0x8000 */
		uint8_t FBUnknownFlag_10000          :1;  /* 0x10000 */
		uint8_t FBUnknownFlag_20000          :1;  /* 0x20000 */
		/* Discovered in AppleIntelFramebufferController::getFeatureControl.
		 * This takes its effect only if GFMPPFM in the plist FeatureControl section is set to 2, otherwise GFMPPFM is off.
		 */
		uint8_t FBDisableGFMPPFM             :1;  /* 0x40000 */
		uint8_t FBUnknownFlag_80000          :1;  /* 0x80000 */
		uint8_t FBUnknownFlag_100000         :1;  /* 0x100000 */
		/* Discovered in AppleIntelFramebufferController::getFeatureControl.
		 * This takes its effect only if SupportDynamicCDClk in the plist FeatureControl section is set to 1, otherwise off.
		 * Also requires dc6config to be set to 3 (default).
		 */
		uint8_t FBEnableDynamicCDCLK         :1;  /* 0x200000 */
		uint8_t FBUnknownFlag_400000         :1;  /* 0x400000 */
		/* Discovered in AppleIntelFramebuffer::enableController.
		 * Used by AppleIntelFramebuffer::ValidateSourceSize.
		 * Setting this bit increases the maximum source size from 4096x4096 to 5120x5120.
		 * Most likely this enables 5K support via Intel HD.
		 */
		uint8_t FBSupport5KSourceSize        :1;  /* 0x800000 */
		uint8_t FBUknownZeroFlags;
	} bits;
	uint32_t value;
};

/* This is the same as ATI/AMD code.
 * At this moment only 2, 4, 0x400, and 0x800 are somewhat supported.
 * Interestingly connector type is not so important nowadays, e.g. VGA works fine on Kaby on DP.
 * As of SKL and newer ConnectorType is converted to fPortType by the following algo:
 * - connector with zero index (LVDS) gets fPortType 3.
 * - connector with ConnectorHDMI type gets fPortType 1.
 * - otherwise a connector has fPortType 2 (DisplayPort-like).
 */
enum ConnectorType : uint32_t {
	ConnectorZero       = 0x0,
	ConnectorDummy      = 0x1,   /* Always used as dummy, seems to sometimes work as VGA */
	ConnectorLVDS       = 0x2,   /* Just like on AMD LVDS is used for eDP */
	ConnectorDigitalDVI = 0x4,   /* This is not eDP despite a common misbelief */
	ConnectorSVID       = 0x8,
	ConnectorVGA        = 0x10,
	ConnectorDP         = 0x400,
	ConnectorHDMI       = 0x800,
	ConnectorAnalogDVI  = 0x2000
};

/* I can see very few mentioned in the code (0x1, 0x8, 0x40), though connectors themselves define way more! */

union ConnectorFlags {
	struct ConnectorFlagBits {
		/* Bits 1, 2, 8 are mentioned in AppleIntelFramebufferController::GetGPUCapability */
		/* Lets apperture memory to be not required AppleIntelFramebuffer::isApertureMemoryRequired */
		uint8_t CNAlterAppertureRequirements :1;  /* 0x1 */
		uint8_t CNUnknownFlag_2              :1;  /* 0x2 */
		uint8_t CNUnknownFlag_4              :1;  /* 0x4 */
		/* Normally set for LVDS displays (i.e. built-in displays) */
		uint8_t CNConnectorAlwaysConnected   :1;  /* 0x8 */
		/* AppleIntelFramebuffer::maxSupportedDepths checks this and returns 2 IODisplayModeInformation::maxDepthIndex ?? */
		uint8_t CNUnknownFlag_10             :1;  /* 0x10 */
		uint8_t CNUnknownFlag_20             :1;  /* 0x20 */
		/* Disable blit translation table? AppleIntelFramebufferController::ConfigureBufferTranslation */
		uint8_t CNDisableBlitTranslationTable:1;  /* 0x40 */
		/* Used in AppleIntelFramebufferController::setPowerWellState */
		/* Activates MISC IO power well (SKL_DISP_PW_MISC_IO) */
		/* May help with HDMI audio configuration issues */
		/* REF: https://github.com/acidanthera/bugtracker/issues/1189 */
		uint8_t CNUseMiscIoPowerWell         :1;  /* 0x80 */
		/* Used in AppleIntelFramebufferController::setPowerWellState */
		/* Activates Power Well 2 usage (SKL_PW_CTL_IDX_PW_2) */
		uint8_t CNUsePowerWell2              :1;  /* 0x100 */
		uint8_t CNUnknownFlag_200            :1;  /* 0x200 */
		uint8_t CNUnknownFlag_400            :1;  /* 0x400 */
		/* Sets fAvailableLaneCount to 30 instead of 20 when specified */
		uint8_t CNIncreaseLaneCount          :1;  /* 0x800 */
		uint8_t CNUnknownFlag_1000           :1;  /* 0x1000 */
		uint8_t CNUnknownFlag_2000           :1;  /* 0x2000 */
		uint8_t CNUnknownFlag_4000           :1;  /* 0x4000 */
		uint8_t CNUnknownFlag_8000           :1;  /* 0x8000 */
		uint16_t CNUnknownZeroFlags;
	} bits;
	uint32_t value;
};

struct PACKED ConnectorInfo {
	/* Watch out, this is really messy (see AppleIntelFramebufferController::MapFBToPort).
	 * I am not fully sure why this exists, and recommend setting index to array index (i.e. the sequential number from 0).
	 *
	 * The only accepted values are 0, 1, 2, 3, and -1 (0xFF). When index is equal to array index the logic is simple:
	 * Port with index    0    is always considered built-in (of LVDS type) regardless of any other values.
	 * Ports with indexes 1~3  are checked against type, HDMI will allow the use of digital audio, otherwise DP is assumed.
	 * Port with index    0xFF is ignored and skipped.
	 *
	 * When index != array index port type will be read from connector[index].type.
	 * Say, we have 2 active ports:
	 * 0 - [1]     busId 4 type LVDS
	 * 1 - [2]     busId 5 type DP
	 * 2 - [3]     busId 6 type HDMI
	 * 3 - [-1]    busId 0 type Dummy
	 * This will result in 2 framebuffers which types will be shifted:
	 * 0 - busId 4 type DP
	 * 1 - busId 5 type HDMI
	 * In fact BusId values are also read as connector[index].busId, but are later mapped back via
	 * AppleIntelFramebufferController::getGMBusIDfromPort by looking up a connector with the specified index.
	 * The lookup will stop as soon as a special marker connector (-1) is found. To illustrate, if we have 2 active ports:
	 * 0 - [1]     busId 4 type LVDS
	 * 1 - [2]     busId 5 type DP
	 * 2 - [-1]    busId 6 type HDMI
	 * 3 - [-1]    busId 0 type Dummy
	 * The result will be 2 framebuffers which types and the second busId will be shifted:
	 * 0 - busId 4 type DP
	 * 1 - busId 6 type HDMI
	 * It is also used for port-number calculation.
	 * - LVDS displays (more precisely, displays with CNConnectorAlwaysConnected flag set) get port-number 0.
	 * - Other displays go through index - port-number mapping: 1 - 5, 2 - 6, 3 - 7, or fallback to 0.
	 */
	int8_t index;
	/* Proven by AppleIntelFramebufferController::MapFBToPort, by a call to AppleIntelFramebufferController::getGMBusIDfromPort.
	 * This is GMBUS (Graphic Management Bus) ID described in https://01.org/sites/default/files/documentation/intel-gfx-prm-osrc-hsw-display_0.pdf.
	 * The use could be found in Intel Linux Graphics Driver source code:
	 * https://github.com/torvalds/linux/blob/6481d5ed076e69db83ca75e751ad492a6fb669a7/drivers/gpu/drm/i915/intel_i2c.c#L43
	 * https://github.com/torvalds/linux/blob/605dc7761d2701f73c17183649de0e3044609817/drivers/gpu/drm/i915/i915_reg.h#L3053
	 * However, it should be noted that Apple identifiers are slightly different from Linux driver.
	 * In Linux 0 means disabled, however, for Apple it has some special meaning and is used for internal display.
	 * Other than that the values are the same:
	 * - GMBUS_PIN_DPC    (4)  HDMIC
	 * - GMBUS_PIN_DPB    (5)  SDVO, HDMIB
	 * - GMBUS_PIN_DPD    (6)  HDMID
	 * - GMBUS_PIN_VGADDC (2)  VGA until Broadwell inclusive.
	 * So basically you could use 4, 5, 6 for arbitrary HDMI or DisplayPort displays.
	 * Since 5 supports SDVO (https://en.wikipedia.org/wiki/Serial_Digital_Video_Out), it may also be used to support DVI displays.
	 * Starting with Skylake VGA works via SDVO too (instead of a dedicated GMBUS_PIN_VGADDC id).
	 */
	uint8_t busId;
	/* Appears to be used for grouping ports just like Piker says, but I cannot find the usage. */
	uint8_t pipe;
	uint8_t pad;
	ConnectorType type;
	/* These are connector flags, they have nothing to do with delays regardless of what Piker says.
	 * I tried to describe some in ConnectorFlags.
	 */
	ConnectorFlags flags;
};

/* Wide connector type format */
struct PACKED ConnectorInfoICL {
	uint32_t index;
	uint32_t busId;
	uint32_t pipe;
	uint32_t pad;
	ConnectorType type;
	ConnectorFlags flags;
};

struct PACKED FramebufferSNB {
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount; /* also fNumFramebuffer */
	uint8_t  fFBMemoryCount;
	/* 0 means unused. */
	uint32_t fBacklightFrequency;
	uint32_t fBacklightMax;
	ConnectorInfo connectors[4];
};

struct PACKED FramebufferIVB {
	uint32_t framebufferId;
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount; /* also fNumFramebuffer */
	uint8_t  fFBMemoryCount;
	uint32_t fStolenMemorySize;
	uint32_t fFramebufferMemorySize;
	uint32_t fUnifiedMemorySize;
	uint32_t fBacklightFrequency;
	uint32_t fBacklightMax;
	uint32_t unk1[2];
	uint32_t unk2[2];
	uint32_t unk3;
	ConnectorInfo connectors[4];
	uint32_t pad2[26];
};

/* Some names are taken from 10.9 Azul driver. While they may not be the same names used in the struct, they are handy at least. */
struct PACKED FramebufferHSW {
	uint32_t framebufferId;
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount; /* also fNumFramebuffer */
	uint8_t  fFBMemoryCount;
	uint32_t fStolenMemorySize;
	uint32_t fFramebufferMemorySize;
	uint32_t fCursorMemorySize;
	uint32_t fUnifiedMemorySize;
	uint32_t fBacklightFrequency;
	uint32_t fBacklightMax;
	uint32_t pad[2];
	ConnectorInfo connectors[4];
	FramebufferFlags flags;
	uint8_t  unk1[3];
	uint8_t  camelliaVersion;
	uint32_t unk2;
	uint32_t fNumTransactionsThreshold;
	uint32_t fVideoTurboFreq;
	uint32_t unk3;
};

struct PACKED FramebufferBDW {
	uint32_t framebufferId;
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount;
	uint8_t  fFBMemoryCount;
	uint32_t fStolenMemorySize;
	uint32_t fFramebufferMemorySize;
	uint32_t fUnifiedMemorySize;
	uint32_t fBacklightFrequency;
	uint32_t fBacklightMax;
	uint32_t pad[3];
	ConnectorInfo connectors[4];
	FramebufferFlags flags;
	uint32_t unk1;
	uint32_t camelliaVersion;
	uint32_t unk2[6];
	uint32_t fNumTransactionsThreshold;
	uint32_t fVideoTurboFreq;
	uint32_t fRC6_Threshold;
};

struct PACKED FramebufferSKL {
	uint32_t framebufferId;
	uint32_t pad;
	uint64_t fModelNameAddr;
	/* While it is hard to be sure, because having 0 here results in online=true returned by
	 * AppleIntelFramebuffer::GetOnlineInfo, after all it appears to be the case, and the unused
	 * so-called mobile framebufers are simply set to fail-safe defaults.
	 * For some reason it is often called fDisabled...
	 */
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount;
	uint8_t  fFBMemoryCount;
	/* This one is per framebuffer fStolenMemorySize * fFBMemoryCount */
	uint32_t fStolenMemorySize;
	/* This is for boot framebuffer from what I can understand */
	uint32_t fFramebufferMemorySize;
	uint32_t fUnifiedMemorySize;
	uint32_t fBacklightFrequency;
	uint32_t fBacklightMax;
	uint32_t pad2[2];
	ConnectorInfo connectors[4];
	FramebufferFlags flags;
	/* Check DDI Buffer Translations in Linux driver for details. */
	uint8_t fBTTableOffsetIndexSlice; /* FBEnableSliceFeatures = 1 */
	uint8_t fBTTableOffsetIndexNormal; /* FBEnableSliceFeatures = 0 */
	uint8_t fBTTableOffsetIndexHDMI; /* fDisplayType = 1 */
	uint8_t pad3;
	uint32_t camelliaVersion;
	uint64_t unk3[3];
	uint32_t fNumTransactionsThreshold;
	/* Defaults to 14, used when UseVideoTurbo bit is set */
	uint32_t fVideoTurboFreq;
	uint32_t pad4;
	uint64_t fBTTArraySliceAddr;
	uint64_t fBTTArrayNormalAddr;
	uint64_t fBTTArrayHDMIAddr;
	uint32_t fSliceCount;
	uint32_t fEuCount;
	uint32_t unk6[2];
};

struct PACKED FramebufferCFL {
	uint32_t framebufferId;
	uint32_t pad;
	uint64_t fModelNameAddr;
	/* While it is hard to be sure, because having 0 here results in online=true returned by
	 * AppleIntelFramebuffer::GetOnlineInfo, after all it appears to be the case, and the unused
	 * so-called mobile framebufers are simply set to fail-safe defaults.
	 * For some reason it is often called fDisabled...
	 */
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount;
	uint8_t  fFBMemoryCount;
	/* This one is per framebuffer fStolenMemorySize * fFBMemoryCount */
	uint32_t fStolenMemorySize;
	/* This is for boot framebuffer from what I can understand */
	uint32_t fFramebufferMemorySize;
	uint32_t fUnifiedMemorySize;
	uint32_t pad2[2];
	ConnectorInfo connectors[4];
	FramebufferFlags flags;
	/* Check DDI Buffer Translations in Linux driver for details. */
	uint8_t fBTTableOffsetIndexSlice; /* FBEnableSliceFeatures = 1 */
	uint8_t fBTTableOffsetIndexNormal; /* FBEnableSliceFeatures = 0 */
	uint8_t fBTTableOffsetIndexHDMI; /* fDisplayType = 1 */
	uint8_t pad3;
	uint32_t camelliaVersion;
	uint64_t unk3[3];
	uint32_t fNumTransactionsThreshold;
	/* Defaults to 14, used when UseVideoTurbo bit is set */
	uint32_t fVideoTurboFreq;
	uint32_t pad4;
	uint64_t fBTTArraySliceAddr;
	uint64_t fBTTArrayNormalAddr;
	uint64_t fBTTArrayHDMIAddr;
	uint32_t fSliceCount;
	uint32_t fEuCount;
	uint32_t unk6[2];
};

/* Not sure what it is, in CNL value2 is a pointer, and value1 could be size.  */
struct PACKED FramebufferCNLCurrents {
	uint32_t value1;
	uint32_t pad;
	uint64_t valu2;
};

struct PACKED FramebufferCNL {
	uint32_t framebufferId;
	/* Unclear what values really are, yet 4 stands for non-LP chipset.
	 * See AppleIntelFramebufferController::start.
	 */
	uint32_t fPchType;
	uint64_t fModelNameAddr;
	/* While it is hard to be sure, because having 0 here results in online=true returned by
	 * AppleIntelFramebuffer::GetOnlineInfo, after all it appears to be the case, and the unused
	 * so-called mobile framebufers are simply set to fail-safe defaults.
	 * For some reason it is often called fDisabled...
	 */
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount;
	uint8_t  fFBMemoryCount;
	/* This one is per framebuffer fStolenMemorySize * fFBMemoryCount */
	uint32_t fStolenMemorySize;
	/* This is for boot framebuffer from what I can understand */
	uint32_t fFramebufferMemorySize;
	uint32_t fUnifiedMemorySize;
	uint32_t pad1[2];
	ConnectorInfo connectors[4];
	FramebufferFlags flags;
	/* Check DDI Buffer Translations in Linux driver for details. */
	uint8_t fBTTableOffsetIndexSlice; /* FBEnableSliceFeatures = 1 */
	uint8_t fBTTableOffsetIndexNormal; /* FBEnableSliceFeatures = 0 */
	uint8_t fBTTableOffsetIndexHDMI; /* fDisplayType = 1 */
	uint8_t pad2;
	uint64_t unk1[5];
	uint64_t fBTTArraySliceAddr;
	uint64_t fBTTArrayNormalAddr;
	uint64_t fBTTArrayHDMIAddr;
	FramebufferCNLCurrents currents[8];
	uint32_t camelliaVersion;
	uint64_t unk2[3];
	uint32_t fNumTransactionsThreshold;
	/* Defaults to 14, used when UseVideoTurbo bit is set */
	uint32_t fVideoTurboFreq;
	uint32_t fSliceCount;
	uint32_t fEuCount;
	uint32_t unk4;
	uint32_t unk5[2];
};

struct PACKED FramebufferICLLP {
	uint32_t framebufferId;
	/* Unclear what values really are, yet 4 stands for non-LP chipset.
	 * See AppleIntelFramebufferController::start.
	 */
	uint32_t fPchType;
	uint64_t fModelNameAddr;
	/* While it is hard to be sure, because having 0 here results in online=true returned by
	 * AppleIntelFramebuffer::GetOnlineInfo, after all it appears to be the case, and the unused
	 * so-called mobile framebufers are simply set to fail-safe defaults.
	 * For some reason it is often called fDisabled...
	 */
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount;
	uint8_t  fFBMemoryCount;
	/* This one is per framebuffer fStolenMemorySize * fFBMemoryCount */
	uint32_t fStolenMemorySize;
	/* This is for boot framebuffer from what I can understand */
	uint32_t fFramebufferMemorySize;
	uint32_t fUnifiedMemorySize;
	ConnectorInfoICL connectors[6];
	/* Flags are quite different in ICL now */
	union { uint32_t value; } flags;
	uint32_t unk2;
	FramebufferCNLCurrents currents[3];
	uint32_t unk3[2];
	uint32_t camelliaVersion;
	uint32_t unk4[3];
	uint32_t fNumTransactionsThreshold;
	/* Defaults to 14, used when UseVideoTurbo bit is set */
	uint32_t fVideoTurboFreq;
	uint32_t fSliceCount;
	uint32_t fEuCount;
	uint32_t unk5;
	uint8_t unk6;
	uint8_t pad[3];
};

struct PACKED FramebufferICLHP {
	uint32_t framebufferId;
	/* Unclear what values really are, yet 4 stands for non-LP chipset.
	 * See AppleIntelFramebufferController::start.
	 */
	uint32_t fPchType;
	uint64_t fModelNameAddr;
	/* While it is hard to be sure, because having 0 here results in online=true returned by
	 * AppleIntelFramebuffer::GetOnlineInfo, after all it appears to be the case, and the unused
	 * so-called mobile framebufers are simply set to fail-safe defaults.
	 * For some reason it is often called fDisabled...
	 */
	uint8_t  fMobile;
	uint8_t  fPipeCount;
	uint8_t  fPortCount;
	uint8_t  fFBMemoryCount;
	/* This one is per framebuffer fStolenMemorySize * fFBMemoryCount */
	uint32_t fStolenMemorySize;
	/* This is for boot framebuffer from what I can understand */
	uint32_t fFramebufferMemorySize;
	uint32_t fUnifiedMemorySize;
	uint32_t fBacklightFrequency;
	uint32_t fBacklightMax;
	uint32_t pad1[2];
	ConnectorInfoICL connectors[3];
	FramebufferFlags flags;
	FramebufferCNLCurrents currents[8];
	uint32_t unk2[5];
	uint32_t camelliaVersion;
	uint32_t unk3[6];
	/* Defaults to 14, used when UseVideoTurbo bit is set */
	uint32_t fNumTransactionsThreshold;
	uint32_t fVideoTurboFreq;
	uint32_t fSliceCount;
	uint32_t fEuCount;
	uint32_t unk4;
};

#endif /* kern_fb_hpp */
