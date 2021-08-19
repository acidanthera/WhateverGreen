//
//  kern_igfx_backlight.hpp
//  WhateverGreen
//
//  Created by FireWolf on 7/29/21.
//  Copyright © 2021 vit9696. All rights reserved.
//

#ifndef kern_igfx_backlight_hpp
#define kern_igfx_backlight_hpp

#include <IOKit/IOEventSource.h>
#include "kern_util.hpp"

/**
 *  Backlight registers
 */
static constexpr uint32_t BLC_PWM_CPU_CTL = 0x48254;
static constexpr uint32_t BXT_BLC_PWM_CTL1 = 0xC8250;
static constexpr uint32_t BXT_BLC_PWM_FREQ1 = 0xC8254;
static constexpr uint32_t BXT_BLC_PWM_DUTY1 = 0xC8258;

/**
 *  Represents a single brightness adjustment request
 */
struct BrightnessRequest {
	/**
	 *  The framebuffer controller
	 */
	void *controller {nullptr};
	
	/**
	 *  The register address
	 */
	uint32_t address {0};
	
	/**
	 *  The target register value
	 */
	uint32_t target {0};
	
	/**
	 *  Specify the portion of bits in the register value that represent the brightness level
	 */
	uint32_t mask {0};
	
	/**
	 *  Create an empty request
	 */
	BrightnessRequest() = default;
	
	/**
	 *  Create a brightness request
	 */
	BrightnessRequest(void *controller, uint32_t address, uint32_t target, uint32_t mask = 0xFFFFFFFF) :
		controller(controller), address(address), target(target), mask(mask) {}
	
	/**
	 *  Get the current brightness level
	 *
	 *  @param current The current register value
	 *  @return The current brightness level.
	 */
	inline uint32_t getCurrentBrightness(uint32_t current) {
		return current & mask;
	}
	
	/**
	 *  Get the target brightness level
	 *
	 *  @return The target brightness level.
	 */
	inline uint32_t getTargetBrightness() {
		return target & mask;
	}
	
	/**
	 *  Get the target register value from the given brightness level
	 *
	 *  @param brightness The brightness level
	 *  @return The corresponding register value
	 */
	inline uint32_t getTargetRegisterValue(uint32_t brightness) {
		return brightness | (target & ~mask);
	}
};

/**
 *  Type of the adjustment request queue
 */
using BrightnessRequestQueue = CircularBuffer<BrightnessRequest>;

/**
 *  An event source that adjusts the brightness smoothly
 */
class BrightnessRequestEventSource: public IOEventSource {
	/**
	 *  Constructors & Destructors
	 */
	OSDeclareDefaultStructors(BrightnessRequestEventSource);
	
	using super = IOEventSource;
	
	/**
	 *  Check whether a brightness adjustment request is pending and if so process the request on the workloop
	 *
	 *  @return `true` if the work loop should invoke this function again.
	 *          i.e., One or more requests are pending after the workloop has processed the current one.
	 */
	bool checkForWork() override;
	
public:
	/**
	 *  Create an event source
	 *
	 *  @param owner The owner of the event source
	 *  @return A non-null event source on success, `nullptr` otherwise.
	 */
	static BrightnessRequestEventSource *create(OSObject *owner);
};

#endif /* kern_igfx_backlight_hpp */
