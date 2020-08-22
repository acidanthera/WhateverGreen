//
//  kern_agdc.hpp
//  WhateverGreen
//
//  Copyright © 2020 vit9696. All rights reserved.
//

#ifndef kern_agdc_h
#define kern_agdc_h

// Some knowledge here comes from AppleGraphicsDeviceControlUserCommand.h from 10.9 SDK.

#define APPLE_GRAPHICS_DEVICE_CONTOL_VERSION 0x205
#define APPLE_GRAPHICS_DEVICE_CONTROL_SERVICE "AppleGraphicsDeviceControl"

#define ATTRIBUTE_ONLINE        '!off'
#define ATTRIBUTE_SETMODE       'mset'
#define ATTRIBUTE_SETEDID       'edid'
#define ATTRIBUTE_DOCONTROL     'dctl'

#define kAGDCVendorCmdFlag      (1 << 31)

#define AGDC_EDID_BLOCKSIZE 128

/* You should use these macros to pack and unpack the
 * detailed timing structs.. UserCode should only be using
 * AGDCDetailedTimingInformation_t
 */
#define _AGDC_FIELD_COPY(src, dest, x)      dest->x = src->x
#define  _AGDC_CONVERT_(_src, tsrc, _dest, tdest)             \
   do {                                                       \
    tsrc *src = _src;                                         \
    tdest *dest = _dest;                                      \
    dest = dest;                                              \
    src = src;                                                \
    _AGDC_FIELD_COPY(src, dest, horizontalScaledInset);       \
    _AGDC_FIELD_COPY(src, dest, verticalScaledInset);         \
    _AGDC_FIELD_COPY(src, dest, scalerFlags);                 \
    _AGDC_FIELD_COPY(src, dest, horizontalScaled);            \
    _AGDC_FIELD_COPY(src, dest, verticalScaled);              \
    _AGDC_FIELD_COPY(src, dest, signalConfig);                \
    _AGDC_FIELD_COPY(src, dest, signalLevels);                \
    _AGDC_FIELD_COPY(src, dest, pixelClock);                  \
    _AGDC_FIELD_COPY(src, dest, minPixelClock);               \
    _AGDC_FIELD_COPY(src, dest, maxPixelClock);               \
    _AGDC_FIELD_COPY(src, dest, horizontalActive);            \
    _AGDC_FIELD_COPY(src, dest, horizontalBlanking);          \
    _AGDC_FIELD_COPY(src, dest, horizontalSyncOffset);        \
    _AGDC_FIELD_COPY(src, dest, horizontalSyncPulseWidth);    \
    _AGDC_FIELD_COPY(src, dest, verticalActive);              \
    _AGDC_FIELD_COPY(src, dest, verticalBlanking);            \
    _AGDC_FIELD_COPY(src, dest, verticalSyncOffset);          \
    _AGDC_FIELD_COPY(src, dest, verticalSyncPulseWidth);      \
    _AGDC_FIELD_COPY(src, dest, horizontalBorderLeft);        \
    _AGDC_FIELD_COPY(src, dest, horizontalBorderRight);       \
    _AGDC_FIELD_COPY(src, dest, verticalBorderTop);           \
    _AGDC_FIELD_COPY(src, dest, verticalBorderBottom);        \
    _AGDC_FIELD_COPY(src, dest, horizontalSyncConfig);        \
    _AGDC_FIELD_COPY(src, dest, horizontalSyncLevel);         \
    _AGDC_FIELD_COPY(src, dest, verticalSyncConfig);          \
    _AGDC_FIELD_COPY(src, dest, verticalSyncLevel);           \
    _AGDC_FIELD_COPY(src, dest, numLinks);                    \
    _AGDC_FIELD_COPY(src, dest, verticalBlankingExtension);   \
    _AGDC_FIELD_COPY(src, dest, pixelEncoding);               \
    _AGDC_FIELD_COPY(src, dest, bitsPerColorComponent);       \
    _AGDC_FIELD_COPY(src, dest, colorimetry);                 \
    _AGDC_FIELD_COPY(src, dest, dynamicRange);                \
    _AGDC_FIELD_COPY(src, dest, dscCompressedBitsPerPixel);   \
    _AGDC_FIELD_COPY(src, dest, dscSliceHeight);              \
    _AGDC_FIELD_COPY(src, dest, dscSliceWidth);               \
   } while (0)
