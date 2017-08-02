//
//  reference.c
//  WhateverGreen
//  Explanation of how RadeonFramebuffer connectors are created
//
//  Copyright © 2017 vit9696. All rights reserved.
//
//  Reverse-engineered from AMDSupport.kext
//  Copyright © 2017 Apple Inc. All rights reserved.
//

/**
 *  Connectors from AMDSupport since 10.12
 */
struct ModernConnector {
  uint32_t type;
  uint32_t flags;
  uint16_t features;
  uint16_t priority;
  uint32_t reserved1;
  uint8_t transmitter;
  uint8_t encoder;
  uint8_t hotplug;
  uint8_t sense;
  uint32_t reserved2;
};

enum ConnectorType {
  ConnectorLVDS       = 0x2,
  ConnectorDigitalDVI = 0x4,
  ConnectorSVID       = 0x8,
  ConnectorVGA        = 0x10,
  ConnectorDP         = 0x400,
  ConnectorHDMI       = 0x800,
  ConnectorAnalogDVI  = 0x2000
};

/**
 *  Internal atom connector struct since 10.13
 */
struct AtomConnectorInfo {
  uint16_t *atomObject;
  uint16_t usConnObjectId;
  uint16_t usGraphicObjIds;
  uint8_t *hpdRecord;
  uint8_t *i2cRecord;
};

// Definitions taken from asic_reg/ObjectID.h
enum {
  /* External Third Party Encoders */
  ENCODER_OBJECT_ID_SI170B                  = 0x08,
  ENCODER_OBJECT_ID_CH7303                  = 0x09,
  ENCODER_OBJECT_ID_CH7301                  = 0x0A,
  ENCODER_OBJECT_ID_INTERNAL_DVO1           = 0x0B,    /* This belongs to Radeon Class Display Hardware */
  ENCODER_OBJECT_ID_EXTERNAL_SDVOA          = 0x0C,
  ENCODER_OBJECT_ID_EXTERNAL_SDVOB          = 0x0D,
  ENCODER_OBJECT_ID_TITFP513                = 0x0E,
  ENCODER_OBJECT_ID_INTERNAL_LVTM1          = 0x0F,    /* not used for Radeon */
  ENCODER_OBJECT_ID_VT1623                  = 0x10,
  ENCODER_OBJECT_ID_HDMI_SI1930             = 0x11,
  ENCODER_OBJECT_ID_HDMI_INTERNAL           = 0x12,
  /* Kaleidoscope (KLDSCP) Class Display Hardware (internal) */
  ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1   = 0x13,
  ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1    = 0x14,
  ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1    = 0x15,
  ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2    = 0x16,  /* Shared with CV/TV and CRT */
  ENCODER_OBJECT_ID_SI178                   = 0X17,  /* External TMDS (dual link, no HDCP.) */
  ENCODER_OBJECT_ID_MVPU_FPGA               = 0x18,  /* MVPU FPGA chip */
  ENCODER_OBJECT_ID_INTERNAL_DDI            = 0x19,
  ENCODER_OBJECT_ID_VT1625                  = 0x1A,
  ENCODER_OBJECT_ID_HDMI_SI1932             = 0x1B,
  ENCODER_OBJECT_ID_DP_AN9801               = 0x1C,
  ENCODER_OBJECT_ID_DP_DP501                = 0x1D,
  ENCODER_OBJECT_ID_INTERNAL_UNIPHY         = 0x1E,
  ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA   = 0x1F,
  ENCODER_OBJECT_ID_INTERNAL_UNIPHY1        = 0x20,
  ENCODER_OBJECT_ID_INTERNAL_UNIPHY2        = 0x21,
  ENCODER_OBJECT_ID_ALMOND                  = 0x22,
  ENCODER_OBJECT_ID_NUTMEG                  = 0x22,
  ENCODER_OBJECT_ID_TRAVIS                  = 0x23,
  ENCODER_OBJECT_ID_INTERNAL_VCE            = 0x24,
  ENCODER_OBJECT_ID_INTERNAL_UNIPHY3        = 0x25,
  ENCODER_OBJECT_ID_INTERNAL_AMCLK          = 0x27
};

