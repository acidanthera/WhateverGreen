//
//  kern_igfx_util.cpp
//  WhateverGreen
//
//  Created by FireWolf on 8/7/21.
//  Copyright © 2021 vit9696. All rights reserved.
//

#include "kern_igfx_util.hpp"

//
// MARK: - OSObject Wrapper
//

/**
 *  Meta Class Definitions
 */
OSDefineMetaClassAndStructors(OSObjectWrapper, OSObject);

/**
 *  Initialize the wrapper with the given object
 *
 *  @param object The wrapped object that is not an `OSObject`
 *  @return `true` on success, `false` otherwise.
 */
bool OSObjectWrapper::init(void *object) {
	if (!super::init())
		return false;
	
	this->object = object;
	return true;
}

/**
 *  Create a wrapper for the given object that is not an `OSObject`
 *
 *  @param object A non-null object
 *  @return A non-null wrapper on success, `nullptr` otherwise.
 *  @warning The caller is responsbile for managing the lifecycle of the given object.
 */
OSObjectWrapper *OSObjectWrapper::of(void *object) {
	auto instance = OSTypeAlloc(OSObjectWrapper);
	if (instance == nullptr)
		return nullptr;
	
	if (!instance->init(object)) {
		instance->release();
		return nullptr;
	}
	
	return instance;
}
