//
//  kern_model.cpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/kern_iokit.hpp>

#include "kern_rad.hpp"

/**
 * General rules for GPU names:
 *
 * 1. Only use device identifiers present in Apple kexts starting with 5xxx series
 * 2. Follow Apple naming style (e.g., ATI for 5xxx series, AMD for 6xxx series, Radeon Pro for 5xx GPUs)
 * 3. Write earliest available hardware with slashes (e.g. Radeon Pro 455/555)
 * 4. Avoid using generic names like "AMD Radeon RX"
 * 5. Provide revision identifiers whenever possible
 * 6. Detection order should be vendor-id, device-id, subsystem-vendor-id, subsystem-id, revision-id
 *
 * Some identifiers are taken from https://github.com/pciutils/pciids/blob/master/pci.ids?raw=true
 * Last synced version 2017.07.24 (https://github.com/pciutils/pciids/blob/699e70f3de/pci.ids?raw=true)
 */

struct Model {
	enum Detect {
		DetectDef = 0x0,
		DetectSub = 0x1,
		DetectRev = 0x2,
		DetectAll = DetectSub | DetectRev
	};
	Detect mode {DetectDef};
	uint16_t subven {0};
	uint16_t sub {0};
	uint16_t rev {0};
	const char *name {nullptr};
};

struct DevicePair {
	uint16_t dev;
	const Model *models;
	size_t modelNum;
};

static constexpr Model dev6640[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD FirePro M6100"}
};

static constexpr Model dev6641[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 8930M"}
};

static constexpr Model dev6646[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 M280X"}
};

static constexpr Model dev6647[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 M270X"}
};