enum {
  CONNECTOR_OBJECT_ID_NONE                = 0x00,
  CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I   = 0x01,
  CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I     = 0x02,
  CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D   = 0x03,
  CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D     = 0x04,
  CONNECTOR_OBJECT_ID_VGA                 = 0x05,
  CONNECTOR_OBJECT_ID_COMPOSITE           = 0x06,
  CONNECTOR_OBJECT_ID_SVIDEO              = 0x07,
  CONNECTOR_OBJECT_ID_YPbPr               = 0x08,
  CONNECTOR_OBJECT_ID_D_CONNECTOR         = 0x09,
  CONNECTOR_OBJECT_ID_9PIN_DIN            = 0x0A,  /* Supports both CV & TV */
  CONNECTOR_OBJECT_ID_SCART               = 0x0B,
  CONNECTOR_OBJECT_ID_HDMI_TYPE_A         = 0x0C,
  CONNECTOR_OBJECT_ID_HDMI_TYPE_B         = 0x0D,
  CONNECTOR_OBJECT_ID_LVDS                = 0x0E,
  CONNECTOR_OBJECT_ID_7PIN_DIN            = 0x0F,
  CONNECTOR_OBJECT_ID_PCIE_CONNECTOR      = 0x10,
  CONNECTOR_OBJECT_ID_CROSSFIRE           = 0x11,
  CONNECTOR_OBJECT_ID_HARDCODE_DVI        = 0x12,
  CONNECTOR_OBJECT_ID_DISPLAYPORT         = 0x13,
  CONNECTOR_OBJECT_ID_eDP                 = 0x14,
  CONNECTOR_OBJECT_ID_MXM                 = 0x15,
  CONNECTOR_OBJECT_ID_LVDS_eDP            = 0x16
};

