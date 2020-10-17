//
//  kern_igfx_i2c_aux.cpp
//  WhateverGreen
//
//  Created by FireWolf on 9/21/20.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include <Headers/kern_util.hpp>
#include "kern_igfx.hpp"

void IGFX::AdvancedI2COverAUXSupport::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingGraphics = false;
	requiresPatchingFramebuffer = true;
}

void IGFX::AdvancedI2COverAUXSupport::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// Enable the advanced I2C-over-AUX support if LSPCON is enabled
	enabled = callbackIGFX->modLSPCONDriverSupport.enabled;
	
	// Or if verbose output in transactions is enabled
	if (checkKernelArgument("-igfxi2cdbg")) {
		enabled = true;
		verbose = true;
	}
}

void IGFX::AdvancedI2COverAUXSupport::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	KernelPatcher::RouteRequest requests[] = {
		{
			"__ZN31AppleIntelFramebufferController14ReadI2COverAUXEP21AppleIntelFramebufferP21AppleIntelDisplayPathjtPhbh",
			wrapReadI2COverAUX,
			reinterpret_cast<mach_vm_address_t&>(orgReadI2COverAUX)
		},
		{
			"__ZN31AppleIntelFramebufferController15WriteI2COverAUXEP21AppleIntelFramebufferP21AppleIntelDisplayPathjtPhb",
			wrapWriteI2COverAUX,
			reinterpret_cast<mach_vm_address_t&>(orgWriteI2COverAUX)
		}
	};
	
	if (patcher.routeMultiple(index, requests, address, size)) {
		DBGLOG("igfx", "I2C: Functions have been routed successfully");
	} else {
		patcher.clearError();
		SYSLOG("igfx", "I2C: Failed to route functions.");
	}
}

IOReturn IGFX::AdvancedI2COverAUXSupport::wrapReadI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint16_t length, uint8_t *buffer, bool intermediate, uint8_t flags) {
	if (callbackIGFX->modAdvancedI2COverAUXSupport.verbose) {
		uint32_t index = 0xFF;
		AppleIntelFramebufferExplorer::getIndex(framebuffer, index);
		DBGLOG("igfx", "I2C:  ReadI2COverAUX() called. FB%d: Addr = 0x%02x; Len = %02d; MOT = %d; Flags = %d.",
			   index, address, length, intermediate, flags);
		IOReturn retVal = callbackIGFX->modAdvancedI2COverAUXSupport.orgReadI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate, flags);
		DBGLOG("igfx", "I2C:  ReadI2COverAUX() returns 0x%x.", retVal);
		return retVal;
	} else {
		return callbackIGFX->modAdvancedI2COverAUXSupport.orgReadI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate, flags);
	}
}

IOReturn IGFX::AdvancedI2COverAUXSupport::wrapWriteI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint16_t length, uint8_t *buffer, bool intermediate) {
	if (callbackIGFX->modAdvancedI2COverAUXSupport.verbose) {
		uint32_t index = 0xFF;
		AppleIntelFramebufferExplorer::getIndex(framebuffer, index);
		DBGLOG("igfx", "I2C: WriteI2COverAUX() called. FB%d: Addr = 0x%02x; Len = %02d; MOT = %d; Flags = 0.",
			   index, address, length, intermediate);
		IOReturn retVal = callbackIGFX->modAdvancedI2COverAUXSupport.orgWriteI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate);
		DBGLOG("igfx", "I2C: WriteI2COverAUX() returns 0x%x.", retVal);
		return retVal;
	} else {
		return callbackIGFX->modAdvancedI2COverAUXSupport.orgWriteI2COverAUX(that, framebuffer, displayPath, address, length, buffer, intermediate);
	}
}

IOReturn IGFX::AdvancedI2COverAUXSupport::advSeekI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint8_t flags) {
	// No need to check the given `address` and `offset`
	// if they are invalid, the underlying RunAUXCommand() will return an error
	// First start the transaction by performing an empty write
	IOReturn retVal = wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, true);

	// Guard: Check the START transaction
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "I2C: AdvSeekI2COverAUX() Error: Failed to start the I2C transaction. Return value = 0x%x.\n", retVal);
		return retVal;
	}

	// Write a single byte to the given I2C slave
	// and set the Middle-of-Transaction bit to 1
	return wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 1, reinterpret_cast<uint8_t*>(&offset), true);
}

