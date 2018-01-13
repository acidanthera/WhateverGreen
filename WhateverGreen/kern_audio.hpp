//
//  kern_audio.hpp
//  WhateverGreen
//
//  Copyright © 2017 vit9696. All rights reserved.
//

#ifndef kern_audio_hpp
#define kern_audio_hpp

#include <Headers/kern_util.hpp>

#include <Library/LegacyIOService.h>
#include <sys/types.h>

class EXPORT WhateverAudio : public IOService {
	OSDeclareDefaultStructors(WhateverAudio)
	uint32_t getAnalogLayout();
public:
	IOService *probe(IOService *provider, SInt32 *score) override;
};

#endif /* kern_audio_hpp */
