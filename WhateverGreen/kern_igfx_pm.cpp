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

#include <kern/clock.h>

namespace {
constexpr const char* log = "igfx_pm";
_Atomic(uint32_t) freq_max; // max freq in this time unit
_Atomic(uint64_t) prev_time_ns; // abs time of previous setGTFrequencyMMIO call

//struct [[gnu::packed]] IGHwCsDesc {
//  char type;
//  char gap[4];
//  char *title;
//  char unk0[48];
//  char unk1[12];
//};
}

/**
 * We patch IGHardwareCommandStreamer2::submitExecList to control RPS for any kind of command streamer.
 * There is no synchronisation between RPNSWREQ access in the original code whatsoever, meaning that
 * concurrent requests will race, resulting in conflicting frequency setting.
 * One solution is to maintain a global maximum across all the streamers within RPNSWREQ_WINDOW_NS, and
 * only allow to decrease the frequency when the window elapses.
 */
void IGFX::RPSControl::IGHardwareCommandStreamer2__setGTFrequencyMMIO(void* streamer, unsigned freq) {
//	auto cs_desc = getMember<IGHwCsDesc*>(streamer, 0x1c8);
//	if (cs_desc->type == 1)
//		freq = 0x22800000;
//	SYSLOG(log, "setGTFrequencyMMIO freq 0x%x cs_desc->type 0x%x (%s)", freq, cs_desc->type, cs_desc->title);

	uint64_t now, now_ns;
	clock_get_uptime(&now);
	absolutetime_to_nanoseconds(now, &now_ns);

	if (now_ns - atomic_load_explicit(&prev_time_ns, memory_order_relaxed) <= RPNSWREQ_WINDOW_NS) {
		auto freq_curr = atomic_load_explicit(&freq_max, memory_order_relaxed);
		uint32_t freq_new;
		
		do {
			if (freq > freq_curr)
				freq_new = freq;
			else
				break;
		} while (!atomic_compare_exchange_weak_explicit(&freq_max, &freq_curr, freq_new,
														memory_order_relaxed, memory_order_relaxed));

		// Only write the maximum to RPNSWREQ
		if (freq > freq_curr)
			FunctionCast(IGFX::RPSControl::IGHardwareCommandStreamer2__setGTFrequencyMMIO,
			callbackIGFX->RPSControl.orgIGHardwareCommandStreamer2__setGTFrequencyMMIO)(streamer, freq);
	} else {
		// Restart time window
		atomic_store_explicit(&prev_time_ns, now_ns, memory_order_relaxed);
		
		// Use any frequency when restarted
		atomic_store_explicit(&freq_max, freq, memory_order_relaxed);
		
		FunctionCast(IGFX::RPSControl::IGHardwareCommandStreamer2__setGTFrequencyMMIO,
		callbackIGFX->RPSControl.orgIGHardwareCommandStreamer2__setGTFrequencyMMIO)(streamer, freq);
	}
}

void IGFX::RPSControl::init(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	atomic_store(&freq_max, 0);
	atomic_store(&prev_time_ns, 0);
	
	mach_vm_address_t orgIGHardwareCommandStreamer2__submitExecList {};
	orgIGHardwareCommandStreamer2__submitExecList = patcher.solveSymbol(index, "__ZN26IGHardwareCommandStreamer214submitExecListEj", address, size);
	
	/**
	 * IGHardwareCommandStreamer2::submitExecList only controls RPS for RCS type streamers.
	 * Patch it to enable control for any kind of streamer.
	 */
	if (orgIGHardwareCommandStreamer2__submitExecList) {
		mach_vm_address_t start = orgIGHardwareCommandStreamer2__submitExecList;
		constexpr unsigned ninsts_max {64};
		
		hde64s dis;
		
		bool found_cmp = false;
		bool found_jmp = false;

		for (size_t i = 0; i < ninsts_max; i++) {
			auto sz = Disassembler::hdeDisasm(start, &dis);

			if (dis.flags & F_ERROR) {
				SYSLOG(log, "Error disassembling submitExecList");
				break;
			}

			/* cmp byte ptr [rcx], 0*/
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
			}
		} else
			SYSLOG(log, "jnz in submitExecList not found");
	} else {
		SYSLOG(log, "Failed to solve submitExecList (%d)", patcher.getError());
		patcher.clearError();
	}
	
	KernelPatcher::RouteRequest requests[] {
		{"__ZN26IGHardwareCommandStreamer218setGTFrequencyMMIOEj",
			&IGFX::RPSControl::IGHardwareCommandStreamer2__setGTFrequencyMMIO,
			orgIGHardwareCommandStreamer2__setGTFrequencyMMIO
		}
	};

	if (!patcher.routeMultiple(index, requests, address, size, true, true))
		SYSLOG(log, "failed to route igfx PM functions");
}