// Initially applecon data is nulled.
IOReturn AtiBiosParser2::translateAtomConnectorInfo(AtiBiosParser2 *parser, AtomConnectorInfo *atomcon, ModernConnector *applecon) {
  if (((atomcon->usConnObjectId & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT) != GRAPH_OBJECT_TYPE_CONNECTOR)
    return kIOReturnNotFound;

  uint8_t *i2cRecord = atomcon->i2cRecord;
  if (i2cRecord) {
    applecon->sense = (i2cRecord[2] & 0xF) + 1;
  } else {
    applecon->sense = 0;
    kprintf("ATOM: %s: ASSERT(NULL != i2cRecord)\n", "uint8_t AtiBiosParser2::getAuxDdcLine(atom_i2c_record *)");
  }

  uint8_t *hpdRecord = (uint8_t *)atomcon->hpdRecord;
  if (hpdRecord) {
    applecon->hotplug = hpdRecord[2];
  } else {
    applecon->hotplug = 0;
    kprintf("ATOM: %s: ASSERT(NULL != hpdRecord)\n", "uint8_t AtiBiosParser2::getHotPlugPin(atom_hpd_int_record *)");
  }

  AtiBiosParser2::getOutputInformation(parser, atomcon, applecon);
  AtiBiosParser2::getConnectorFeatures(parser, atomcon, applecon);

  if (applecon->flags) {
    if ((applecon->flags & 0x704) && applecon->hotplug)
      applecon->features |= 0x100;
    return kIOReturnSuccess;
  } else {
    kprintf("ATOM: %s: ASSERT(0 != drvInfo.connections)\n", "IOReturn AtiBiosParser2::translateAtomConnectorInfo(AtiObjectInfoTableInterface_V2::AtomConnectorInfo &, ConnectorInfo &)");
  }

  return kIOReturnNotFound;
}

IOReturn AtiBiosParser2::getOutputInformation(AtiBiosParser2 *parser, AtomConnectorInfo *atomcon, ModernConnector *applecon) {
  if (!atomcon->usGraphicObjIds) {
    kprintf("ATOM: %s: ASSERT(0 != objId.u16All)\n", "IOReturn AtiBiosParser2::getOutputInformation(AtiObjectInfoTableInterface_V2::AtomConnectorInfo &, ConnectorInfo &)");
    return kIOReturnBadArgument;
  }

  uint8_t encoder = (uint8_t)atomcon->usGraphicObjIds;
  bool one = ((atomcon->usGraphicObjIds & ENUM_ID_MASK) >> ENUM_ID_SHIFT) == 1;

  switch (encoder) {
    case ENCODER_OBJECT_ID_NUTMEG:
      applecon->flags |= 0x10;
      return kIOReturnSuccess;
    case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
      if (one) {
        applecon->transmitter |= 0x10;
      } else {
        applecon->transmitter |= 0x20;
        applecon->encoder |= 0x1;
      }
      return kIOReturnSuccess;
    case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
      if (one) {
        applecon->transmitter |= 0x11;
        applecon->encoder |= 0x2;
      } else {
        applecon->transmitter |= 0x21;
        applecon->encoder |= 0x3;
      }
      return kIOReturnSuccess;
    case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
      if (one) {
        applecon->transmitter |= 0x12;
        applecon->encoder |= 0x4;
      } else {
        applecon->transmitter |= 0x22;
        applecon->encoder |= 0x5;
      }
      return kIOReturnSuccess;
    case ENCODER_OBJECT_ID_INTERNAL_UNIPHY3:
      if (one) {
        applecon->transmitter |= 0x13;
        applecon->encoder |= 0x6;
      } else {
        applecon->transmitter |= 0x23;
        applecon->encoder |= 0x7;
      }
      return kIOReturnSuccess;
  }

  return kIOReturnInvalid;
}

IOReturn AtiBiosParser2::getConnectorFeatures(AtiBiosParser2 *parser, AtomConnectorInfo *atomcon, ModernConnector *applecon) {
  if (!atomcon->usConnObjectId) {
    kprintf("ATOM: %s: ASSERT(0 != objId.u16All)\n", "IOReturn AtiBiosParser2::getConnectorFeatures(AtiObjectInfoTableInterface_V2::AtomConnectorInfo &, ConnectorInfo &)");
    return kIOReturnBadArgument;
  }

  uint8_t connector = (uint8_t)atomcon->usGraphicObjIds;

  switch (connector) {
    case CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I:
    case CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D:
      applecon->type = ConnectorDigitalDVI;
      applecon->flags |= 4;
      // unlock 2k res, causes crashes on R9 290X
      // undone by -raddvi
      applecon->transmitter &= 0xCF;
      return kIOReturnSuccess;
    case CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I:
    case CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D:
      applecon->type = ConnectorDigitalDVI;
      applecon->flags |= 4;
      return kIOReturnSuccess;
    case CONNECTOR_OBJECT_ID_HDMI_TYPE_A:
    case CONNECTOR_OBJECT_ID_HDMI_TYPE_B:
      applecon->type = ConnectorHDMI;
      applecon->flags |= 0x204;
      return kIOReturnSuccess;
    case CONNECTOR_OBJECT_ID_VGA:
      applecon->type = ConnectorVGA;
      applecon->flags |= 0x10;
      return kIOReturnSuccess;
    case CONNECTOR_OBJECT_ID_LVDS:
      applecon->type = ConnectorLVDS;
      applecon->flags |= 0x40;
      applecon->features |= 0x9;
      // unlock 2k res
      applecon->transmitter &= 0xCF;
      return kIOReturnSuccess;
    case CONNECTOR_OBJECT_ID_DISPLAYPORT:
      applecon->type = ConnectorDP;
      applecon->flags |= 0x304;
      return kIOReturnSuccess;
    case CONNECTOR_OBJECT_ID_eDP:
      applecon->type = ConnectorLVDS;
      applecon->flags |= 0x100;
      applecon->features |= 0x109;
      return kIOReturnSuccess;
  }

  return kIOReturnInvalid;
}
