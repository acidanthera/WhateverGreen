//
//  kern_igfx_pm.cpp
//  WhateverGreen
//
//  Created by Pb on 22/06/2020.
//  Copyright © 2020 vit9696. All rights reserved.
//

/*
* Portions Copyright © 2013 Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/

#include <IOKit/IOService.h>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_disasm.hpp>
#include <IOKit/IOLib.h>
#include <IOKit/IOMessage.h>
#include <mach/clock.h>

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

constexpr uint32_t MCHBAR_MIRROR_BASE_SNB = 0x140000;
constexpr uint32_t GEN6_RP_STATE_CAP = MCHBAR_MIRROR_BASE_SNB + 0x5998;

constexpr uint32_t GEN9_FREQUENCY_SHIFT = 23;
constexpr uint32_t GEN9_FREQ_SCALER  = 3;

constexpr uint32_t FORCEWAKE_KERNEL_FALLBACK = 1 << 15;

constexpr uint32_t FORCEWAKE_ACK_TIMEOUT_MS = 50;

constexpr uint32_t FORCEWAKE_MEDIA_GEN9 = 0xa270;
constexpr uint32_t FORCEWAKE_RENDER_GEN9 = 0xa278;
constexpr uint32_t FORCEWAKE_BLITTER_GEN9 = 0xa188;

constexpr uint32_t FORCEWAKE_ACK_MEDIA_GEN9 = 0x0D88;
constexpr uint32_t FORCEWAKE_ACK_RENDER_GEN9 = 0x0D84;
constexpr uint32_t FORCEWAKE_ACK_BLITTER_GEN9 = 0x130044;

enum FORCEWAKE_DOM_BITS : unsigned {
	DOM_RENDER = 0b001,
	DOM_MEDIA = 0b010,
	DOM_BLITTER = 0b100,
	DOM_LAST = DOM_BLITTER,
	DOM_FIRST = DOM_RENDER
};

constexpr uint32_t regForDom(unsigned d) {
	if (d == DOM_RENDER)
		return FORCEWAKE_RENDER_GEN9;
	if (d == DOM_MEDIA)
		return FORCEWAKE_MEDIA_GEN9;
	if (d == DOM_BLITTER)
		return FORCEWAKE_BLITTER_GEN9;
	assertf(false, "Unknown force wake domain %d", d);
	return 0;
}

constexpr uint32_t ackForDom(unsigned d) {
	if (d == DOM_RENDER)
		return FORCEWAKE_ACK_RENDER_GEN9;
	if (d == DOM_MEDIA)
		return FORCEWAKE_ACK_MEDIA_GEN9;
	if (d == DOM_BLITTER)
		return FORCEWAKE_ACK_BLITTER_GEN9;
	assertf(false, "Unknown force wake domain %d", d);
	return 0;
}

constexpr const char* const strForDom(unsigned d) {
	if (d == DOM_RENDER)
		return "Render";
	if (d == DOM_MEDIA)
		return "Media";
	if (d == DOM_BLITTER)
		return "Blitter";
	return "(unk)";
}

constexpr uint32_t masked_field(uint32_t mask, uint32_t value) {
	return (mask << 16) | value;
}

constexpr uint32_t fw_set(uint32_t v) {
	return masked_field(v, v);
}

constexpr uint32_t fw_clear(uint32_t v) {
	return masked_field(v, 0);
}
}

// MARK: - RPS Control Patch

void IGFX::RPSControlPatch::init() {
	// We need to patch both drivers
	requiresPatchingGraphics = true;
	requiresPatchingFramebuffer = true;
	
	// Requires access to global framebuffer controllers
	requiresGlobalFramebufferControllersAccess = true;
	
	// Requires read access to MMIO registers
	requiresMMIORegistersReadAccess = true;
}

void IGFX::RPSControlPatch::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	uint32_t rpsc = 0;
	if (PE_parse_boot_argn("igfxrpsc", &rpsc, sizeof(rpsc)) ||
		WIOKit::getOSDataValue(info->videoBuiltin, "rps-control", rpsc)) {
		enabled = rpsc > 0 && available;
		DBGLOG("weg", "RPS control patch overriden (%u) availabile %d", rpsc, available);
	}
}

void IGFX::RPSControlPatch::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest routeRequest = {
		"__ZL15pmNotifyWrapperjjPyPj",
		wrapPmNotifyWrapper,
		orgPmNotifyWrapper
	};
	
	if (!patcher.routeMultiple(index, &routeRequest, 1, address, size))
		SYSLOG(log, "Failed to route pmNotifyWrapper.");
}

void IGFX::RPSControlPatch::processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	// Address of `IGHardwareCommandStreamer2::submitExecList`
	mach_vm_address_t orgSubmitExecList;
	
	KernelPatcher::SolveRequest request = {
		getKernelVersion() >= KernelVersion::Catalina ? "__ZN26IGHardwareCommandStreamer514submitExecListEj" : "__ZN26IGHardwareCommandStreamer214submitExecListEj",
		orgSubmitExecList
	};
	
	if (!patcher.solveMultiple(index, &request, 1, address, size)) {
		SYSLOG(log, "Failed to solve the symbol for submitExecList.");
		return;
	}
	
	// `submitExecList()` only controls RPS for RCS type streamers
	// Patch it to enable control for any kind of streamer
	if (!patchRCSCheck(orgSubmitExecList))
		SYSLOG(log, "Failed to patch RCS check.");
}

int IGFX::RPSControlPatch::wrapPmNotifyWrapper(unsigned int a0, unsigned int a1, unsigned long long *a2, unsigned int *freq) {
	// Request the maximum RPS at exec list submission
	// While this sounds dangerous, we are still getting proper power management due to force wake clears.
	uint32_t cfreq = 0;
	callbackIGFX->modRPSControlPatch.orgPmNotifyWrapper(a0, a1, a2, &cfreq);
	
	if (!callbackIGFX->modRPSControlPatch.freq_max) {
		callbackIGFX->modRPSControlPatch.freq_max = callbackIGFX->readRegister32(callbackIGFX->defaultController(), GEN6_RP_STATE_CAP) & 0xFF;
		DBGLOG("log", "Read RP0 %d", callbackIGFX->modRPSControlPatch.freq_max);
	}
	
	*freq = (GEN9_FREQ_SCALER << GEN9_FREQUENCY_SHIFT) * callbackIGFX->modRPSControlPatch.freq_max;
	return 0;
}

bool IGFX::RPSControlPatch::patchRCSCheck(mach_vm_address_t& start) {
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

// TODO: DEPRECATED
//void IGFX::RPSControl::initGraphics(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
//	mach_vm_address_t orgIGHardwareCommandStreamer2__submitExecList {};
//	const char* sym = getKernelVersion() >= KernelVersion::Catalina ?
//	"__ZN26IGHardwareCommandStreamer514submitExecListEj" : "__ZN26IGHardwareCommandStreamer214submitExecListEj";
//	orgIGHardwareCommandStreamer2__submitExecList = patcher.solveSymbol(index, sym, address, size);
//
//	/**
//	 * IGHardwareCommandStreamer2::submitExecList only controls RPS for RCS type streamers.
//	 * Patch it to enable control for any kind of streamer.
//	 */
//	if (orgIGHardwareCommandStreamer2__submitExecList) {
//		mach_vm_address_t start = orgIGHardwareCommandStreamer2__submitExecList;
//		//patchRCSCheck(start);
//		// The second patch is to get to patchFrequencyRequest (unused for now)
////		patchRCSCheck(start);
//	} else {
//		SYSLOG(log, "Failed to solve submitExecList (%d)", patcher.getError());
//		patcher.clearError();
//	}
//}
//
//// TODO: DEPRECATED
///**
// * Request maximum RPS at exec list submission.
// * While this sounds dangerous, we are still getting proper power management due to
// * force wake clears.
// */
//int IGFX::RPSControl::pmNotifyWrapper(unsigned int a0,unsigned int a1,unsigned long long * a2,unsigned int * freq) {
//	uint32_t cfreq = 0;
//
//	FunctionCast(IGFX::RPSControl::pmNotifyWrapper, callbackIGFX->RPSControl.orgPmNotifyWrapper)(a0, a1, a2, &cfreq);
//	
//	if (!callbackIGFX->RPSControl.freq_max) {
//		callbackIGFX->RPSControl.freq_max = callbackIGFX->AppleIntelFramebufferController__ReadRegister32(*callbackIGFX->gFramebufferController, GEN6_RP_STATE_CAP) & 0xff;
//		DBGLOG(log, "Read RP0 %d", callbackIGFX->RPSControl.freq_max);
//	}
//	
////	DBGLOG(log, "pmNotifyWrapper sets freq 0x%x", cfreq);
//	*freq = (GEN9_FREQ_SCALER << GEN9_FREQUENCY_SHIFT) * callbackIGFX->RPSControl.freq_max;
//
//	return 0;
//}
//
//// TODO: DEPRECATED
//void IGFX::RPSControl::initFB(IGFX& ig,KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
//	KernelPatcher::RouteRequest req {
//			"__ZL15pmNotifyWrapperjjPyPj",
//			&IGFX::RPSControl::pmNotifyWrapper,
//			orgPmNotifyWrapper
//	};
//
//	if (!(ig.AppleIntelFramebufferController__ReadRegister32 && ig.gFramebufferController && patcher.routeMultiple(index, &req, 1, address, size, true, true)))
//		SYSLOG(log, "failed to route igfx FB PM functions");
//}

