//
//  kern_igfx_pm.cpp
//  WhateverGreen
//
//  Created by Pb on 22/06/2020.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Library/LegacyIOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include "kern_agdc.hpp"
#include "kern_igfx.hpp"

#define _MMIO(x) x

#define MCHBAR_MIRROR_BASE_SNB	0x140000
#define GEN6_RP_STATE_CAP	_MMIO(MCHBAR_MIRROR_BASE_SNB + 0x5998)
#define GEN6_RP_CONTROL				_MMIO(0xA024)
#define   GEN6_RP_MEDIA_TURBO			(1 << 11)
#define   GEN6_RP_ENABLE			(1 << 7)
#define   GEN6_RP_MEDIA_MODE_MASK		(3 << 9)
#define   GEN6_RP_MEDIA_SW_MODE			(0 << 9)

#define GT_FREQUENCY_MULTIPLIER 50
#define GEN9_FREQ_SCALER 3

namespace {
// FIXME: Hardcoded member offset
uint32_t mmioRead(void* accel, uint32_t reg) {
	auto mmio = getMember<volatile uint32_t *>(accel, 0x10a8);
	return mmio[reg/4];
}

void mmioWrite(void* accel, uint32_t reg, uint32_t v) {
	auto mmio = getMember<volatile uint32_t *>(accel, 0x10a8);
	mmio[reg/4] = v;
}

static constexpr const char* log = "igfx_pm";
}

/* gen6_init_rps_frequencies */
char IGFX::RPSControl::IGHardwareCommandStreamer2__init(void* streamer,void* accel, void* wl, void* sched, uint8_t type) {
	uint32_t rp_state_cap = mmioRead(accel, GEN6_RP_STATE_CAP);
	unsigned rp0_freq = (rp_state_cap >>  0) & 0xff;
	unsigned rp1_freq = (rp_state_cap >>  8) & 0xff;
	unsigned min_freq = (rp_state_cap >> 16) & 0xff;

	/* Store the frequency values in 16.66 MHZ units, which is
	 * the natural hardware unit for SKL
	 */
	rp0_freq *= GEN9_FREQ_SCALER;
	rp1_freq *= GEN9_FREQ_SCALER;
	min_freq *= GEN9_FREQ_SCALER;

	SYSLOG(log, "rp0 0x%x rp1 0x%x min 0x%x", rp0_freq, rp1_freq, min_freq);

	uint32_t rpmodectl = mmioRead(accel, GEN6_RP_CONTROL);
	SYSLOG(log, "Video Turbo Mode: %d", rpmodectl & GEN6_RP_MEDIA_TURBO);
	SYSLOG(log, "HW control enabled: %d", rpmodectl & GEN6_RP_ENABLE);
	SYSLOG(log, "SW control enabled: %d\n", (rpmodectl & GEN6_RP_MEDIA_MODE_MASK) == GEN6_RP_MEDIA_SW_MODE);

	return FunctionCast(IGFX::RPSControl::IGHardwareCommandStreamer2__init,
						callbackIGFX->RPSControl.orgIGHardwareCommandStreamer2__init)(streamer, accel, wl, sched, type);
}

uint64_t IGFX::RPSControl::pmNotify(uint32_t a0, uint32_t a1, uint64_t *, uint32_t* freq) {
	SYSLOG(log, "pmNotify a0 0x%x a1 0x%x freq 0x%x");
	return 0;
}

void IGFX::RPSControl::init(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest requests[] {
		{"__ZN26IGHardwareCommandStreamer24initEP22IOGraphicsAccelerator2P10IOWorkLoopP12IGScheduler210IGHwCsType",
			&IGFX::RPSControl::IGHardwareCommandStreamer2__init,
			orgIGHardwareCommandStreamer2__init},
		{"__ZL9_pmNotifyjjPyPj",
			&IGFX::RPSControl::pmNotify
		}
	};

	if (!patcher.routeMultiple(index, requests, address, size, true, true))
		SYSLOG(log, "failed to route igfx PM functions");
}
