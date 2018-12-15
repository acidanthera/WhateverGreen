//
//  kern_nvhda.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_nvhda_hpp
#define kern_nvhda_hpp

#include <Headers/kern_util.hpp>
#include <Library/LegacyIOService.h>

class EXPORT NVHDAEnabler : public IOService {
	OSDeclareDefaultStructors(NVHDAEnabler);
	static constexpr uint32_t HDAEnableReg = 0x488;
	static constexpr uint32_t HDAEnableBit = 0x02000000;
public:
	IOService* probe(IOService *provider, SInt32 *score) override;
	bool start(IOService *provider) override;
};

#endif /* kern_nvhda_hpp */
