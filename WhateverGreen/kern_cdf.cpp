//
//  kern_cdf.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_cdf.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>

// NVDAGK100Hal.kext    - system built-in, for Kepler
static const char *pathGKHal[] = {
	"/System/Library/Extensions/NVDAGK100Hal.kext/Contents/MacOS/NVDAGK100Hal"
};

// NVDAGK100HalWeb.kext - from web driver, for Kepler
static const char *pathGKWeb[] = {
	"/Library/Extensions/NVDAGK100HalWeb.kext/Contents/MacOS/NVDAGK100HalWeb",
	"/System/Library/Extensions/NVDAGK100HalWeb.kext/Contents/MacOS/NVDAGK100HalWeb"
};

// NVDAGM100HalWeb.kext - from web driver, for Maxwell
static const char *pathGMWeb[] = {
	"/Library/Extensions/NVDAGM100HalWeb.kext/Contents/MacOS/NVDAGM100HalWeb",
	"/System/Library/Extensions/NVDAGM100HalWeb.kext/Contents/MacOS/NVDAGM100HalWeb"
};

// NVDAGP100HalWeb.kext - from web driver, for Pascal
static const char *pathGPWeb[] = {
	"/Library/Extensions/NVDAGP100HalWeb.kext/Contents/MacOS/NVDAGP100HalWeb",
	"/System/Library/Extensions/NVDAGP100HalWeb.kext/Contents/MacOS/NVDAGP100HalWeb"
};

