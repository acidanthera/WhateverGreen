//
//  kern_ngfx.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_ngfx.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>

#include <sys/types.h>
#include <sys/sysctl.h>

static const char *pathGeForce[] {
	"/System/Library/Extensions/GeForce.kext/Contents/MacOS/GeForce"
};

static const char *pathGeForceWeb[] {
	"/Library/Extensions/GeForceWeb.kext/Contents/MacOS/GeForceWeb",
	"/System/Library/Extensions/GeForceWeb.kext/Contents/MacOS/GeForceWeb"
};

static const char *pathNVDAStartupWeb[] {
	"/Library/Extensions/NVDAStartupWeb.kext/Contents/MacOS/NVDAStartupWeb",
	"/System/Library/Extensions/NVDAStartupWeb.kext/Contents/MacOS/NVDAStartupWeb"
};

static KernelPatcher::KextInfo kextList[] {
	{ "com.apple.GeForce", pathGeForce, arrsize(pathGeForce), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.web.GeForceWeb", pathGeForceWeb, arrsize(pathGeForceWeb), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.NVDAStartupWeb", pathNVDAStartupWeb, arrsize(pathNVDAStartupWeb), {}, {}, KernelPatcher::KextInfo::Unloaded }
};

enum KextIndex {
	IndexGeForce,
	IndexGeForceWeb,
	IndexNVDAStartupWeb
};

NGFX *NGFX::callbackNGFX;

void NGFX::init() {
	callbackNGFX = this;

	PE_parse_boot_argn("ngfxcompat", &forceDriverCompatibility, sizeof(forceDriverCompatibility));
	disableTeamUnrestrict = checkKernelArgument("-ngfxlibvalfix");

	lilu.onKextLoadForce(kextList, arrsize(kextList));
}

void NGFX::deinit() {

}

void NGFX::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	bool hasNVIDIA = false;
	for (size_t i = 0; i < info->videoExternal.size(); i++) {
		if (info->videoExternal[i].vendor == WIOKit::VendorID::NVIDIA) {
			hasNVIDIA = true;
			if (info->videoExternal[i].video->getProperty("disable-gfx-submit"))
				fifoSubmit = 0;
			break;
		}
	}

	if (hasNVIDIA) {
		if (getKernelVersion() > KernelVersion::Mavericks && getKernelVersion() < KernelVersion::HighSierra) {
			if (!disableTeamUnrestrict) {
				orgCsfgGetTeamId = reinterpret_cast<decltype(orgCsfgGetTeamId)>(patcher.solveSymbol(KernelPatcher::KernelID, "_csfg_get_teamid"));
				if (orgCsfgGetTeamId) {
					DBGLOG("ngfx", "obtained csfg_get_teamid");
					KernelPatcher::RouteRequest request("_csfg_get_platform_binary", wrapCsfgGetPlatformBinary, orgCsfgGetPlatformBinary);
					patcher.routeMultiple(KernelPatcher::KernelID, &request, 1);
				} else {
					SYSLOG("ngfx", "failed to resolve csfg_get_teamid");
					patcher.clearError();
				}
			}
		}
	} else {
		for (size_t i = 0; i < arrsize(kextList); i++)
			kextList[i].switchOff();
	}
}

bool NGFX::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (kextList[IndexGeForce].loadIndex == index) {
		KernelPatcher::RouteRequest request("__ZN13nvAccelerator18SetAccelPropertiesEv", wrapSetAccelProperties, orgSetAccelProperties);
		patcher.routeMultiple(index, &request, 1, address, size);
		restoreLegacyOptimisations(patcher, index, address, size);
		return true;
	}

	if (kextList[IndexGeForceWeb].loadIndex == index) {
		KernelPatcher::RouteRequest request("__ZN19nvAcceleratorParent18SetAccelPropertiesEv", wrapSetAccelProperties, orgSetAccelProperties);
		patcher.routeMultiple(index, &request, 1, address, size);
		return true;
	}

	if (kextList[IndexNVDAStartupWeb].loadIndex == index) {
		KernelPatcher::RouteRequest request("__ZN14NVDAStartupWeb5probeEP9IOServicePi", wrapStartupWebProbe, orgStartupWebProbe);
		patcher.routeMultiple(index, &request, 1, address, size);
		return true;
	}

	return false;
}

