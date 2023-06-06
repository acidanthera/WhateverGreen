//
//  kern_start.cpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_weg.hpp"

static WEG weg;

static const char *bootargOff[] {
	"-wegoff"
};

static const char *bootargDebug[] {
	"-wegdbg"
};

static const char *bootargBeta[] {
	"-wegbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::SnowLeopard,
	KernelVersion::Sonoma,
	[]() {
		weg.init();
	}
};
