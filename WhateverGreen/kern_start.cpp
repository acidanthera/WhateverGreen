//
//  kern_start.cpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_rad.hpp"

static RAD rad;

static const char *bootargOff[] {
	"-radoff"
};

static const char *bootargDebug[] {
	"-raddbg"
};

static const char *bootargBeta[] {
	"-radbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::Yosemite,
	KernelVersion::HighSierra,
	[]() {
		rad.init();
	}
};