void NGFX::restoreLegacyOptimisations(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (getKernelVersion() < KernelVersion::HighSierra || getKernelVersion() > KernelVersion::Mojave) {
		DBGLOG("ngfx", "not bothering vaddr presubmit performance fix on pre-10.13 and past 10.14");
		return;
	}

	PE_parse_boot_argn("ngfxsubmit", &fifoSubmit, sizeof(fifoSubmit));
	DBGLOG("ngfx", "read legacy fifo submit as %d", fifoSubmit);

	if (fifoSubmit == 0) {
		DBGLOG("ngfx", "vaddr presubmit performance fix was disabled manually");
		return;
	}

	orgFifoPrepare = patcher.solveSymbol<decltype(orgFifoPrepare)>(index, "__ZN15nvGpFifoChannel7PrepareEv", address, size);
	if (orgFifoPrepare) {
		DBGLOG("ngfx", "obtained nvGpFifoChannel::Prepare");
	} else {
		DBGLOG("ngfx", "failed to resolve nvGpFifoChannel::Prepare");
		patcher.clearError();
	}

	orgFifoComplete = patcher.solveSymbol<decltype(orgFifoComplete)>(index, "__ZN15nvGpFifoChannel8CompleteEv", address, size);
	if (orgFifoComplete) {
		DBGLOG("ngfx", "obtained nvGpFifoChannel::Complete");
	} else {
		DBGLOG("ngfx", "failed to resolve nvGpFifoChannel::Complete");
		patcher.clearError();
	}

	if (orgFifoPrepare && orgFifoComplete) {
		mach_vm_address_t presubmitBase = 0;

		// Firstly we need to recover the PreSubmit function, which was badly broken.
		auto presubmit = patcher.solveSymbol(index, "__ZN21nvVirtualAddressSpace9PreSubmitEv", address, size);
		if (presubmit) {
			DBGLOG("ngfx", "obtained nvVirtualAddressSpace::PreSubmit");
			// Here we patch the prologue to signal that this call to PreSubmit is not coming from patched areas.
			// The original prologue is executed in orgSubmitHandler, make sure you update it there!!!
			uint8_t prologue[] {0x55, 0x48, 0x89, 0xE5};
			uint8_t uprologue[] {0xB0, 0x00, 0x90, 0x90};
			if (!memcmp(reinterpret_cast<void *>(presubmit), prologue, sizeof(prologue))) {
				patcher.routeBlock(presubmit, uprologue, sizeof(uprologue));
				if (patcher.getError() == KernelPatcher::Error::NoError)
					orgVaddrPreSubmit = reinterpret_cast<decltype(orgVaddrPreSubmit)>(patcher.routeFunction(presubmit + sizeof(prologue),
						reinterpret_cast<mach_vm_address_t>(wrapVaddrPreSubmitTrampoline), true));
				if (patcher.getError() == KernelPatcher::Error::NoError) {
					presubmitBase = presubmit + sizeof(prologue);
					DBGLOG("ngfx", "routed nvVirtualAddressSpace::PreSubmit");
				} else {
					SYSLOG("ngfx", "failed to route nvVirtualAddressSpace::PreSubmit");
					patcher.clearError();
				}
			} else {
				SYSLOG("ngfx", "prologue mismatch in nvVirtualAddressSpace::PreSubmit");
			}
		} else {
			SYSLOG("ngfx", "failed to resolve nvVirtualAddressSpace::PreSubmit");
			patcher.clearError();
		}

		// Then we have to recover the calls to the PreSubmit function, which were removed.
		if (orgVaddrPreSubmit && presubmitBase) {
			const char *symbols[] {
				"__ZN21nvVirtualAddressSpace12MapMemoryDmaEP11nvSysMemoryP11nvMemoryMapP18nvPageTableMappingj",
				"__ZN21nvVirtualAddressSpace12MapMemoryDmaEP16__GLNVsurfaceRecjjyj",
				"__ZN21nvVirtualAddressSpace12MapMemoryDmaEyyPK14MMU_MAP_TARGET",
				"__ZN21nvVirtualAddressSpace14UnmapMemoryDmaEP11nvSysMemoryP11nvMemoryMapP18nvPageTableMappingj",
				"__ZN21nvVirtualAddressSpace14UnmapMemoryDmaEP16__GLNVsurfaceRecjjyj",
				"__ZN21nvVirtualAddressSpace14UnmapMemoryDmaEyy"
			};

			uint8_t seqRbx[] {0xC6, 0x83, 0x7C, 0x03, 0x00, 0x00, 0x00};
			uint8_t seqR13[] {0x41, 0xC6, 0x85, 0x7C, 0x03, 0x00, 0x00, 0x00};
			uint8_t seqR12[] {0x41, 0xC6, 0x84, 0x24, 0x7C, 0x03, 0x00, 0x00, 0x00};

			uint8_t repRbx[] {0xB0, 0x01, 0xE8, 0x00, 0x00, 0x00, 0x00};
			uint8_t repR13[] {0xB0, 0x02, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x90};
			uint8_t repR12[] {0xB0, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90};

			size_t dispOff = 3;
			// Pick something reasonably high to ensure the sequence is found.
			size_t maxLookup = 0x1000;

			struct {
				uint8_t *patch;
				uint8_t *code;
				size_t sz;
			} patches[] {
				{seqRbx, repRbx, sizeof(seqRbx)},
				{seqR13, repR13, sizeof(seqR13)},
				{seqR12, repR12, sizeof(seqR12)}
			};

			for (auto &sym : symbols) {
				auto addr = patcher.solveSymbol(index, sym, address, size);
				if (addr) {
					DBGLOG("ngfx", "obtained %s", sym);

					for (size_t off = 0; off < maxLookup; off++) {
						for (auto &patch : patches) {
							if (!memcmp(reinterpret_cast<uint8_t *>(addr+off), patch.patch, patch.sz)) {
								// Calculate the jump offset
								auto disp = static_cast<int32_t>(presubmitBase - (addr+off+dispOff + 5));
								DBGLOG("ngfx", "found pattern of %lu bytes at %lu offset, disp %X", patch.sz, off, disp);
								*reinterpret_cast<int32_t *>(patch.code + dispOff) = disp;
								patcher.routeBlock(addr+off, patch.code, patch.sz);
								if (patcher.getError() == KernelPatcher::Error::NoError) {
									DBGLOG("ngfx", "successfully patched %s", sym);
								} else {
									SYSLOG("ngfx", "failed to patch %s", sym);
									patcher.clearError();
								}
								off = maxLookup;
								break;
							}
						}
					}

				} else {
					SYSLOG("ngfx", "failed to obtain %s", sym);
					patcher.clearError();
				}
			}
		}
	}
}

