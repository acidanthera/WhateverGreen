//
//  kern_con.hpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_con_hpp
#define kern_con_hpp

#include <Headers/kern_util.hpp>
#include <libkern/libkern.h>

namespace RADConnectors {

	/**
	 *  Connectors from AMDSupport before 10.12
	 */
	struct LegacyConnector {
		uint32_t type;
		uint32_t flags;
		uint16_t features;
		uint16_t priority;
		uint8_t transmitter;
		uint8_t encoder;
		uint8_t hotplug;
		uint8_t sense;
	};

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

	/**
	 *  Opaque connector type for simplicity
	 */
	union Connector {
		LegacyConnector legacy;
		ModernConnector modern;

		static_assert(sizeof(LegacyConnector) == 16, "LegacyConnector has wrong size");
		static_assert(sizeof(ModernConnector) == 24, "ModernConnector has wrong size");

		/**
		 *  Assigns connector from one to another
		 *
		 *  @param out destination connector
		 *  @param in source connector
		 */
		template <typename T, typename Y>
		static inline void assign(T &out, const Y &in) {
			memset(&out, 0, sizeof(T));
			out.type = in.type;
			out.flags = in.flags;
			out.features = in.features;
			out.priority = in.priority;
			out.transmitter = in.transmitter;
			out.encoder = in.encoder;
			out.hotplug = in.hotplug;
			out.sense = in.sense;
		}
	};

	/**
	 *  Connector types available in the drivers
	 */
	enum ConnectorType {
		ConnectorLVDS = 0x2,
		ConnectorDigitalDVI = 0x4,
		ConnectorSVID = 0x8,
		ConnectorVGA = 0x10,
		ConnectorDP = 0x400,
		ConnectorHDMI = 0x800,
		ConnectorAnalogDVI = 0x2000
	};

	/**
	 *  Prints connector type
	 *
	 *  @param type connector type
	 */
	inline const char *printType(uint32_t type) {
		switch (type) {
			case ConnectorLVDS:
				return "LVDS";
			case ConnectorDigitalDVI:
				return "DVI ";
			case ConnectorSVID:
				return "SVID";
			case ConnectorVGA:
				return "VGA ";
			case ConnectorDP:
				return "DP  ";
			case ConnectorHDMI:
				return "HDMI";
			case ConnectorAnalogDVI:
				return "ADVI";
			default:
				return "UNKN";
		}
	}

	/**
	 *  Prints connector
	 *
	 *  @param out        out buffer
	 *  @param connector  typed connector
	 *
	 *  @return out
	 */
	template <typename T, size_t N>
	inline char *printConnector(char (&out)[N], T &connector) {
		snprintf(out, N, "type %08X (%s) flags %08X feat %04X pri %04X txmit %02X enc %02X hotplug %02X sense %02X", connector.type, printType(connector.type),
				 connector.flags, connector.features, connector.priority, connector.transmitter, connector.encoder, connector.hotplug, connector.sense);
		return out;
	}

	/**
	 *  Is modern system
	 *
	 *  @return true if modern connectors are used
	 */
	inline bool modern() {
		return getKernelVersion() >= KernelVersion::Sierra;
	}

	/**
	 *  Prints connectors
	 *
	 *  @param con  pointer to an array of legacy or modern connectors (depends on the kernel version)
	 *  @param num  number of connectors in con
	 */
	inline void print(Connector *con, uint8_t num) {
#ifdef DEBUG
		for (uint8_t i = 0; con && i < num; i++) {
			char tmp[192];
			DBGLOG("con", "%u is %s", i, modern() ?
				   printConnector(tmp, (&con->modern)[i]) : printConnector(tmp, (&con->legacy)[i]));
		}
#endif
	}

	/**
	 *  Sanity check connector size
	 *
	 *  @param size  buffer size in bytes
	 *  @param num   number of connectors
	 *
	 *  @return true if connectors are in either legacy or modern format
	 */
	inline bool valid(uint32_t size, uint8_t num) {
		return  (size % sizeof(ModernConnector) == 0 && size / sizeof(ModernConnector) == num) ||
		        (size % sizeof(LegacyConnector) == 0 && size / sizeof(LegacyConnector) == num);
	}

	/**
	 *  Copy new connectors
	 *
	 *  @param out  destination connectors
	 *  @param num  number of copied connectors
	 *  @param in   source connectors
	 *  @param size size of source connectors
	 */
	inline void copy(RADConnectors::Connector *out, uint8_t num, const RADConnectors::Connector *in, uint32_t size) {
		bool outModern = modern();
		bool inModern = size % sizeof(ModernConnector) == 0 && size / sizeof(ModernConnector) == num;

		for (uint8_t i = 0; i < num; i++) {
			if (outModern) {
				if (inModern)
					Connector::assign((&out->modern)[i], (&in->modern)[i]);
				else
					Connector::assign((&out->modern)[i], (&in->legacy)[i]);
			} else {
				if (inModern)
					Connector::assign((&out->legacy)[i], (&in->modern)[i]);
				else
					Connector::assign((&out->legacy)[i], (&in->legacy)[i]);
			}

		}
	}
};

#endif /* kern_con_hpp */