static KernelPatcher::KextInfo kextList[] {
	{ "com.apple.nvidia.driver.NVDAGK100Hal", pathGKHal, arrsize(pathGKHal), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.web.NVDAGK100HalWeb", pathGKWeb, arrsize(pathGKWeb), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.web.NVDAGM100HalWeb", pathGMWeb, arrsize(pathGMWeb), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.web.NVDAGP100HalWeb", pathGPWeb, arrsize(pathGPWeb), {}, {}, KernelPatcher::KextInfo::Unloaded },
};

enum : size_t {
	KextGK100HalSys,
	KextGK100HalWeb,
	KextGM100HalWeb,
	KextGP100HalWeb
};

// target framework for 10.10.x and 10.11.x
static const char *binaryIOKitFramework = "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit";
// target framework as of 10.12.x
static const char *binaryCoreDisplayFramework = "/System/Library/Frameworks/CoreDisplay.framework/Versions/A/CoreDisplay";
// accompanied process for 10.10.x or 10.11.x
static const char *procWindowServerOld = "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/Resources/WindowServer";
static uint32_t procWindowServerOldLen = sizeof("/System/Library/Frameworks/CoreGraphics.framework/Versions/A/Resources/WindowServer") - 1;
// accompanied process for 10.12.x
static const char *procWindowServerNew = "/System/Library/PrivateFrameworks/SkyLight.framework/Versions/A/Resources/WindowServer";
static uint32_t procWindowServerNewLen = sizeof("/System/Library/PrivateFrameworks/SkyLight.framework/Versions/A/Resources/WindowServer") - 1;

enum : uint32_t {
	SectionYosEC   = 1,
	SectionSieHS   = 2,
	SectionHS1034  = 3
};

// Patches
//
// for NVDAGK100Hal and NVDAGK100HalWeb
//
// Reference:
// https://github.com/Floris497/mac-pixel-clock-patch-V2/blob/master/NVIDIA-patcher.command
//
static const uint8_t gk100Find[] = { 0x88, 0x84, 0x02, 0x00 };
static const uint8_t gk100Repl[] = { 0x80, 0x1A, 0x06, 0x00 };
//
// for NVDAGM100HalWeb and NVDAGP100HalWeb
//
// Reference:
// https://github.com/Floris497/mac-pixel-clock-patch-V2/blob/master/NVIDIA-WEB-MAXWELL-patcher.command
//
static const uint8_t gmp100Find[] = { 0x88, 0x84, 0x02, 0x00 };
static const uint8_t gmp100Repl[] = { 0x40, 0x42, 0x0F, 0x00 };
//
// for frameworks
//
// Reference:
// https://github.com/Floris497/mac-pixel-clock-patch-V2/blob/master/CoreDisplay-patcher.command
//
// Modified by PMheart (jmpq adress optimisations)
//
static const uint8_t frameworkOldFind[] {
	0xB8, 0x01, 0x00, 0x00, 0x00,                   // mov  eax, 0x1
	0xF6, 0xC1, 0x01,                               // test cl, 0x1
	0x0F, 0x85                                      // jne  "somewhere"  ; Don't care for the exact offset!
};

static const uint8_t frameworkOldRepl[] {
	0x33, 0xC0,                                     // xor eax, eax      ; 0
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,       // nop (7x)          ; placeholders
	0xE9                                            // jmp "somewhere"   ; Don't care for the exact offset!
};

// 10.13.4+
static const uint8_t frameworkNewFind[] {
	0xBB, 0x01, 0x00, 0x00, 0x00,                   // mov ebx, 0x1
	0xA8, 0x01,                                     // test al, 0x1
	0x0F, 0x85                                      // jne <somewhere>
};

static const uint8_t frameworkNewRepl[] {
	0x31, 0xDB,                                     // xor ebx, ebx
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90,             // nop (6x)
	0xE9                                            // jmp <somewhere>
};

static UserPatcher::BinaryModPatch frameworkPatchOld {
	CPU_TYPE_X86_64,
	frameworkOldFind,
	frameworkOldRepl,
	arrsize(frameworkOldFind),
	0,            // skip  = 0 -> replace all occurrences
	1,            // count = 1 -> 1 set of hex inside the target binaries
	UserPatcher::FileSegment::SegmentTextText,
	SectionHS1034 // 10.10.x till 10.13.3 (all universal)
};

static UserPatcher::BinaryModPatch frameworkPatchNew {
	CPU_TYPE_X86_64,
	frameworkNewFind,
	frameworkNewRepl,
	arrsize(frameworkNewFind),
	0,            // skip  = 0 -> replace all occurrences
	1,            // count = 1 -> 1 set of hex inside the target binaries
	UserPatcher::FileSegment::SegmentTextText,
	SectionHS1034 // 10.13.4+
};

// 10.10.x and 10.11.x
static UserPatcher::BinaryModInfo binaryModYosEC { binaryIOKitFramework, &frameworkPatchOld, 1 };
// 10.12.x and 10.13.0-10.13.3
static UserPatcher::BinaryModInfo binaryModSieHS { binaryCoreDisplayFramework, &frameworkPatchOld, 1 };
// 10.13.4+
static UserPatcher::BinaryModInfo binaryModHS1034 { binaryCoreDisplayFramework, &frameworkPatchNew, 1 };

// 10.10.x and 10.11.x
static UserPatcher::ProcInfo procInfoYosEC { procWindowServerOld, procWindowServerOldLen, SectionYosEC };
// 10.12.x and 10.13.x
static UserPatcher::ProcInfo procInfoSieHS { procWindowServerNew, procWindowServerNewLen, SectionSieHS };

CDF *CDF::callbackCDF;

void CDF::init() {
	disableHDMI20 = checkKernelArgument("-cdfoff");
	if (disableHDMI20) {
		SYSLOG("cdf", "disabling HDMI 2.0 unlock patches by argument");
		return;
	}

	callbackCDF = this;
	lilu.onKextLoadForce(kextList, arrsize(kextList));

	if (getKernelVersion() == KernelVersion::Yosemite || getKernelVersion() == KernelVersion::ElCapitan) {
		// 10.10, 10.11
		currentProcInfo = &procInfoYosEC;
		currentModInfo = &binaryModYosEC;
	} else if (getKernelVersion() == KernelVersion::Sierra || (getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() < 5)) {
		// 10.12, 10.13.0-10.13.3
		currentProcInfo = &procInfoSieHS;
		currentModInfo = &binaryModSieHS;
	} else if ((getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() >= 5) || getKernelVersion() == KernelVersion::Mojave) {
		// 10.13.4+
		currentProcInfo = &procInfoSieHS;
		currentModInfo = &binaryModHS1034;
	}

	if (currentProcInfo && currentModInfo)
		lilu.onProcLoadForce(currentProcInfo, 1, nullptr, nullptr, currentModInfo, 1);
}

void CDF::deinit() {

}

void CDF::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	if (disableHDMI20)
		return;

	bool hasNVIDIA = false;
	for (size_t i = 0; i < info->videoExternal.size(); i++) {
		if (info->videoExternal[i].vendor == WIOKit::VendorID::NVIDIA) {
			hasNVIDIA = true;
			break;
		}
	}

	if (!hasNVIDIA) {
		for (size_t i = 0; i < arrsize(kextList); i++)
			kextList[i].switchOff();
	}

	if (!hasNVIDIA && !info->videoBuiltin && currentProcInfo && currentModInfo) {
		currentProcInfo->section = UserPatcher::ProcInfo::SectionDisabled;
		for (size_t i = 0; i < currentModInfo->count; i++)
			currentModInfo->patches[i].section = UserPatcher::ProcInfo::SectionDisabled;
	}
}

bool CDF::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (disableHDMI20)
		return false;

	if (kextList[KextGK100HalSys].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGK100HalSys], gk100Find, gk100Repl, sizeof(gk100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gk100 patch %d", patcher.getError());
			patcher.clearError();
		}
		return true;
	}

	if (kextList[KextGK100HalWeb].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGK100HalWeb], gk100Find, gk100Repl, sizeof(gk100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gk100 web patch %d", patcher.getError());
			patcher.clearError();
		}
		return true;
	}

	if (kextList[KextGM100HalWeb].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGM100HalWeb], gmp100Find, gmp100Repl, sizeof(gmp100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gm100 web patch %d", patcher.getError());
			patcher.clearError();
		}
		return true;
	}

	if (kextList[KextGP100HalWeb].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGP100HalWeb], gmp100Find, gmp100Repl, sizeof(gmp100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gp100 web patch %d", patcher.getError());
			patcher.clearError();
		}
		return true;
	}

	return false;
}