static constexpr Model dev665c[] {
	{Model::DetectSub, 0x1462, 0x2932, 0x0000, "AMD Radeon HD 8770"},
	{Model::DetectSub, 0x1462, 0x2934, 0x0000, "AMD Radeon R9 260"},
	{Model::DetectSub, 0x1462, 0x2938, 0x0000, "AMD Radeon R9 360"},
	{Model::DetectSub, 0x148c, 0x0907, 0x0000, "AMD Radeon R7 360"},
	{Model::DetectSub, 0x148c, 0x9260, 0x0000, "AMD Radeon R9 260"},
	{Model::DetectSub, 0x148c, 0x9360, 0x0000, "AMD Radeon R9 360"},
	{Model::DetectSub, 0x1682, 0x0907, 0x0000, "AMD Radeon R7 360"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7790"}
};

static constexpr Model dev665d[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 260"}
};

static constexpr Model dev6704[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD FirePro V7900"}
};

static constexpr Model dev6718[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6970"}
};

static constexpr Model dev6719[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6950"}
};

static constexpr Model dev6720[] {
	{Model::DetectSub, 0x1028, 0x0490, 0x0000, "AMD Radeon HD 6970M"},
	{Model::DetectSub, 0x1028, 0x04a4, 0x0000, "AMD FirePro M8900"},
	{Model::DetectSub, 0x1028, 0x053f, 0x0000, "AMD FirePro M8900"},
	{Model::DetectSub, 0x106b, 0x0b00, 0x0000, "AMD Radeon HD 6970M"},
	{Model::DetectSub, 0x1558, 0x5102, 0x0000, "AMD Radeon HD 6970M"},
	{Model::DetectSub, 0x174b, 0xe188, 0x0000, "AMD Radeon HD 6970M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6990M"}
};

static constexpr Model dev6722[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6900M"}
};

static constexpr Model dev6738[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6870"}
};

static constexpr Model dev6739[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6850"}
};

static constexpr Model dev6740[] {
	{Model::DetectSub, 0x1019, 0x2392, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x1028, 0x04a3, 0x0000, "Precision M4600"},
	{Model::DetectSub, 0x1028, 0x053e, 0x0000, "AMD FirePro M5950"},
	{Model::DetectSub, 0x103c, 0x1630, 0x0000, "AMD FirePro M5950"},
	{Model::DetectSub, 0x103c, 0x1631, 0x0000, "AMD FirePro M5950"},
	{Model::DetectSub, 0x103c, 0x164e, 0x0000, "AMD Radeon HD 6730M5"},
	{Model::DetectSub, 0x103c, 0x1657, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x1658, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x165a, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x165b, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x1688, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x1689, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x168a, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x185e, 0x0000, "AMD Radeon HD 7690M"},
	{Model::DetectSub, 0x103c, 0x3388, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x3389, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectSub, 0x103c, 0x3582, 0x0000, "AMD Radeon HD 6770M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6730M"}
};

static constexpr Model dev6741[] {
	{Model::DetectSub, 0x1028, 0x04c1, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x1028, 0x04c5, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x1028, 0x04cd, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x1028, 0x04d7, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x1028, 0x04d9, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x1028, 0x052d, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x103c, 0x1646, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x103c, 0x1688, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x103c, 0x1689, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x103c, 0x168a, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x103c, 0x1860, 0x0000, "AMD Radeon HD 7690M"},
	{Model::DetectSub, 0x103c, 0x3385, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x103c, 0x3560, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x103c, 0x358d, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x103c, 0x3590, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x103c, 0x3593, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x1043, 0x2125, 0x0000, "AMD Radeon HD 7670M"},
	{Model::DetectSub, 0x1043, 0x2127, 0x0000, "AMD Radeon HD 7670M"},
	{Model::DetectSub, 0x104d, 0x907b, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x104d, 0x9080, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x104d, 0x9081, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x1179, 0xfd63, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x1179, 0xfd65, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x144d, 0xc0b3, 0x0000, "AMD Radeon HD 6750M"},
	{Model::DetectSub, 0x144d, 0xc539, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x144d, 0xc609, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectSub, 0x17aa, 0x21e1, 0x0000, "AMD Radeon HD 6630M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6650M"}
};

static constexpr Model dev6745[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6600M"}
};

static constexpr Model dev6750[] {
	{Model::DetectSub, 0x1462, 0x2670, 0x0000, "AMD Radeon HD 6670A"},
	{Model::DetectSub, 0x17aa, 0x3079, 0x0000, "AMD Radeon HD 7650A"},
	{Model::DetectSub, 0x17aa, 0x3087, 0x0000, "AMD Radeon HD 7650A"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6650A"}
};

static constexpr Model dev6758[] {
	{Model::DetectSub, 0x1028, 0x0b0e, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x103c, 0x6882, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x174b, 0xe181, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x174b, 0xe198, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x1787, 0x2309, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x1043, 0x0443, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x1458, 0x2205, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x1043, 0x03ea, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x1458, 0x2545, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectSub, 0x174b, 0xe194, 0x0000, "AMD Radeon HD 6670"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7670"}
};

static constexpr Model dev6759[] {
	{Model::DetectSub, 0x1462, 0x2509, 0x0000, "AMD Radeon HD 7570"},
	{Model::DetectSub, 0x148c, 0x7570, 0x0000, "AMD Radeon HD 7570"},
	{Model::DetectSub, 0x1682, 0x3280, 0x0000, "AMD Radeon HD 7570"},
	{Model::DetectSub, 0x1682, 0x3530, 0x0000, "AMD Radeon HD 8850"},
	{Model::DetectSub, 0x174b, 0x7570, 0x0000, "AMD Radeon HD 7570"},
	{Model::DetectSub, 0x1b0a, 0x90b5, 0x0000, "AMD Radeon HD 7570"},
	{Model::DetectSub, 0x1b0a, 0x90b6, 0x0000, "AMD Radeon HD 7570"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6570"}
};

static constexpr Model dev6760[] {
	{Model::DetectSub, 0x1028, 0x04cc, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x1028, 0x051c, 0x0000, "AMD Radeon HD 6450M"},
	{Model::DetectSub, 0x1028, 0x051d, 0x0000, "AMD Radeon HD 6450M"},
	{Model::DetectSub, 0x103c, 0x1622, 0x0000, "AMD Radeon HD 6450M"},
	{Model::DetectSub, 0x103c, 0x1623, 0x0000, "AMD Radeon HD 6450M"},
	{Model::DetectSub, 0x103c, 0x1656, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x1658, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x1659, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x165b, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x167d, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x167f, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x169c, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x1855, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x103c, 0x1859, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x103c, 0x185c, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x103c, 0x185d, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x103c, 0x185f, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x103c, 0x1863, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x103c, 0x355c, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x355f, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x3581, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x358c, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x358f, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x3592, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x3596, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x103c, 0x3671, 0x0000, "AMD FirePro M3900"},
	{Model::DetectSub, 0x1043, 0x100a, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x102a, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x1043, 0x104b, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x105d, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x106b, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x106d, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x107d, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x2002, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x2107, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x2108, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x2109, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x8515, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x8517, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1043, 0x855a, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0x0001, 0x0000, "AMD Radeon HD 6450M"},
	{Model::DetectSub, 0x1179, 0x0003, 0x0000, "AMD Radeon HD 6450M"},
	{Model::DetectSub, 0x1179, 0x0004, 0x0000, "AMD Radeon HD 6450M"},
	{Model::DetectSub, 0x1179, 0xfb22, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb23, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb2c, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb31, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb32, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb33, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb38, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb39, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb3a, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb40, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb41, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb42, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb47, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb48, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb51, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb52, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb53, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb81, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb82, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfb83, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfc52, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfc56, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfcd3, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfcd4, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfcee, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x1179, 0xfdee, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x144d, 0xc0b3, 0x0000, "AMD Radeon HD 6490M"},
	{Model::DetectSub, 0x144d, 0xc609, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x144d, 0xc625, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x144d, 0xc636, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x17aa, 0x3900, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x17aa, 0x3902, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x17aa, 0x3970, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x17aa, 0x5101, 0x0000, "AMD Radeon HD 7470M"},
	{Model::DetectSub, 0x17aa, 0x5102, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x17aa, 0x5103, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectSub, 0x17aa, 0x5106, 0x0000, "AMD Radeon HD 7450M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6470M"}
};

static constexpr Model dev6761[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6430M"}
};

static constexpr Model dev6768[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6400M"}
};

static constexpr Model dev6770[] {
	{Model::DetectSub, 0x17aa, 0x308d, 0x0000, "AMD Radeon HD 7450A"},
	{Model::DetectSub, 0x17aa, 0x3658, 0x0000, "AMD Radeon HD 7470A"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6450A"}
};

static constexpr Model dev6779[] {
	{Model::DetectSub, 0x103c, 0x2aee, 0x0000, "AMD Radeon HD 7450A"},
	{Model::DetectSub, 0x1462, 0x2346, 0x0000, "AMD Radeon HD 7450"},
	{Model::DetectSub, 0x1462, 0x2496, 0x0000, "AMD Radeon HD 7450"},
	{Model::DetectSub, 0x148c, 0x7450, 0x0000, "AMD Radeon HD 7450"},
	{Model::DetectSub, 0x148c, 0x8450, 0x0000, "AMD Radeon HD 8450"},
	{Model::DetectSub, 0x1545, 0x7470, 0x0000, "AMD Radeon HD 7470"},
	{Model::DetectSub, 0x1642, 0x3a66, 0x0000, "AMD Radeon HD 7450"},
	{Model::DetectSub, 0x1642, 0x3a76, 0x0000, "AMD Radeon HD 7450"},
	{Model::DetectSub, 0x1682, 0x3200, 0x0000, "AMD Radeon HD 7450"},
	{Model::DetectSub, 0x174b, 0x7450, 0x0000, "AMD Radeon HD 7450"},
	{Model::DetectSub, 0x1b0a, 0x90a8, 0x0000, "AMD Radeon HD 6450A"},
	{Model::DetectSub, 0x1b0a, 0x90b3, 0x0000, "AMD Radeon HD 7450A"},
	{Model::DetectSub, 0x1b0a, 0x90bb, 0x0000, "AMD Radeon HD 7450A"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 6450"}
};

static constexpr Model dev6780[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD FirePro W9000"}
};

static constexpr Model dev6790[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7970"}
};

static constexpr Model dev6798[] {
	{Model::DetectSub, 0x1002, 0x3001, 0x0000, "AMD Radeon R9 280X"},
	{Model::DetectSub, 0x1002, 0x4000, 0x0000, "AMD Radeon HD 8970"},
	{Model::DetectSub, 0x1043, 0x3001, 0x0000, "AMD Radeon R9 280X"},
	{Model::DetectSub, 0x1043, 0x3006, 0x0000, "AMD Radeon R9 280X"},
	{Model::DetectSub, 0x1043, 0x3005, 0x0000, "AMD Radeon R9 280X"},
	{Model::DetectSub, 0x1462, 0x2775, 0x0000, "AMD Radeon R9 280X"},
	{Model::DetectSub, 0x1682, 0x3001, 0x0000, "AMD Radeon R9 280X"},
	{Model::DetectSub, 0x1043, 0x9999, 0x0000, "ASUS ARES II"},
	{Model::DetectSub, 0x1458, 0x3001, 0x0000, "AMD Radeon R9 280X"},
	{Model::DetectSub, 0x1787, 0x2317, 0x0000, "AMD Radeon HD 7990"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7970"}
};

static constexpr Model dev679a[] {
	{Model::DetectSub, 0x1002, 0x3000, 0x0000, "AMD Radeon HD 7950"},
	{Model::DetectSub, 0x174b, 0x3000, 0x0000, "AMD Radeon HD 7950"},
	{Model::DetectSub, 0x1043, 0x047e, 0x0000, "AMD Radeon HD 7950"},
	{Model::DetectSub, 0x1043, 0x0424, 0x0000, "AMD Radeon HD 7950"},
	{Model::DetectSub, 0x174b, 0xa003, 0x0000, "AMD Radeon R9 280"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 8950"}
};

static constexpr Model dev679e[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7870"}
};

static constexpr Model dev67b0[] {
	{Model::DetectSub, 0x1028, 0x0b00, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x103c, 0x6566, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1043, 0x0476, 0x0000, "ASUS ARES III"},
	{Model::DetectSub, 0x1043, 0x04d7, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1043, 0x04db, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1043, 0x04df, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1043, 0x04e9, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1458, 0x22bc, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1458, 0x22c1, 0x0000, "AMD Radeon R9 390"},
	{Model::DetectSub, 0x1462, 0x2015, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x148c, 0x2347, 0x0000, "Devil 13 Dual Core R9 290X"},
	{Model::DetectSub, 0x148c, 0x2357, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1682, 0x9395, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x174b, 0x0e34, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x174b, 0xe324, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectSub, 0x1787, 0x2357, 0x0000, "AMD Radeon R9 390X"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 290X"}
};

static constexpr Model dev67c0[] {
	{Model::DetectRev, 0x0000, 0x0000, 0x0080, "AMD Radeon E9550"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Pro WX 7100"}
};

static constexpr Model dev67c4[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Pro WX 7100"}
};

static constexpr Model dev67c7[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Pro WX 5100"}
};

static constexpr Model dev67df[] {
	{Model::DetectRev, 0x0000, 0x0000, 0x00c1, "Radeon RX 580"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c2, "Radeon RX 570"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c3, "Radeon RX 580"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c4, "Radeon RX 480"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c5, "Radeon RX 470"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c6, "Radeon RX 570"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c7, "Radeon RX 480"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00cf, "Radeon RX 470"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00d7, "Radeon RX 470"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00e0, "Radeon RX 470"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00e7, "Radeon RX 580"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00ef, "Radeon RX 570"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00ff, "Radeon RX 470"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon RX 480"}
};

static constexpr Model dev67e0[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon Pro WX 7100"}
};

static constexpr Model dev67e3[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Pro WX 4100"}
};

static constexpr Model dev67ef[] {
	{Model::DetectRev, 0x0000, 0x0000, 0x00c0, "Radeon Pro 460/560"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c1, "Radeon RX 460"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c5, "Radeon RX 460"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c7, "Radeon Pro 455/555"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00cf, "Radeon RX 460"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00e0, "Radeon RX 560"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00e5, "Radeon RX 560"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00e7, "Radeon RX 560"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00ef, "Radeon Pro 450/550"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00ff, "Radeon RX 460"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Pro 460"}
};

static constexpr Model dev67ff[] {
	{Model::DetectRev, 0x0000, 0x0000, 0x00c0, "Radeon Pro 465"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c1, "Radeon Pro 560"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00cf, "Radeon RX 560"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00ef, "Radeon RX 560"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00ff, "Radeon RX 550"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Pro 560"}
};

static constexpr Model dev6800[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7970M"}
};

static constexpr Model dev6801[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 8970M"}
};

static constexpr Model dev6806[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD FirePro W7000"}
};

static constexpr Model dev6808[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD FirePro W7000"}
};

static constexpr Model dev6810[] {
	{Model::DetectSub, 0x1458, 0x2272, 0x0000, "AMD Radeon R9 270X"},
	{Model::DetectSub, 0x1462, 0x3033, 0x0000, "AMD Radeon R9 270X"},
	{Model::DetectSub, 0x174b, 0xe271, 0x0000, "AMD Radeon R9 270X"},
	{Model::DetectSub, 0x1787, 0x201c, 0x0000, "AMD Radeon R9 270X"},
	{Model::DetectSub, 0x148c, 0x0908, 0x0000, "AMD Radeon R9 370"},
	{Model::DetectSub, 0x1682, 0x7370, 0x0000, "AMD Radeon R7 370"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 370X"}
};

static constexpr Model dev6818[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7870"}
};

static constexpr Model dev6819[] {
	{Model::DetectSub, 0x174b, 0xe218, 0x0000, "AMD Radeon HD 7850"},
	{Model::DetectSub, 0x174b, 0xe221, 0x0000, "AMD Radeon HD 7850"},
	{Model::DetectSub, 0x1458, 0x255a, 0x0000, "AMD Radeon HD 7850"},
	{Model::DetectSub, 0x1462, 0x3058, 0x0000, "AMD Radeon R7 265"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 270"}
};

static constexpr Model dev6820[] {
	{Model::DetectSub, 0x103c, 0x1851, 0x0000, "AMD Radeon HD 7750M"},
	{Model::DetectSub, 0x17aa, 0x3643, 0x0000, "AMD Radeon R9 A375"},
	{Model::DetectSub, 0x17aa, 0x3801, 0x0000, "AMD Radeon R9 M275"},
	{Model::DetectSub, 0x1028, 0x06da, 0x0000, "AMD FirePro W5170M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 M375"}
};

static constexpr Model dev6821[] {
	{Model::DetectSub, 0x1002, 0x031e, 0x0000, "AMD FirePro SX4000"},
	{Model::DetectSub, 0x1028, 0x05cc, 0x0000, "AMD FirePro M5100"},
	{Model::DetectSub, 0x1028, 0x15cc, 0x0000, "AMD FirePro M5100"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 M370X"}
};

static constexpr Model dev6823[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 8850M/R9 M265X"}
};

static constexpr Model dev6825[] {
	{Model::DetectSub, 0x1028, 0x053f, 0x0000, "AMD FirePro M6000"},
	{Model::DetectSub, 0x1028, 0x05cd, 0x0000, "AMD FirePro M6000"},
	{Model::DetectSub, 0x1028, 0x15cd, 0x0000, "AMD FirePro M6000"},
	{Model::DetectSub, 0x103c, 0x176c, 0x0000, "AMD FirePro M6000"},
	{Model::DetectSub, 0x8086, 0x2111, 0x0000, "AMD Radeon HD 7730M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7870M"}
};

static constexpr Model dev6827[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7850M/8850M"}
};

static constexpr Model dev682b[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 8830M"}
};

static constexpr Model dev682d[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD FirePro M4000"}
};

static constexpr Model dev682f[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7730M"}
};

static constexpr Model dev6835[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 255"}
};

static constexpr Model dev6839[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7700"}
};

static constexpr Model dev683b[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7700"}
};

static constexpr Model dev683d[] {
	{Model::DetectSub, 0x1002, 0x0030, 0x0000, "AMD Radeon HD 8760"},
	{Model::DetectSub, 0x1019, 0x0030, 0x0000, "AMD Radeon HD 8760"},
	{Model::DetectSub, 0x103c, 0x6890, 0x0000, "AMD Radeon HD 8760"},
	{Model::DetectSub, 0x1043, 0x8760, 0x0000, "AMD Radeon HD 8760"},
	{Model::DetectSub, 0x1462, 0x2710, 0x0000, "AMD Radeon HD 7770"},
	{Model::DetectSub, 0x174b, 0x8304, 0x0000, "AMD Radeon HD 8760"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7770"}
};

static constexpr Model dev683f[] {
	{Model::DetectSub, 0x1462, 0x2790, 0x0000, "AMD Radeon HD 8740"},
	{Model::DetectSub, 0x1462, 0x2791, 0x0000, "AMD Radeon HD 8740"},
	{Model::DetectSub, 0x1642, 0x3b97, 0x0000, "AMD Radeon HD 8740"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7750"}
};

static constexpr Model dev6840[] {
	{Model::DetectSub, 0x1025, 0x0696, 0x0000, "AMD Radeon HD 7650M"},
	{Model::DetectSub, 0x1025, 0x0697, 0x0000, "AMD Radeon HD 7650M"},
	{Model::DetectSub, 0x1025, 0x0698, 0x0000, "AMD Radeon HD 7650M"},
	{Model::DetectSub, 0x1025, 0x0699, 0x0000, "AMD Radeon HD 7650M"},
	{Model::DetectSub, 0x103c, 0x1789, 0x0000, "AMD FirePro M2000"},
	{Model::DetectSub, 0x103c, 0x17f1, 0x0000, "AMD Radeon HD 7570M"},
	{Model::DetectSub, 0x103c, 0x17f4, 0x0000, "AMD Radeon HD 7650M"},
	{Model::DetectSub, 0x103c, 0x1813, 0x0000, "AMD Radeon HD 7590M"},
	{Model::DetectSub, 0x144d, 0xc0c5, 0x0000, "AMD Radeon HD 7690M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7670M"}
};

static constexpr Model dev6841[] {
	{Model::DetectSub, 0x1028, 0x057f, 0x0000, "AMD Radeon HD 7570M"},
	{Model::DetectSub, 0x103c, 0x17f1, 0x0000, "AMD Radeon HD 7570M"},
	{Model::DetectSub, 0x103c, 0x1813, 0x0000, "AMD Radeon HD 7570M"},
	{Model::DetectSub, 0x1179, 0x0001, 0x0000, "AMD Radeon HD 7570M"},
	{Model::DetectSub, 0x1179, 0x0002, 0x0000, "AMD Radeon HD 7570M"},
	{Model::DetectSub, 0x1179, 0xfb43, 0x0000, "AMD Radeon HD 7550M"},
	{Model::DetectSub, 0x1179, 0xfb91, 0x0000, "AMD Radeon HD 7550M"},
	{Model::DetectSub, 0x1179, 0xfb92, 0x0000, "AMD Radeon HD 7550M"},
	{Model::DetectSub, 0x1179, 0xfb93, 0x0000, "AMD Radeon HD 7550M"},
	{Model::DetectSub, 0x1179, 0xfba2, 0x0000, "AMD Radeon HD 7550M"},
	{Model::DetectSub, 0x1179, 0xfba3, 0x0000, "AMD Radeon HD 7550M"},
	{Model::DetectSub, 0x144d, 0xc0c7, 0x0000, "AMD Radeon HD 7550M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon HD 7650M"}
};

static constexpr Model dev6861[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Pro WX 9100"}
};

static constexpr Model dev6863[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon Vega Frontier Edition"}
};

static constexpr Model dev687f[] {
	{Model::DetectRev, 0x0000, 0x0000, 0x00c0, "Radeon RX Vega 64"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c1, "Radeon RX Vega 64"},
	{Model::DetectRev, 0x0000, 0x0000, 0x00c3, "Radeon RX Vega 56"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "Radeon RX Vega 64"}
};

static constexpr Model dev6898[] {
	{Model::DetectSub, 0x174b, 0x6870, 0x0000, "AMD Radeon HD 6870"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5870"}
};

static constexpr Model dev6899[] {
	{Model::DetectSub, 0x174b, 0x237b, 0x0000, "ATI Radeon HD 5850 (x2)"},
	{Model::DetectSub, 0x174b, 0x6850, 0x0000, "AMD Radeon HD 6850"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5850"}
};

static constexpr Model dev68a0[] {
	{Model::DetectSub, 0x1028, 0x12ef, 0x0000, "ATI FirePro M7820"},
	{Model::DetectSub, 0x103c, 0x1520, 0x0000, "ATI FirePro M7820"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Mobility Radeon HD 5870"}
};

static constexpr Model dev68a1[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Mobility Radeon HD 5850"}
};

static constexpr Model dev68b0[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5770"}
};

static constexpr Model dev68b1[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5770"}
};

static constexpr Model dev68b8[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5770"}
};

static constexpr Model dev68c0[] {
	{Model::DetectSub, 0x103c, 0x1521, 0x0000, "ATI FirePro M5800"},
	{Model::DetectSub, 0x17aa, 0x3978, 0x0000, "AMD Radeon HD 6570M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Mobility Radeon HD 5730"}
};

static constexpr Model dev68c1[] {
	{Model::DetectSub, 0x1025, 0x0347, 0x0000, "ATI Mobility Radeon HD 5470"},
	{Model::DetectSub, 0x1025, 0x0517, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x051a, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x051b, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x051c, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x051d, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x0525, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x0526, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x052b, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x052c, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x053c, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x053d, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x053e, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x053f, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x1025, 0x0607, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectSub, 0x103c, 0x1521, 0x0000, "ATI FirePro M5800"},
	{Model::DetectSub, 0x103c, 0xfd52, 0x0000, "AMD Radeon HD 6530M"},
	{Model::DetectSub, 0x103c, 0xfd63, 0x0000, "AMD Radeon HD 6530M"},
	{Model::DetectSub, 0x103c, 0xfd65, 0x0000, "AMD Radeon HD 6530M"},
	{Model::DetectSub, 0x103c, 0xfdd2, 0x0000, "AMD Radeon HD 6530M"},
	{Model::DetectSub, 0x17aa, 0x3977, 0x0000, "AMD Radeon HD 6550M"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Mobility Radeon HD 5650"}
};

static constexpr Model dev68d8[] {
	{Model::DetectSub, 0x1028, 0x68e0, 0x0000, "ATI Radeon HD 5670"},
	{Model::DetectSub, 0x174b, 0x5690, 0x0000, "ATI Radeon HD 5690"},
	{Model::DetectSub, 0x174b, 0xe151, 0x0000, "ATI Radeon HD 5670"},
	{Model::DetectSub, 0x174b, 0xe166, 0x0000, "ATI Radeon HD 5670"},
	{Model::DetectSub, 0x1043, 0x0356, 0x0000, "ATI Radeon HD 5670"},
	{Model::DetectSub, 0x1787, 0x200d, 0x0000, "ATI Radeon HD 5670"},
	{Model::DetectSub, 0x17af, 0x3011, 0x0000, "ATI Radeon HD 5690"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5730"}
};

static constexpr Model dev68d9[] {
	{Model::DetectSub, 0x148c, 0x3000, 0x0000, "AMD Radeon HD 6510"},
	{Model::DetectSub, 0x148c, 0x3001, 0x0000, "AMD Radeon HD 6610"},
	{Model::DetectSub, 0x1545, 0x7570, 0x0000, "AMD Radeon HD 7570"},
	{Model::DetectSub, 0x174b, 0x3000, 0x0000, "AMD Radeon HD 6510"},
	{Model::DetectSub, 0x174b, 0x6510, 0x0000, "AMD Radeon HD 6510"},
	{Model::DetectSub, 0x174b, 0x6610, 0x0000, "AMD Radeon HD 6610"},
	{Model::DetectSub, 0x1787, 0x3000, 0x0000, "AMD Radeon HD 6510"},
	{Model::DetectSub, 0x17af, 0x3000, 0x0000, "AMD Radeon HD 6510"},
	{Model::DetectSub, 0x17af, 0x3010, 0x0000, "ATI Radeon HD 5630"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5570"}
};

static constexpr Model dev68e0[] {
	{Model::DetectSub, 0x1682, 0x9e52, 0x0000, "ATI FirePro M3800"},
	{Model::DetectSub, 0x1682, 0x9e53, 0x0000, "ATI FirePro M3800"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Mobility Radeon HD 5450"}
};

static constexpr Model dev68e1[] {
	{Model::DetectSub, 0x148c, 0x3001, 0x0000, "AMD Radeon HD 6230"},
	{Model::DetectSub, 0x148c, 0x3002, 0x0000, "AMD Radeon HD 6250"},
	{Model::DetectSub, 0x148c, 0x3003, 0x0000, "AMD Radeon HD 6350"},
	{Model::DetectSub, 0x148c, 0x7350, 0x0000, "AMD Radeon HD 7350"},
	{Model::DetectSub, 0x148c, 0x8350, 0x0000, "AMD Radeon HD 8350"},
	{Model::DetectSub, 0x1545, 0x7350, 0x0000, "AMD Radeon HD 7350"},
	{Model::DetectSub, 0x1682, 0x7350, 0x0000, "AMD Radeon HD 7350"},
	{Model::DetectSub, 0x174b, 0x5470, 0x0000, "ATI Radeon HD 5470"},
	{Model::DetectSub, 0x174b, 0x6230, 0x0000, "AMD Radeon HD 6230"},
	{Model::DetectSub, 0x174b, 0x6350, 0x0000, "AMD Radeon HD 6350"},
	{Model::DetectSub, 0x174b, 0x7350, 0x0000, "AMD Radeon HD 7350"},
	{Model::DetectSub, 0x17af, 0x3001, 0x0000, "AMD Radeon HD 6230"},
	{Model::DetectSub, 0x17af, 0x3014, 0x0000, "AMD Radeon HD 6350"},
	{Model::DetectSub, 0x17af, 0x3015, 0x0000, "AMD Radeon HD 7350"},
	{Model::DetectSub, 0x17af, 0x8350, 0x0000, "AMD Radeon HD 8350"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "ATI Radeon HD 5450"}
};

static constexpr Model dev6920[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 M395"}
};

static constexpr Model dev6921[] {
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 M295X"}
};

static constexpr Model dev6938[] {
	{Model::DetectSub, 0x106b, 0x013a, 0x0000, "AMD Radeon R9 M295X"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 380X"}
};

static constexpr Model dev6939[] {
	{Model::DetectSub, 0x148c, 0x9380, 0x0000, "AMD Radeon R9 380"},
	{Model::DetectSub, 0x174b, 0xe308, 0x0000, "AMD Radeon R9 380"},
	{Model::DetectSub, 0x1043, 0x0498, 0x0000, "AMD Radeon R9 380"},
	{Model::DetectSub, 0x1462, 0x2015, 0x0000, "AMD Radeon R9 380"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon R9 285"}
};

static constexpr Model dev7300[] {
	{Model::DetectSub, 0x1002, 0x1b36, 0x0000, "AMD Radeon Pro Duo"},
	{Model::DetectSub, 0x1043, 0x04a0, 0x0000, "AMD Radeon FURY X"},
	{Model::DetectSub, 0x1002, 0x0b36, 0x0000, "AMD Radeon FURY X"},
	{Model::DetectDef, 0x0000, 0x0000, 0x0000, "AMD Radeon FURY"}
};

static constexpr DevicePair devices[] {
	{0x6640, dev6640, arrsize(dev6640)},
	{0x6641, dev6641, arrsize(dev6641)},
	{0x6646, dev6646, arrsize(dev6646)},
	{0x6647, dev6647, arrsize(dev6647)},
	{0x665c, dev665c, arrsize(dev665c)},
	{0x665d, dev665d, arrsize(dev665d)},
	{0x6704, dev6704, arrsize(dev6704)},
	{0x6718, dev6718, arrsize(dev6718)},
	{0x6719, dev6719, arrsize(dev6719)},
	{0x6720, dev6720, arrsize(dev6720)},
	{0x6722, dev6722, arrsize(dev6722)},
	{0x6738, dev6738, arrsize(dev6738)},
	{0x6739, dev6739, arrsize(dev6739)},
	{0x6740, dev6740, arrsize(dev6740)},
	{0x6741, dev6741, arrsize(dev6741)},
	{0x6745, dev6745, arrsize(dev6745)},
	{0x6750, dev6750, arrsize(dev6750)},
	{0x6758, dev6758, arrsize(dev6758)},
	{0x6759, dev6759, arrsize(dev6759)},
	{0x6760, dev6760, arrsize(dev6760)},
	{0x6761, dev6761, arrsize(dev6761)},
	{0x6768, dev6768, arrsize(dev6768)},
	{0x6770, dev6770, arrsize(dev6770)},
	{0x6779, dev6779, arrsize(dev6779)},
	{0x6780, dev6780, arrsize(dev6780)},
	{0x6790, dev6790, arrsize(dev6790)},
	{0x6798, dev6798, arrsize(dev6798)},
	{0x679a, dev679a, arrsize(dev679a)},
	{0x679e, dev679e, arrsize(dev679e)},
	{0x67b0, dev67b0, arrsize(dev67b0)},
	{0x67c0, dev67c0, arrsize(dev67c0)},
	{0x67c4, dev67c4, arrsize(dev67c4)},
	{0x67c7, dev67c7, arrsize(dev67c7)},
	{0x67df, dev67df, arrsize(dev67df)},
	{0x67e0, dev67e0, arrsize(dev67e0)},
	{0x67e3, dev67e3, arrsize(dev67e3)},
	{0x67ef, dev67ef, arrsize(dev67ef)},
	{0x67ff, dev67ff, arrsize(dev67ff)},
	{0x6800, dev6800, arrsize(dev6800)},
	{0x6801, dev6801, arrsize(dev6801)},
	{0x6806, dev6806, arrsize(dev6806)},
	{0x6808, dev6808, arrsize(dev6808)},
	{0x6810, dev6810, arrsize(dev6810)},
	{0x6818, dev6818, arrsize(dev6818)},
	{0x6819, dev6819, arrsize(dev6819)},
	{0x6820, dev6820, arrsize(dev6820)},
	{0x6821, dev6821, arrsize(dev6821)},
	{0x6823, dev6823, arrsize(dev6823)},
	{0x6825, dev6825, arrsize(dev6825)},
	{0x6827, dev6827, arrsize(dev6827)},
	{0x682b, dev682b, arrsize(dev682b)},
	{0x682d, dev682d, arrsize(dev682d)},
	{0x682f, dev682f, arrsize(dev682f)},
	{0x6835, dev6835, arrsize(dev6835)},
	{0x6839, dev6839, arrsize(dev6839)},
	{0x683b, dev683b, arrsize(dev683b)},
	{0x683d, dev683d, arrsize(dev683d)},
	{0x683f, dev683f, arrsize(dev683f)},
	{0x6840, dev6840, arrsize(dev6840)},
	{0x6841, dev6841, arrsize(dev6841)},
	{0x6861, dev6861, arrsize(dev6861)},
	{0x6863, dev6863, arrsize(dev6863)},
	{0x687f, dev687f, arrsize(dev687f)},
	{0x6898, dev6898, arrsize(dev6898)},
	{0x6899, dev6899, arrsize(dev6899)},
	{0x68a0, dev68a0, arrsize(dev68a0)},
	{0x68a1, dev68a1, arrsize(dev68a1)},
	{0x68b0, dev68b0, arrsize(dev68b0)},
	{0x68b1, dev68b1, arrsize(dev68b1)},
	{0x68b8, dev68b8, arrsize(dev68b8)},
	{0x68c0, dev68c0, arrsize(dev68c0)},
	{0x68c1, dev68c1, arrsize(dev68c1)},
	{0x68d8, dev68d8, arrsize(dev68d8)},
	{0x68d9, dev68d9, arrsize(dev68d9)},
	{0x68e0, dev68e0, arrsize(dev68e0)},
	{0x68e1, dev68e1, arrsize(dev68e1)},
	{0x6920, dev6920, arrsize(dev6920)},
	{0x6921, dev6921, arrsize(dev6921)},
	{0x6938, dev6938, arrsize(dev6938)},
	{0x6939, dev6939, arrsize(dev6939)},
	{0x7300, dev7300, arrsize(dev7300)}
};

const char *RAD::getModel(uint16_t ven, uint16_t dev, uint16_t rev, uint16_t subven, uint16_t sub) {
	// Initially check vendor-id common to all ATI/AMD (0x1002)
	if (ven != WIOKit::VendorID::ATIAMD)
		return nullptr;
	
	for (auto &device : devices) {
		if (device.dev == dev) {
			for (size_t j = 0; j < device.modelNum; j++) {
				auto &model = device.models[j];
				
				if (model.mode & Model::DetectSub && (model.subven != subven || model.sub != sub))
					continue;
				
				if (model.mode & Model::DetectRev && (model.rev != rev))
					continue;
				
				return model.name;
			}
			break;
		}
	}
	
	return nullptr;
}