#define CONVERT_AGDC_TO_OS_TIMING_DATA(src, dest)       _AGDC_CONVERT_(src, AGDCDetailedTimingInformation_t, dest, IODetailedTimingInformationV2)
#define CONVERT_OS_TO_AGDC_TIMING_DATA(src, dest)       _AGDC_CONVERT_(src, IODetailedTimingInformationV2, dest, AGDCDetailedTimingInformation_t)


#pragma pack(push, 1)

// Additional notes for AppleGraphicsDevicePolicy.kext:
// agdp=-1 -> force unload AppleGraphicsDevicePolicy.kext
// agdp=0  -> do nothing basically
// agdp>0  -> set extra FeatureControl bits:
//  0x1     -> timeout bit
//  0x2     -> skip no media enter
//  0x4     -> special link modes
//  0x10    -> some aux command
//  0x40    -> something with timing

enum kAGDCCommand_t {
    kAGDCVendorInfo                            = 0x1,                  // may be called without a WL
    kAGDCVendorEnableController                = 0x2,                  // may be called without a WL

    // 0x100 - 0x1ff PM = Power Management
    kAGDCPMInfo                                = 0x100,
    kAGDCPMStateCeiling                        = 0x101,
    kAGDCPMStateFloor                          = 0x102,
    kAGDCPMPState                              = 0x103,
    kAGDCPMPowerLimit                          = 0x104,
    kAGDCPMGetGPUInfo                          = 0x120,
    kAGDCPMGetPStateFreqTable                  = 0x121,
    kAGDCPMGetPStateResidency                  = 0x122,
    kAGDCPMGetCStateNames                      = 0x123,
    kAGDCPMGetCStateResidency                  = 0x124,
    kAGDCPMGetMiscCntrNum                      = 0x125,
    kAGDCPMGetMiscCntrInfo                     = 0x126,
    kAGDCPMGetMiscCntr                         = 0x127,
    kAGDCPMTakeCPStateResidencySnapshot        = 0x128,
    kAGDCPMGetPStateResidencyDiff              = 0x129,
    kAGDCPMGetCStateResidencyDiff              = 0x12A,
    kAGDCPMGetPStateResidencyDiffAbs           = 0x12B,

    // 0x600 – 0x6ff                                    Reserved

    // 0x700
    kAGDCFBPerFramebufferCMD                   = 0x700,
    kAGDCFBOnline                              = 0x701,
    kAGDCFBSetEDID                             = 0x702,
    kAGDCFBSetMode                             = 0x703,
    kAGDCFBInjectEvent                         = 0x704,
    kAGDCFBDoControl                           = 0x705,
    kAGDCFBDPLinkConfig                        = 0x706,
    kAGDCFBSetEDIDEx                           = 0x707,
    kAGDCFBGetCapability                       = 0x710,
    kAGDCFBGetCapabilityEx                     = 0x711,

    // 0x800 – 0x8ff Reserved for common debug!!!
    kAGDCMultiLinkConfig                       = 0x920,
    kAGDCLinkConfig                            = 0x921,
    /* 0x922 TODO */
    kAGDCRegisterCallback                      = 0x923,
    /* 0x924 TODO */
    kAGDCGetPortStatus                         = 0x925,
    kAGDCConfigureAudio                        = 0x926,
    kAGDCCallbackCapability                    = 0x927,
    kAGDCStreamSleepControl                    = 0x928,
    kAGDCPortEnable                            = 0x940,
    kAGDCPortCapability                        = 0x941,

    kAGDCDiagnoseGetDevicePropertySize         = 0x975,
    kAGDCDiagnoseGetDeviceProperties           = 0x976,

    kAGDCGPUCapability                         = 0x980,                 // may be called without a WL
    kAGDCStreamAssociate                       = 0x981,
    kAGDCStreamRequest                         = 0x982,
    kAGDCStreamAccessI2C                       = 0x983,
    kAGDCStreamAccessI2CCapability             = 0x984,
    kAGDCStreamAccessAUX                       = 0x985,
    kAGDCStreamGetEDID                         = 0x986,
    kAGDCStreamSetState                        = 0x987,
    kAGDCStreamConfig                          = 0x988,
    kAGDCEnableController                      = 0x989,

    kAGDCTrainingBegin                         = 0x1300,
    kAGDCTrainingAttempt                       = 0x1301,
    kAGDCTrainingEnd                           = 0x1302,

    kAGDCTestConfiguration                     = 0x1800,
    kAGDCCommitConfiguration                   = 0x1801,
    kAGDCReleaseConfiguration                  = 0x1802,

