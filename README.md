WhateverGreen
=============

[Lilu](https://github.com/vit9696/Lilu) plugin providing patches to select ATI/AMD GPUs. Requires Lilu 1.1.6 or newer.

#### Features
- Fixes boot to black screen
- Fixes sleep wake to black screen 
- Fixes boot logo distortion in certain cases (`-radlogo`)
- Fixes transmitter/encoder in autodetected connectors for multimonitor support (`-raddvi`)
- Fixes HD 7730/7750/7770/R7 250/R7 250X initialisation (`radpg=15`)
- Fixes HDMI audio for _natively supported_ devices if not injected manually
- Allows tuning of aty_config, aty_properties, cail_properties via ACPI
- Allows enforcing 24-bit mode on unsupported displays (`-rad24`)
- Allows booting without video acceleration (`-radvesa`)
- Allows automatically setting GPU model name or providing it manually for RadeonFramebuffer
- Allows specifying custom connectors via device properties for RadeonFramebuffer
- Allows tuning autodetected connector priority via device properties (HD 7xxx or newer)

#### Documentation
Read FAQs and avoid asking any questions:  
- [English FAQ](https://github.com/vit9696/WhateverGreen/blob/master/Manual/FAQ.en.md)
- [Русский FAQ](https://github.com/vit9696/WhateverGreen/blob/master/Manual/FAQ.ru.md)

No support is provided for the time being.

#### Boot arguments
Add `-raddbg` to enable debug printing (available in DEBUG binaries).  
Add `-radvesa` to disable ATI/AMD video acceleration completely.  
Add `-radoff` to disable WhateverGreen.  
Add `-radbeta` to enable WhateverGreen on unsupported os versions (10.13 and below are enabled by default).  
Add `-rad24` to enforce 24-bit display mode.  
Add `-radlogo` to patch boot logo distortion.  
Add `-raddvi` to enable DVI transmitter correction (required for 290X, 370, etc.)  
Add `radpg=15` to disable several power-gating modes (see FAQ, required for 7xxx GPUs).

#### Credits
- [Apple](https://www.apple.com) for macOS
- [AMD](https://www.amd.com) for ATOM VBIOS parsing code
- [The PCI ID Repository](http://pci-ids.ucw.cz) for multiple GPU model names
- [igork](https://applelife.ru/members/igork.564/) for power-gating patch discovery
- [RemB](https://applelife.ru/members/remb.8064/) for continuing sleep-wake research and finding the right register
- [Vandroiy](https://applelife.ru/members/vandroiy.83653/) for maintaining the GPU model detection database
- [vit9696](https://github.com/vit9696) for writing the software and maintaining it