IOReturn IGFX::AdvancedI2COverAUXSupport::advReadI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint16_t length, uint8_t *buffer, uint8_t flags) {
	// Guard: Check the buffer length
	if (length == 0) {
		SYSLOG("igfx", "I2C: AdvReadI2COverAUX() Error: Buffer length must be non-zero.");
		return kIOReturnInvalid;
	}

	// Guard: Check the buffer
	if (buffer == nullptr) {
		SYSLOG("igfx", "I2C: AdvReadI2COverAUX() Error: Buffer cannot be NULL.");
		return kIOReturnInvalid;
	}

	// Guard: Start the transaction and set the access offset successfully
	IOReturn retVal = advSeekI2COverAUX(that, framebuffer, displayPath, address, offset, flags);
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "I2C: AdvReadI2COverAUX() Error: Failed to set the data offset.");
		return retVal;
	}

	// Process the read request
	// ReadI2COverAUX() can only process up to 16 bytes in one AUX transaction
	// because the burst data size is 20 bytes, in which the first 4 bytes are used for the AUX message header
	while (length != 0) {
		// Calculate the new length for this I2C-over-AUX transaction
		uint16_t newLength = length >= 16 ? 16 : length;

		// This is an intermediate transaction
		retVal = wrapReadI2COverAUX(that, framebuffer, displayPath, address, newLength, buffer, true, flags);

		// Guard: The intermediate transaction succeeded
		if (retVal != kIOReturnSuccess) {
			// Terminate the transaction
			wrapReadI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false, flags);
			return retVal;
		}

		// Update the buffer position and length
		length -= newLength;
		buffer += newLength;
	}

	// All intermediate transactions succeeded
	// Terminate the transaction
	return wrapReadI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false, flags);
}

IOReturn IGFX::AdvancedI2COverAUXSupport::advWriteI2COverAUX(void *that, IORegistryEntry *framebuffer, void *displayPath, uint32_t address, uint32_t offset, uint16_t length, uint8_t *buffer, uint8_t flags) {
	// Guard: Check the buffer length
	if (length == 0) {
		SYSLOG("igfx", "I2C: AdvWriteI2COverAUX() Error: Buffer length must be non-zero.");
		return kIOReturnInvalid;
	}

	// Guard: Check the buffer
	if (buffer == nullptr) {
		SYSLOG("igfx", "I2C: AdvWriteI2COverAUX() Error: Buffer cannot be NULL.");
		return kIOReturnInvalid;
	}

	// Guard: Start the transaction and set the access offset successfully
	IOReturn retVal = advSeekI2COverAUX(that, framebuffer, displayPath, address, offset, flags);
	if (retVal != kIOReturnSuccess) {
		SYSLOG("igfx", "I2C: AdvWriteI2COverAUX() Error: Failed to set the data offset.");
		return retVal;
	}

	// Process the write request
	// WriteI2COverAUX() can only process up to 16 bytes in one AUX transaction
	// because the burst data size is 20 bytes, in which the first 4 bytes are used for the AUX message header
	while (length != 0) {
		// Calculate the new length for this I2C-over-AUX transaction
		uint16_t newLength = length >= 16 ? 16 : length;

		// This is an intermediate transaction
		retVal = wrapWriteI2COverAUX(that, framebuffer, displayPath, address, newLength, buffer, true);

		// Guard: The intermediate transaction succeeded
		if (retVal != kIOReturnSuccess) {
			// Terminate the transaction
			wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false);
			return retVal;
		}

		// Update the buffer position and length
		length -= newLength;
		buffer += newLength;
	}

	// All intermediate transactions succeeded
	// Terminate the transaction
	return wrapWriteI2COverAUX(that, framebuffer, displayPath, address, 0, nullptr, false);
}