    kAGDCPluginMetricsPlug                     = 0x4100,
    kAGDCPluginMetricsUnPlug                   = 0x4101,
    kAGDCPluginMetricsHPD                      = 0x4102,
    kAGDCPluginMetricsSPI                      = 0x4103,
    kAGDCPluginMetricsSyncLT                   = 0x4104,
    kAGDCPluginMetricsSyncLTEnd                = 0x4105,
    kAGDCPluginMetricsLTBegin                  = 0x4106,
    kAGDCPluginMetricsLTEnd                    = 0x4107,
    kAGDCPluginMetricsDisplayInfo              = 0x4108,
    kAGDCPluginMetricsMonitorInfo              = 0x4109,
    kAGDCPluginMetricsLightUpDp                = 0x410A,
    kAGDCPluginMetricsHDCPStart                = 0x410B,
    kAGDCPluginMetricsFirstPhaseComplete       = 0x410C,
    kAGDCPluginMetricsLocalityCheck            = 0x410D,
    kAGDCPluginMetricsRepeaterAuthenticatio    = 0x410E,
    kAGDCPluginMetricsHDCPEncryption           = 0x410F,
    kAGDCPluginMetricsHPDSinktoTB              = 0x4110,
    kAGDCPluginMetricsHPDTBtoGPU               = 0x4111,
    kAGDCPluginMetricsVersion                  = 0x4112,
    kAGDCPluginMetricsGetMetricInfo            = 0x4113,
    kAGDCPluginMetricsGetMetricData            = 0x4114,
    kAGDCPluginMetricsMarker                   = 0x4116,
    kAGDCPluginMetricsGetMessageTracer         = 0x4117,
    kAGDCPluginMetricsXgDiscovery              = 0x4119,
    kAGDCPluginMetricsXgDriversStart           = 0x411A,
    kAGDCPluginMetricsXgPublished              = 0x411B,
    kAGDCPluginMetricsXgResetPort              = 0x411D,
    kAGDCPluginMetricsPowerOff                 = 0x411E,
    kAGDCPluginMetricsPowerOn                  = 0x411F,
    kAGDCPluginMetricsSPIData                  = 0x4120,
    kAGDCPluginMetricsEFIData                  = 0x4121,
};

enum kAGDCVendorClass_t {
    kAGDCVendorClassReserved             = 0,
    kAGDCVendorClassIntegratedGPU        = 1,
    kAGDCVendorClassDiscreteGPU          = 2,
    kAGDCVendorClassOtherHW              = 3,
    kAGDCVendorClassOtherSW              = 4,
    kAGDCVendorClassAppleGPUPolicyManager = 5,              // AGDCPolicy
    kAGDCVendorClassAppleGPUPowerManager = 6,               // AGPM
};

struct AGDCVendorInfo_t {
    uint32_t                        Version;
    char                            VendorString[32];
    uint32_t                        VendorID;
    kAGDCVendorClass_t              VendorClass;
};

struct AGDCVendorControllerEnable_t {
    uint32_t                        enabled;
};

struct AGDCStreamAddress_t {
    uint32_t                        port;                   // Port Number 1->n
    uint32_t                        stream;                 // 0 or MST address
};

struct AGDCGPUCapability_t {
    struct {
        uint64_t        portMap;
        uint64_t        MSTPortMap;
        uint64_t        ddcTransportMap;
        uint64_t        auxTransportMap;
        struct {
            uint32_t    DVI;
            uint32_t    DP;
            uint32_t    MST;
            uint32_t    maximum;
        }               numberOfStreams;
        uint32_t        numberOfFramebuffers;

        uint64_t        _reserved[9];                   // kernel is 64 bit, user is 32/64.. pointers are not safe!!!

        uint64_t        _reserved_a;
        uint64_t        _reserved_b;
        uint64_t        _reserved_c;
        uint64_t        _reserved_d;
    } gpu;
};

struct AGDCStreamAssociate_t {
    int32_t                 id;
    AGDCStreamAddress_t     address;                    // for GET this is an output, otherwise both are inputs to a driver
};

struct AGDCStreamRequest_t {
    struct {
        AGDCStreamAddress_t address;                    // Input, a hint which ports are in the set we want to bind
    }                       link[4];
    int32_t                 id;                         // Ouput FB
    int32_t                 groupID;                    // Output preferred groupID (if any) -1 otherwise.
};

