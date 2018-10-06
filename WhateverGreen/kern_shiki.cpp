//
//  kern_shiki.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_shiki.hpp"

#include <Library/LegacyIOService.h>
#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_file.hpp>
#include <Headers/kern_iokit.hpp>
#include <IOKit/IODeviceTreeSupport.h>

#include "kern_resources.hpp"

void SHIKI::init() {
	disableShiki = !(lilu.getRunMode() & LiluAPI::RunningNormal);
	disableShiki |= checkKernelArgument("-shikioff");

	if (disableShiki)
		return;

	bool forceOnlineRenderer     = false;
	bool allowNonBGRA            = false;
	bool forceCompatibleRenderer = false;
	bool addExecutableWhitelist  = false;
	bool replaceBoardID          = false;
	bool unlockFP10Streaming     = false;

	cpuGeneration = CPUInfo::getGeneration();

	int bootarg {0};
	if (PE_parse_boot_argn("shikigva", &bootarg, sizeof(bootarg))) {
		forceOnlineRenderer     = bootarg & ForceOnlineRenderer;
		allowNonBGRA            = bootarg & AllowNonBGRA;
		forceCompatibleRenderer = bootarg & ForceCompatibleRenderer;
		addExecutableWhitelist  = bootarg & AddExecutableWhitelist;
		replaceBoardID          = bootarg & ReplaceBoardID;
		unlockFP10Streaming     = bootarg & UnlockFP10Streaming;
	} else {
		if (PE_parse_boot_argn("-shikigva", &bootarg, sizeof(bootarg))) {
			SYSLOG("shiki", "-shikigva is deprecated use shikigva %d bit instead", ForceOnlineRenderer);
			forceOnlineRenderer = true;
		}

		// Starting with 10.13.4 Apple has fixed AppleGVA to no longer require patching for compatible renderer.
		if ((getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() < 5) ||
			getKernelVersion() < KernelVersion::HighSierra) {
			autodetectGFX = cpuGeneration == CPUInfo::CpuGeneration::SandyBridge ||
			cpuGeneration == CPUInfo::CpuGeneration::IvyBridge ||
			cpuGeneration == CPUInfo::CpuGeneration::Broadwell ||
			cpuGeneration == CPUInfo::CpuGeneration::Skylake ||
			cpuGeneration == CPUInfo::CpuGeneration::KabyLake;

			if (autodetectGFX) {
				forceCompatibleRenderer = true;
				addExecutableWhitelist = getKernelVersion() >= KernelVersion::Sierra;
			}
		}

		DBGLOG("shiki", "will autodetect autodetect GPU %d whitelist %d", autodetectGFX, addExecutableWhitelist);
	}

	if (PE_parse_boot_argn("-shikifps", &bootarg, sizeof(bootarg))) {
		SYSLOG("shiki", "-shikifps is deprecated use shikigva %d bit instead", UnlockFP10Streaming);
		unlockFP10Streaming = true;
	}

	DBGLOG("shiki", "pre-config: online %d, bgra %d, compat %d, whitelist %d, id %d, stream %d",
		   forceOnlineRenderer, allowNonBGRA, forceCompatibleRenderer, addExecutableWhitelist, replaceBoardID,
		   unlockFP10Streaming);

	// Disable unused sections
	if (!forceOnlineRenderer)
		disableSection(SectionOFFLINE);

	if (!allowNonBGRA)
		disableSection(SectionBGRA);

	// Compatible renderer patch varies depending on the CPU.
	if (!forceCompatibleRenderer || !setCompatibleRendererPatch())
		disableSection(SectionCOMPATRENDERER);

	if (!addExecutableWhitelist)
		disableSection(SectionWHITELIST);

	// Custom board-id may be overridden by a boot-arg
	if (replaceBoardID) {
		if (!PE_parse_boot_argn("shiki-id", customBoardID, sizeof(customBoardID)))
			snprintf(customBoardID, sizeof(customBoardID), "Mac-27ADBB7B4CEE8E61"); // iMac14,2
		DBGLOG("shiki", "requesting %s board-id for gva", customBoardID);
	} else {
		disableSection(SectionBOARDID);
	}

	if (!unlockFP10Streaming)
		disableSection(SectionNSTREAM);

	lilu.onProcLoadForce(ADDPR(procInfo), ADDPR(procInfoSize), nullptr, nullptr, ADDPR(binaryMod), ADDPR(binaryModSize));
}