// MARK: - Force Wake Workaround

bool IGFX::ForceWakeWorkaround::pollRegister(uint32_t reg, uint32_t val, uint32_t mask, uint32_t timeout) {
	AbsoluteTime now, deadline;

	clock_interval_to_deadline(timeout, kMillisecondScale, &deadline);
	
	for (clock_get_uptime(&now); now < deadline; clock_get_uptime(&now)) {
		auto rd = callbackIGFX->AppleIntelFramebufferController__ReadRegister32(*callbackIGFX->gFramebufferController, reg);

//		DBGLOG(log, "Rd 0x%x = 0x%x, expected 0x%x", reg, rd, val);

		if ((rd & mask) == val)
			return true;
	}

	return false;
}

bool IGFX::ForceWakeWorkaround::forceWakeWaitAckFallback(uint32_t d, uint32_t val, uint32_t mask) {
	unsigned pass = 1;
	bool ack = false;
	
	do {
		pollRegister(ackForDom(d), 0, FORCEWAKE_KERNEL_FALLBACK, FORCEWAKE_ACK_TIMEOUT_MS);
		
		callbackIGFX->AppleIntelFramebufferController__WriteRegister32(*callbackIGFX->gFramebufferController,
																	   regForDom(d), fw_set(FORCEWAKE_KERNEL_FALLBACK));
		
		IODelay(10 * pass);
		pollRegister(ackForDom(d), FORCEWAKE_KERNEL_FALLBACK, FORCEWAKE_KERNEL_FALLBACK, FORCEWAKE_ACK_TIMEOUT_MS);
		
		ack = (callbackIGFX->AppleIntelFramebufferController__ReadRegister32(*callbackIGFX->gFramebufferController,
																			ackForDom(d)) & mask) == val;

		callbackIGFX->AppleIntelFramebufferController__WriteRegister32(*callbackIGFX->gFramebufferController,
																	   regForDom(d), fw_clear(FORCEWAKE_KERNEL_FALLBACK));
	} while (!ack && pass++ < 10);
	
//	DBGLOG(log, "Force wake fallback used to %s %s in %u passes", set ? "set" : "clear", strForDom(d), pass);
	
	return ack;
}