struct AGDCStreamAccessI2C_t {
    AGDCStreamAddress_t     address;
    struct {
        uint16_t            device;
        uint16_t            speed;
        struct {
            uint16_t        length;
            uint8_t         buffer[256];
        }                   write;
        struct {
            uint16_t        length;
            uint8_t         buffer[256];
        }                   read;
        uint16_t            status;
    }                       i2c;
};

struct AGDCStreamAccessI2CCapability_t {
    AGDCStreamAddress_t     address;
    struct {
        uint16_t            width;
        uint16_t            minSpeed;
        uint16_t            maxSpeed;
        uint16_t            burstSize;
    }                       i2c;
};

struct AGDCStreamAccessAUX_t {
    AGDCStreamAddress_t     address;
    uint32_t                AuxRegister;
    struct {
        uint8_t             size;
        union {
            uint8_t         b[16];
            uint16_t        s[8];
            uint32_t        l[4];
        }                   data;
        uint16_t            status;
    }                       aux;
} ;


struct AGDCStreamGetEDID_t {
    AGDCStreamAddress_t address;
    uint32_t        block;
    uint8_t         data[128];      // 128
    uint32_t        status;
};


struct AGDCDetailedTimingInformation_t {
    uint32_t      horizontalScaledInset;          // pixels
    uint32_t      verticalScaledInset;            // lines

    uint32_t      scalerFlags;
    uint32_t      horizontalScaled;
    uint32_t      verticalScaled;

    uint32_t      signalConfig;
    uint32_t      signalLevels;

    uint64_t      pixelClock;                     // Hz

    uint64_t      minPixelClock;                  // Hz - With error what is slowest actual clock
    uint64_t      maxPixelClock;                  // Hz - With error what is fasted actual clock

    uint32_t      horizontalActive;               // pixels
    uint32_t      horizontalBlanking;             // pixels
    uint32_t      horizontalSyncOffset;           // pixels
    uint32_t      horizontalSyncPulseWidth;       // pixels

    uint32_t      verticalActive;                 // lines
    uint32_t      verticalBlanking;               // lines
    uint32_t      verticalSyncOffset;             // lines
    uint32_t      verticalSyncPulseWidth;         // lines

    uint32_t      horizontalBorderLeft;           // pixels
    uint32_t      horizontalBorderRight;          // pixels
    uint32_t      verticalBorderTop;              // lines
    uint32_t      verticalBorderBottom;           // lines

    uint32_t      horizontalSyncConfig;
    uint32_t      horizontalSyncLevel;            // Future use (init to 0)
    uint32_t      verticalSyncConfig;
    uint32_t      verticalSyncLevel;              // Future use (init to 0)
    uint32_t      numLinks;
    uint32_t      verticalBlankingExtension;
    uint16_t      pixelEncoding;
    uint16_t      bitsPerColorComponent;
    uint16_t      colorimetry;
    uint16_t      dynamicRange;
    uint16_t      dscCompressedBitsPerPixel;
    uint16_t      dscSliceHeight;
    uint16_t      dscSliceWidth;
};

struct AGDCStreamConfig_t {
    int32_t                         id;
    AGDCStreamAddress_t             address;
    AGDCDetailedTimingInformation_t timing;
    uint32_t                        horizontalOffset;
    uint32_t                        verticalOffset;
};

struct AGDCMultiLinkConfig_t {
    AGDCStreamConfig_t                  link[4];
    struct {
        int32_t                         id;
        int32_t                         groupID;
        AGDCStreamAddress_t             address;
        AGDCDetailedTimingInformation_t timing;
        uint32_t                        singleTiming;
    }                                   target;
    uint32_t                            valid;
};

enum AGDCLinkConfigFlags_t {
    kAGDCLinkConfigFlag_Online          = (1 << 0),
    kAGDCLinkConfigFlag_MayGroup        = (1 << 1),
    kAGDCLinkConfigFlag_IsFixed         = (1 << 2),
    kAGDCLinkConfigFlag_IsAssociated    = (1 << 3),
    kAGDCLinkConfigFlag_IsGrouped       = (1 << 4),
};

enum AGDCStreamState_t {
    kAGDCStreamStateRelease     = 0,
    kAGDCStreamStateEnabled     = 1,
    kAGDCStreamStateInvalid     = 0x80000000,
};