void SHIKI::deinit() {

}

void SHIKI::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	if (disableShiki)
		return;

	if (info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple) {
		// DRMI is just fine on Apple hardware
		disableSection(SectionNDRMI);
	}

	if (autodetectGFX) {
		bool hasExternalNVIDIA = false;
		bool hasExternalAMD = false;

		for (size_t i = 0; i < info->videoExternal.size(); i++) {
			auto &extGpu = info->videoExternal[i];
			if (extGpu.vendor == WIOKit::VendorID::NVIDIA)
				hasExternalNVIDIA = true;
			else if (extGpu.vendor == WIOKit::VendorID::ATIAMD)
				hasExternalAMD = true;
		}

		bool disableWhitelist = cpuGeneration != CPUInfo::CpuGeneration::SandyBridge;
		bool disableCompatRenderer = true;
		if (hasExternalNVIDIA && (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge ||
								  cpuGeneration == CPUInfo::CpuGeneration::Broadwell ||
								  cpuGeneration == CPUInfo::CpuGeneration::Skylake ||
								  cpuGeneration == CPUInfo::CpuGeneration::KabyLake)) {
			disableCompatRenderer = false;
			disableWhitelist = false;
		} else if (hasExternalAMD && cpuGeneration == CPUInfo::CpuGeneration::IvyBridge) {
			disableCompatRenderer = false;
			disableWhitelist = false;
		}

		if (disableCompatRenderer)
			disableSection(SectionCOMPATRENDERER);

		if (disableWhitelist)
			disableSection(SectionWHITELIST);

		DBGLOG("shiki", "autodetedect decision: whitelist %d compat %d for cpu %d nvidia %d amd %d",
			   !disableWhitelist, !disableCompatRenderer, cpuGeneration, hasExternalNVIDIA, hasExternalAMD);
	}

	if (customBoardID[0]) {
		auto entry = IORegistryEntry::fromPath("/", gIODTPlane);
		if (entry) {
			DBGLOG("shiki", "changing shiki-id to %s", customBoardID);
			auto data = OSData::withBytes(customBoardID, static_cast<uint32_t>(strlen(customBoardID)+1));
			if (data) {
				entry->setProperty("shiki-id", data);
				data->release();
			}
			entry->release();
		} else {
			SYSLOG("shiki", "failed to obtain iodt tree");
		}
	}
}

void SHIKI::disableSection(uint32_t section) {
	for (size_t i = 0; i < ADDPR(procInfoSize); i++) {
		if (ADDPR(procInfo)[i].section == section)
			ADDPR(procInfo)[i].section = SectionUnused;
	}

	for (size_t i = 0; i < ADDPR(binaryModSize); i++) {
		auto patches = ADDPR(binaryMod)[i].patches;
		for (size_t j = 0; j < ADDPR(binaryMod)[i].count; j++) {
			if (patches[j].section == section)
				patches[j].section = SectionUnused;
		}
	}
}