/**
 * Port of i915 force wake. The difference with Apple code is as follows:
 * 1. 50 ms ACK timeouts, see https://patchwork.kernel.org/patch/7057561/
 * Apple code uses 90 ms.
 * 2. Use reserve bit as a fallback at primary ACK timeout, see https://patchwork.kernel.org/patch/10029821/
 */

// NOTE: We are either in IRQ context, or in a spinlock critical section
void IGFX::ForceWakeWorkaround::forceWake(void*, uint8_t set, uint32_t dom, uint32_t ctx) {
	assert(callbackIGFX->gFramebufferController && *callbackIGFX->gFramebufferController);
//	DBGLOG(log, "ForceWake %u %u", set, dom);
	
	// ctx 2: IRQ, 1: normal
	
	uint32_t ack_exp = set << ctx;
	uint32_t mask = 1 << ctx;
	uint32_t wr = ack_exp | (1 << ctx << 16);
	
	for (unsigned d = DOM_FIRST; d <= DOM_LAST; d <<= 1)
	if (dom & d) {
		callbackIGFX->AppleIntelFramebufferController__WriteRegister32(*callbackIGFX->gFramebufferController,
			regForDom(d), wr);
		IOPause(100);
		if (!pollRegister(ackForDom(d), ack_exp, mask, FORCEWAKE_ACK_TIMEOUT_MS) &&
			!forceWakeWaitAckFallback(d, ack_exp, mask) &&
			!pollRegister(ackForDom(d), ack_exp, mask, FORCEWAKE_ACK_TIMEOUT_MS))
			PANIC(log, "ForceWake timeout for domain %s, expected 0x%x", strForDom(dom), ack_exp);
	}
}

void IGFX::ForceWakeWorkaround::initGraphics(IGFX& ig, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {

	KernelPatcher::RouteRequest req {
			"__ZN16IntelAccelerator26SafeForceWakeMultithreadedEbjj",
			&IGFX::ForceWakeWorkaround::forceWake
	};
	if (!(ig.AppleIntelFramebufferController__ReadRegister32 && ig.gFramebufferController &&
		  patcher.routeMultiple(index, &req, 1, address, size, true, true)))
		SYSLOG(log, "Failed to route SafeForceWake");
}
