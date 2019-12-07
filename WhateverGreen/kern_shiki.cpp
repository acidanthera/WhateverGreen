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

	if (getKernelVersion() >= KernelVersion::Catalina) {
		procInfo = ADDPR(procInfoModern);
		procInfoSize = ADDPR(procInfoModernSize);
	} else {
		procInfo = ADDPR(procInfoLegacy);
		procInfoSize = ADDPR(procInfoLegacySize);
	}

	lilu.onProcLoadForce(procInfo, procInfoSize, nullptr, nullptr, ADDPR(binaryMod), ADDPR(binaryModSize));
}

void SHIKI::deinit() {

}

void SHIKI::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	if (disableShiki)
		return;

	bool forceOnlineRenderer     = false;
	bool allowNonBGRA            = false;
	bool forceCompatibleRenderer = false;
	bool addExecutableWhitelist  = false;
	bool replaceBoardID          = false;
	bool useHwDrmStreaming       = false;
	bool useHwDrmDecoder         = false;
	bool useLegacyHwDrmDecoder   = false;

	cpuGeneration = CPUInfo::getGeneration();

	auto getBootArgument = [](DeviceInfo *info, const char *name, void *bootarg, int size) {
		if (PE_parse_boot_argn(name, bootarg, size))
			return true;

		for (size_t i = 0; i < info->videoExternal.size(); i++) {
			auto prop = OSDynamicCast(OSData, info->videoExternal[i].video->getProperty(name));
			auto propSize = prop ? prop->getLength() : 0;
			if (propSize > 0 && propSize <= size) {
				lilu_os_memcpy(bootarg, prop->getBytesNoCopy(), propSize);
				memset(static_cast<uint8_t *>(bootarg) + propSize, 0, size - propSize);
				return true;
			}
		}

		if (info->videoBuiltin) {
			auto prop = OSDynamicCast(OSData, info->videoBuiltin->getProperty(name));
			auto propSize = prop ? prop->getLength() : 0;
			if (propSize > 0 && propSize <= size) {
				lilu_os_memcpy(bootarg, prop->getBytesNoCopy(), propSize);
				memset(static_cast<uint8_t *>(bootarg) + propSize, 0, size - propSize);
				return true;
			}
		}

		return false;
	};

	int bootarg {0};
	if (getBootArgument(info, "shikigva", &bootarg, sizeof(bootarg))) {
		forceOnlineRenderer     = bootarg & ForceOnlineRenderer;
		allowNonBGRA            = bootarg & AllowNonBGRA;
		forceCompatibleRenderer = bootarg & ForceCompatibleRenderer;
		addExecutableWhitelist  = bootarg & AddExecutableWhitelist;
		useHwDrmDecoder         = bootarg & UseHwDrmDecoder;
		replaceBoardID          = bootarg & ReplaceBoardID;
		useHwDrmStreaming       = bootarg & UseHwDrmStreaming;
		useLegacyHwDrmDecoder   = bootarg & UseLegacyHwDrmDecoder;

		if (useHwDrmDecoder && (replaceBoardID || addExecutableWhitelist))
			PANIC("shiki", "Hardware DRM decoder cannot be used with custom board or whitelist");
	} else {
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

		// FairPlay 1.0 DRM is just fine on Apple hardware, as the only reason for it to break is IGPU presence.
		// For QuickTime movie playback along with TV+ on MacPro5,1 use one of the following:
		// - OpenCore spoof to iMacPro1,1 (preferred).
		// - shikigva=160 shiki-id=Mac-7BA5B2D9E42DDD94 without OpenCore.
		useLegacyHwDrmDecoder = info->firmwareVendor == DeviceInfo::FirmwareVendor::Apple;

		DBGLOG("shiki", "will autodetect autodetect GPU %d whitelist %d", autodetectGFX, addExecutableWhitelist);
	}

	DBGLOG("shiki", "pre-config: online %d, bgra %d, compat %d, whitelist %d, id %d, stream %d, hwdrm %d",
		   forceOnlineRenderer, allowNonBGRA, forceCompatibleRenderer, addExecutableWhitelist, replaceBoardID,
		   useHwDrmStreaming, useHwDrmDecoder);

	// Disable hardware decoder patches when unused
	if (!useHwDrmDecoder) {
		disableSection(SectionHWDRMID);
		disableSection(SectionLEGACYHWDRMID);
	} else if (cpuGeneration >= CPUInfo::CpuGeneration::Haswell) {
		// Only Ivy bridge CPUs need LSKD/LSKDMSE CPUID upgrade.
		disableSection(SectionLEGACYHWDRMID);
	}

	// We do not need NDRMI patches when legacy hardware decoder works.
	if (useLegacyHwDrmDecoder) {
		disableSection(SectionNDRMI);
		disableSection(SectionFCPUID);
	}

	// TV+ is slightly different from the usual FP10, as it cannot be forced not to use hardware decoding through VideoToolBox patch.
	if (useLegacyHwDrmDecoder || useHwDrmDecoder)
		disableSection(SectionLEGACYSWDRMID);

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
		if (getBootArgument(info, "shiki-id", customBoardID, sizeof(customBoardID)))
			customBoardID[sizeof(customBoardID)-1] = '\0';
		else
			snprintf(customBoardID, sizeof(customBoardID), "Mac-27ADBB7B4CEE8E61"); // iMac14,2
		DBGLOG("shiki", "requesting %s board-id for gva", customBoardID);
	} else {
		disableSection(SectionBOARDID);
	}

	if (!useHwDrmStreaming) {
		disableSection(SectionNSTREAM);
	} else if (useHwDrmStreaming && useHwDrmDecoder) {
		// Let's look the patch up first.
		UserPatcher::BinaryModPatch *hwDrmPatch {nullptr};
		for (size_t i = 0; i < ADDPR(binaryModSize); i++) {
			auto patches = ADDPR(binaryMod)[i].patches;
			for (size_t j = 0; j < ADDPR(binaryMod)[i].count; j++) {
				if (patches[j].section == SectionHWDRMID) {
					hwDrmPatch = &patches[j];
					DBGLOG("shiki", "found hwdrm-id at %lu:%lu with size %lu", i, j, hwDrmPatch->size);
					break;
				}
			}
		}

		if (hwDrmPatch) {
			static uint8_t iMacProBoardId[21] = {"Mac-7BA5B2D9E42DDD94"};
			static uint8_t selfBoardId[21] = {};

			if (WIOKit::getComputerInfo(nullptr, 0, reinterpret_cast<char *>(selfBoardId), sizeof(selfBoardId)) && selfBoardId[0] != '\0') {
				hwDrmPatch->find = iMacProBoardId;
				hwDrmPatch->replace = selfBoardId;
				hwDrmPatch->size = sizeof(selfBoardId);
				DBGLOG("shiki", "using partial hwdrm-id patch from %s to %s", reinterpret_cast<char *>(iMacProBoardId), reinterpret_cast<char *>(selfBoardId));
			}
		} else {
			SYSLOG("shiki", "no hwdrm-id patch found");
		}
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

	if (customBoardID[0] || useHwDrmDecoder) {
		auto entry = IORegistryEntry::fromPath("/", gIODTPlane);
		if (entry) {
			if (customBoardID[0]) {
				DBGLOG("shiki", "changing shiki-id to %s", customBoardID);
				entry->setProperty("shiki-id", customBoardID, static_cast<uint32_t>(strlen(customBoardID)+1));
			}
			if (useHwDrmDecoder) {
				DBGLOG("shiki", "setting hwdrm-id to iMacPro1,1");
				entry->setProperty("hwdrm-id", const_cast<char *>("Mac-7BA5B2D9E42DDD94"), static_cast<uint32_t>(sizeof("Mac-7BA5B2D9E42DDD94")));
			}
			entry->release();
		} else {
			SYSLOG("shiki", "failed to obtain iodt tree");
		}
	}
}

void SHIKI::disableSection(uint32_t section) {
	for (size_t i = 0; i < procInfoSize; i++) {
		if (procInfo[i].section == section)
			procInfo[i].section = SectionUnused;
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