bool SHIKI::setCompatibleRendererPatch() {
	// Support 10.10 and higher for now.
	if (getKernelVersion() < KernelVersion::Yosemite)
		return false;

	// Let's look the patch up first.
	UserPatcher::BinaryModPatch *compPatch {nullptr};
	for (size_t i = 0; i < ADDPR(binaryModSize); i++) {
		auto patches = ADDPR(binaryMod)[i].patches;
		for (size_t j = 0; j < ADDPR(binaryMod)[i].count; j++) {
			if (patches[j].section == SectionCOMPATRENDERER) {
				compPatch = &patches[j];
				DBGLOG("shiki", "found compat renderer patch at %lu:%lu with size %lu", i, j, compPatch->size);
				break;
			}
		}
	}

	if (!compPatch)
		return false;

	// I will be frank, the register could change here. But for a good reason it did not for some time.
	// This patch is much simpler than what we had before, so let's stick to it for the time being.

	// lea eax, [rdx-1080000h]
	static uint8_t yosemitePatchFind[] {0x8D, 0x82, 0x00, 0x00, 0xF8, 0xFE};
	static uint8_t yosemitePatchReplace[] {0x8D, 0x82, 0x00, 0x00, 0xF8, 0xFE};
	static constexpr size_t YosemitePatchOff = 2;

	// lea r10d, [r9-1080000h]
	static uint8_t sierraPatchFind[] {0x45, 0x8D, 0x91, 0x00, 0x00, 0xF8, 0xFE};
	static uint8_t sierraPatchReplace[] {0x45, 0x8D, 0x91, 0x00, 0x00, 0xF8, 0xFE};
	static constexpr size_t SierraPatchOff = 3;

	if (getKernelVersion() >= KernelVersion::Sierra) {
		compPatch->find = sierraPatchFind;
		compPatch->replace = sierraPatchReplace;
		compPatch->size = sizeof(sierraPatchFind);
	} else {
		compPatch->find = yosemitePatchFind;
		compPatch->replace = yosemitePatchReplace;
		compPatch->size = sizeof(yosemitePatchFind);
	}

	// Here are all the currently valid IOVARendererID values:
	// 0x1080000 — Sandy Bridge (AppleIntelFramebuffer)
	// 0x1080001 — Sandy Bridge (Gen6Accelerator)
	// 0x1080002 — Ivy Bridge
	// 0x1080004 — Haswell
	// 0x1080008 — Broadwell
	// 0x1080010 — Skylake
	// 0x1080020 — Kaby Lake
	// 0x1020000 — AMD prior to 5xxx (for example, Radeon HD 2600)
	// 0x1020002 — AMD 5xxx and newer
	// 0x1040002 — NVIDIA VP2
	// 0x1040004 — NVIDIA VP3
	// 0x1040008 — NVIDIA VP4 and newer
	// More details are outlined in https://www.applelife.ru/posts/716793.

	// This patch makes AppleGVA believe that we use Haswell, which is not restricted to any modern GPU
	if (cpuGeneration == CPUInfo::CpuGeneration::SandyBridge) {
		*reinterpret_cast<int32_t *>(&yosemitePatchReplace[YosemitePatchOff]) += (0x1080004 - 0x1080001);
		*reinterpret_cast<int32_t *>(&sierraPatchReplace[SierraPatchOff])     += (0x1080004 - 0x1080001);
		return true;
	} else if (cpuGeneration == CPUInfo::CpuGeneration::IvyBridge) {
		// For whatever reason on GA-Z77-DS3H with i7 3770k and Sapphire Radeon R9 280X attempting to
		// use Haswell-like patch ends in crashes in 10.12.6. Sandy patch works everywhere.
		*reinterpret_cast<int32_t *>(&yosemitePatchReplace[YosemitePatchOff]) += (0x1080000 - 0x1080002);
		*reinterpret_cast<int32_t *>(&sierraPatchReplace[SierraPatchOff])     += (0x1080000 - 0x1080002);
		return true;
	} else if (cpuGeneration == CPUInfo::CpuGeneration::Broadwell) {
		*reinterpret_cast<int32_t *>(&yosemitePatchReplace[YosemitePatchOff]) += (0x1080004 - 0x1080008);
		*reinterpret_cast<int32_t *>(&sierraPatchReplace[SierraPatchOff])     += (0x1080004 - 0x1080008);
		return true;
	} else if (cpuGeneration == CPUInfo::CpuGeneration::Skylake) {
		*reinterpret_cast<int32_t *>(&yosemitePatchReplace[YosemitePatchOff]) += (0x1080004 - 0x1080010);
		*reinterpret_cast<int32_t *>(&sierraPatchReplace[SierraPatchOff])     += (0x1080004 - 0x1080010);
		return true;
	} else if (cpuGeneration == CPUInfo::CpuGeneration::KabyLake) {
		*reinterpret_cast<int32_t *>(&yosemitePatchReplace[YosemitePatchOff]) += (0x1080004 - 0x1080020);
		*reinterpret_cast<int32_t *>(&sierraPatchReplace[SierraPatchOff])     += (0x1080004 - 0x1080020);
		return true;
	}

	return false;
}
