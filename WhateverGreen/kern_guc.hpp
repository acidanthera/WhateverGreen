//
//  kern_guc.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_guc_hpp
#define kern_guc_hpp

#include <libkern/libkern.h>

static constexpr size_t GuCFirmwareSignatureSize = 256;

extern const uint8_t *GuCFirmwareSKL;
extern const uint8_t *GuCFirmwareSKLSignature;
extern const size_t GuCFirmwareSKLSize;

extern const uint8_t *GuCFirmwareKBL;
extern const uint8_t *GuCFirmwareKBLSignature;
extern const size_t GuCFirmwareKBLSize;

#endif /* kern_guc_hpp */