struct AGDCLinkConfig_t {
    int32_t                         id;         // input framebuffer field
    int32_t                         groupID;    // output
    uint64_t                        flags;      // output, AGDCLinkConfigFlags_t
    AGDCDetailedTimingInformation_t timing;     // output, DetailedTiming
    AGDCStreamAddress_t             address;    // output, encoded PortNumber
    AGDCStreamState_t               state;      // output
};

typedef void * AGDCLinkAvailable_t __deprecated;

enum kAGDCRegisterLinkControlEvent_t {
    kAGDCRegisterLinkInsert             = 0,
    kAGDCRegisterLinkRemove             = 1,
    kAGDCRegisterLinkChange             = 2,
    kAGDCRegisterLinkChangeMST          = 3,
    kAGDCRegisterLinkFramebuffer        = 4,                                // V106+
    // This event is handled by AppleGraphicsDevicePolicy::VendorEventHandler.
    // The point of this event to validate the timing information, and take decision on what to do with this display.
    // One of the examples of this event is to merge multiple display ports into one connection for 5K/6K output.
    // Drivers should interpret modeStatus to decide on whether to continue execution, change the mode, or disable link.
    kAGDCValidateDetailedTiming         = 10,
    kAGDCRegisterLinkChangeWakeProbe    = 0x80,
};

struct __deprecated AGDCGetLinkMaster_t;
typedef AGDCGetLinkMaster_t AGDCRegisterLinkMaster_t __deprecated;

struct AGDCStreamSetState_t {
    AGDCStreamAddress_t     address;
    AGDCStreamState_t       state;
};

struct AGDCFBOnline_t {
    int32_t                 id;
    uint32_t                state;
};

typedef AGDCFBOnline_t kAppleAFBOnline_t   __deprecated;

struct AGDCFBSetMode_t {
    int32_t                 id;
    uint32_t                modeID;
    uint32_t                width;
    uint32_t                height;
    uint32_t                depth;
    uint32_t                reserved[7];
};

typedef AGDCFBSetMode_t    kAppleAFBSetMode_t __deprecated;

struct AGDCEDID_t {
    int32_t                 id;
    uint32_t                size;
    uint8_t                 data[2*128];
};

typedef AGDCEDID_t       kAppleAFBEdid_t __deprecated;

struct AGDCFBInjectEvent_t {
    int32_t                 id;
    uint32_t                eventID;
};

typedef AGDCFBInjectEvent_t    kAppleAFBinjectEvent_t __deprecated;

struct AGDCFBDoControl_t {
    int32_t                 id;
    uint64_t                payload[4];
};

typedef AGDCFBDoControl_t kAppleAFBdoControl_t __deprecated;

struct AGDCFBCommandPacket_t {
    union {
        AGDCFBOnline_t          online;
        AGDCFBSetMode_t         mode;
        AGDCEDID_t              edid;
        AGDCFBInjectEvent_t     event;
        AGDCFBDoControl_t       control;
        AGDCStreamSetState_t    streamset;
    };
} __deprecated;

typedef AGDCFBCommandPacket_t kAppleAFBCommandPacket_t __deprecated;

struct AGDCPMInfo_t {
    uint32_t                    NumPStates;
    uint32_t                    NumPowerLimits;
    uint32_t                    ControlType;
};

enum kAGDCPMCallbackEvent_t {
    kAGDCPMUpInterruptHint      = 0,
    kAGDCPMDownInterruptHint    = 1,
};

struct kAGDCPMCallbackInfo_t {
    uint32_t                    version;
    struct {
        uint32_t                currentPState;
        uint32_t                suggestedPState;
        uint32_t                coreBusyness;
        uint32_t                memoryBusyness;
        uint32_t                mclkFloor;
    }                           gpu;
};

struct AGDCPMRegisterCallback_t {
    void *cookie;
    int (*handler)(void *obj,
                   kAGDCPMCallbackEvent_t event,
                   kAGDCPMCallbackInfo_t *data);
};

struct AGDCValidateDetailedTiming_t {
    uint32_t                         framebufferIndex;     // IOFBDependentIndex
    AGDCDetailedTimingInformation_t  timing;
    uint16_t                         padding1[5];
    void                             *cfgInfo;             // AppleGraphicsDevicePolicy configuration
    int32_t                          frequency;
    uint16_t                         padding2[6];
    uint32_t                         modeStatus;           // 1 - invalid, 2 - success, 3 - change timing
    uint16_t                         padding3[2];
};

#pragma pack(pop)

#endif /* kern_agdc_h */
