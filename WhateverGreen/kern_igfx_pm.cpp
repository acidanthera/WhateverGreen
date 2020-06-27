//
//  kern_igfx_pm.cpp
//  WhateverGreen
//
//  Created by Pb on 22/06/2020.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include <Headers/kern_atomic.hpp>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_disasm.hpp>
#include <Library/LegacyIOService.h>
#include "kern_igfx.hpp"

namespace {
constexpr const char* log = "igfx_pm";

// For debugging
struct [[gnu::packed]] IGHwCsDesc {
  char type;
  char gap[4];
  char *title;
  char unk0[48];
  char unk1[12];
};

static bool patchRCSCheck(mach_vm_address_t& start) {
	constexpr unsigned ninsts_max {256};
	
	hde64s dis;
	
	bool found_cmp = false;
	bool found_jmp = false;

	for (size_t i = 0; i < ninsts_max; i++) {
		auto sz = Disassembler::hdeDisasm(start, &dis);

		if (dis.flags & F_ERROR) {
			SYSLOG(log, "Error disassembling submitExecList");
			break;
		}

		/* cmp byte ptr [rcx], 0 */
		if (!found_cmp && dis.opcode == 0x80 && dis.modrm_reg == 7 && dis.modrm_rm == 1)
			found_cmp = true;
		/* jnz rel32 */
		if (found_cmp && dis.opcode == 0x0f && dis.opcode2 == 0x85) {
			found_jmp = true;
			break;
		}

		start += sz;
	}
	
	if (found_jmp) {
		auto status = MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock);
		if (status == KERN_SUCCESS) {
			constexpr uint8_t nop6[] {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
			lilu_os_memcpy(reinterpret_cast<void*>(start), nop6, arrsize(nop6));
			MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
			DBGLOG(log, "Patched submitExecList");
			return true;
		} else {
			DBGLOG(log, "Failed to set kernel writing");
			return false;
		}
	} else {
		SYSLOG(log, "jnz in submitExecList not found");
		return false;
	}
}

constexpr uint32_t MCHBAR_MIRROR_BASE_SNB = 0x140000;
constexpr uint32_t GEN6_RP_STATE_CAP = MCHBAR_MIRROR_BASE_SNB + 0x5998;

constexpr uint32_t GEN9_FREQUENCY_SHIFT = 23;
constexpr uint32_t GEN9_FREQ_SCALER  = 3;
}

void IGFX::RPSControl::initGraphics(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	mach_vm_address_t orgIGHardwareCommandStreamer2__submitExecList {};
	const char* sym = getKernelVersion() >= KernelVersion::Catalina ?
	"__ZN26IGHardwareCommandStreamer514submitExecListEj" : "__ZN26IGHardwareCommandStreamer214submitExecListEj";
	orgIGHardwareCommandStreamer2__submitExecList = patcher.solveSymbol(index, sym, address, size);

	/**
	 * IGHardwareCommandStreamer2::submitExecList only controls RPS for RCS type streamers.
	 * Patch it to enable control for any kind of streamer.
	 */
	if (orgIGHardwareCommandStreamer2__submitExecList) {
		mach_vm_address_t start = orgIGHardwareCommandStreamer2__submitExecList;
		patchRCSCheck(start);
		// The second patch is to get to patchFrequencyRequest (unused for now)
//		patchRCSCheck(start);
	} else {
		SYSLOG(log, "Failed to solve submitExecList (%d)", patcher.getError());
		patcher.clearError();
	}
}

/**
 * Request maximum RPS at exec list submission.
 * While this sounds dangerous, there appears to be a secondary mechanism
 * that downclocks the GPU rather quickly back.
 * Using any lower RPS lets that mechanism win the race.
 */
int IGFX::RPSControl::pmNotifyWrapper(unsigned int a0,unsigned int a1,unsigned long long * a2,unsigned int * freq) {
	uint32_t cfreq = 0;

	FunctionCast(IGFX::RPSControl::pmNotifyWrapper, callbackIGFX->RPSControl.orgPmNotifyWrapper)(a0, a1, a2, &cfreq);
	
	if (!callbackIGFX->RPSControl.freq_max) {
		callbackIGFX->RPSControl.freq_max = callbackIGFX->RPSControl.AppleIntelFramebufferController__ReadRegister32(*callbackIGFX->RPSControl.gController, GEN6_RP_STATE_CAP) & 0xff;
		DBGLOG(log, "Read RP0 %d", callbackIGFX->RPSControl.freq_max);
	}
	
	DBGLOG(log, "pmNotifyWrapper sets freq 0x%x", cfreq);
	*freq = (GEN9_FREQ_SCALER << GEN9_FREQUENCY_SHIFT) * callbackIGFX->RPSControl.freq_max;

	return 0;
}

void IGFX::RPSControl::initFB(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	AppleIntelFramebufferController__ReadRegister32 = patcher.solveSymbol<decltype(AppleIntelFramebufferController__ReadRegister32)>(index, "__ZN31AppleIntelFramebufferController14ReadRegister32Em", address, size);
	
	gController = patcher.solveSymbol<decltype(gController)>(index, "_gController", address, size);
	
	KernelPatcher::RouteRequest req {
			"__ZL15pmNotifyWrapperjjPyPj",
			&IGFX::RPSControl::pmNotifyWrapper,
			orgPmNotifyWrapper
	};

	if (!(AppleIntelFramebufferController__ReadRegister32 && gController && patcher.routeMultiple(index, &req, 1, address, size, true, true)))
		SYSLOG(log, "failed to route igfx FB PM functions");
}