void NGFX::applyAcceleratorProperties(IOService *that) {
	if (!that->getProperty("IOVARendererID")) {
		uint8_t rendererId[] {0x08, 0x00, 0x04, 0x01};
		that->setProperty("IOVARendererID", rendererId, sizeof(rendererId));
		DBGLOG("ngfx", "set IOVARendererID to 08 00 04 01");
	}

	if (!that->getProperty("IOVARendererSubID")) {
		uint8_t rendererSubId[] {0x03, 0x00, 0x00, 0x00};
		that->setProperty("IOVARendererSubID", rendererSubId, sizeof(rendererSubId));
		DBGLOG("ngfx", "set IOVARendererSubID to value 03 00 00 00");
	}

	auto gfx = that->getParentEntry(gIOServicePlane);
	int gl = gfx && gfx->getProperty("disable-metal");
	PE_parse_boot_argn("ngfxgl", &gl, sizeof(gl));

	if (gl) {
		DBGLOG("ngfx", "disabling metal support");
		that->removeProperty("MetalPluginClassName");
		that->removeProperty("MetalPluginName");
		that->removeProperty("MetalStatisticsName");
	}
}

int NGFX::wrapCsfgGetPlatformBinary(void *fg) {
	//DBGLOG("ngfx", "csfg_get_platform_binary is called"); // is called quite often

	int result = FunctionCast(wrapCsfgGetPlatformBinary, callbackNGFX->orgCsfgGetPlatformBinary)(fg);
	if (!result) {
		// Special case NVIDIA drivers
		const char *teamId = callbackNGFX->orgCsfgGetTeamId(fg);
		if (teamId && !strcmp(teamId, NvidiaTeamId)) {
			DBGLOG("ngfx", "platform binary override for %s", NvidiaTeamId);
			return 1;
		}
	}

	return result;
}

bool NGFX::wrapVaddrPreSubmit(void *that) {
	bool r = orgVaddrPresubmitTrampoline(that);

	if (that && r) {
		getMember<uint8_t>(that, 0x37D) = 1;
		auto fifo = getMember<void *>(that, 0x2B0);
		if (callbackNGFX->orgFifoPrepare(fifo)) {
			auto fifovt = getMember<void *>(fifo, 0);
			// Calls to nvGpFifoChannel::PreSubmit
			auto fifopresubmit = getMember<t_fifoPreSubmit>(fifovt, 0x1B0);
			if (fifopresubmit(fifo, 0x40000, 0, 0, 0, 0, 0, 0)) {
				getMember<uint16_t>(that, 0x37C) = 1;
				return true;
			}

			callbackNGFX->orgFifoPrepare(fifo);
			return false;
		}
	}

	return r;
}

void NGFX::wrapSetAccelProperties(IOService* that) {
	DBGLOG("ngfx", "nvAccelerator::SetAccelProperties is called");
	FunctionCast(wrapSetAccelProperties, callbackNGFX->orgSetAccelProperties)(that);
	callbackNGFX->applyAcceleratorProperties(that);
}

void NGFX::wrapSetAccelPropertiesWeb(IOService* that) {
	DBGLOG("ngfx", "nvAcceleratorParent::SetAccelProperties is called");
	FunctionCast(wrapSetAccelProperties, callbackNGFX->orgSetAccelPropertiesWeb)(that);
	callbackNGFX->applyAcceleratorProperties(that);
}

IOService *NGFX::wrapStartupWebProbe(IOService *that, IOService *provider, SInt32 *score) {
	DBGLOG("ngfx", "NVDAStartupWeb::probe is called");

	int comp = callbackNGFX->forceDriverCompatibility;
	if (comp < 0)
		comp = provider && provider->getProperty("force-compat");

	if (comp > 0) {
		char osversion[40] = {};
		size_t size = sizeof(osversion);
		sysctlbyname("kern.osversion", osversion, &size, NULL, 0);
		DBGLOG("ngfx", "ignoring driver compatibility requirements with %s OS", osversion);
		that->setProperty("NVDARequiredOS", osversion);
	}

	return FunctionCast(wrapStartupWebProbe, callbackNGFX->orgStartupWebProbe)(that, provider, score);
}
