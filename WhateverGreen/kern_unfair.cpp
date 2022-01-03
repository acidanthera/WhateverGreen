//
//  kern_unfair.cpp
//  WhateverGreen
//
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "kern_unfair.hpp"
#include "kern_weg.hpp"

#include <IOKit/IOService.h>
#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_file.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_user.hpp>
#include <IOKit/IODeviceTreeSupport.h>

UNFAIR *UNFAIR::callbackUNFAIR;

void UNFAIR::init() {
	callbackUNFAIR = this;

	disableUnfair = !(lilu.getRunMode() & LiluAPI::RunningNormal);
	disableUnfair |= checkKernelArgument("-unfairoff");

	if (disableUnfair)
		return;
}

void UNFAIR::deinit() {

}

void UNFAIR::csValidatePage(vnode *vp, memory_object_t pager, memory_object_offset_t page_offset, const void *data, int *validated_p, int *tainted_p, int *nx_p) {
	FunctionCast(csValidatePage, callbackUNFAIR->orgCsValidatePage)(vp, pager, page_offset, data, validated_p, tainted_p, nx_p);

	char path[PATH_MAX];
	int pathlen = PATH_MAX;
	if (vn_getpath(vp, path, &pathlen) == 0) {
		//DBGLOG("unfair", "csValidatePage %s", path);

		if ((callbackUNFAIR->unfairGva & UnfairDyldSharedCache) != 0 && UserPatcher::matchSharedCachePath(path)) {
			if ((callbackUNFAIR->unfairGva & UnfairRelaxHdcpRequirements) != 0) {
				static const uint8_t find[29] = {
					0x4D, 0x61, 0x63, 0x50, 0x72, 0x6F, 0x35, 0x2C, 0x31, 0x00, 0x4D, 0x61, 0x63, 0x50, 0x72, 0x6F,
					0x36, 0x2C, 0x31, 0x00, 0x49, 0x4F, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65
				};
				if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, find, sizeof(find), BaseDeviceInfo::get().modelIdentifier, 20)))
					DBGLOG("unfair", "patched relaxed drm model");
			}

			if ((callbackUNFAIR->unfairGva & UnfairCustomAppleGvaBoardId) != 0) {
				static const uint8_t find[18] = {
					0x62, 0x6F, 0x61, 0x72, 0x64, 0x2D, 0x69, 0x64, 0x00, 0x68, 0x77, 0x2E, 0x6D, 0x6F, 0x64, 0x65,
					0x6C
				};
				static const uint8_t repl[5] = {
					0x68, 0x77, 0x67, 0x76, 0x61
				};
				if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, find, sizeof(find), repl, sizeof(repl))))
					DBGLOG("unfair", "patched board-id -> hwgva-id");
			}

		} else if ((callbackUNFAIR->unfairGva & UnfairAllowHardwareDrmStreamDecoderOnOldCpuid) != 0 &&
				   (UNLIKELY(strcmp(path, "/System/Library/PrivateFrameworks/CoreLSKDMSE.framework/Versions/A/CoreLSKDMSE") == 0) ||
				   UNLIKELY(strcmp(path, "/System/Library/PrivateFrameworks/CoreLSKD.framework/Versions/A/CoreLSKD") == 0))) {
			static const uint8_t find[] = {0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x0F, 0xA2};
			static const uint8_t repl[] = {0xC7, 0xC0, 0xC3, 0x06, 0x03, 0x00, 0x90, 0x90};
			if (UNLIKELY(KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, find, sizeof(find), repl, sizeof(repl))))
				DBGLOG("unfair", "patched streaming cpuid to haswell");
		}
	}
}

void UNFAIR::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	if (disableUnfair)
		return;

	WEG::getVideoArgument(info, "unfairgva", &unfairGva, sizeof(unfairGva));
	if (unfairGva == 0) {
		DBGLOG("unfair", "disabling unfair gva due to missing boot argument");
		disableUnfair = true;
		return;
	}

	DBGLOG("unfair", "activating with %d bitmask", unfairGva);

	if ((unfairGva & UnfairCustomAppleGvaBoardId) != 0) {
		auto entry = IORegistryEntry::fromPath("/", gIODTPlane);
		if (entry) {
			DBGLOG("unfair", "setting hwgva-id to iMacPro1,1");
			entry->setProperty("hwgva-id", const_cast<char *>("Mac-7BA5B2D9E42DDD94"), static_cast<uint32_t>(sizeof("Mac-7BA5B2D9E42DDD94")));
			entry->release();
		} else {
			SYSLOG("shiki", "failed to obtain iodt tree");
			unfairGva &= ~UnfairCustomAppleGvaBoardId;
		}
	}

	KernelPatcher::RouteRequest csRoute("_cs_validate_page", csValidatePage, orgCsValidatePage);
	if (!patcher.routeMultipleLong(KernelPatcher::KernelID, &csRoute, 1)) {
		SYSLOG("unfair", "failed to route cs validation pages");
	}
}
