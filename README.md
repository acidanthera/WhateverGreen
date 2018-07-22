WhateverGreen
=============

[![Build Status](https://travis-ci.org/acidanthera/WhateverGreen.svg?branch=master)](https://travis-ci.org/acidanthera/WhateverGreen) [![Scan Status](https://scan.coverity.com/projects/16177/badge.svg?flat=1)](https://scan.coverity.com/projects/16177)


[Lilu](https://github.com/acidanthera/Lilu) plugin providing patches to select GPUs on macOS. Requires Lilu 1.2.5 or newer.

#### Features
- Fixes boot to black screen on AMD and NVIDIA
- Fixes sleep wake to black screen on AMD
- Fixes boot screen distortion in certain cases
- Fixes transmitter/encoder in autodetected connectors for multimonitor support (`-raddvi`)
- Fixes HD 7730/7750/7770/R7 250/R7 250X initialisation (`radpg=15`)
- Allows tuning of aty_config, aty_properties, cail_properties via ACPI
- Allows enforcing 24-bit mode on unsupported displays (`-rad24`)
- Allows booting without video acceleration (`-radvesa`)
- Allows automatically setting GPU model name or providing it manually for RadeonFramebuffer
- Allows specifying custom connectors via device properties for RadeonFramebuffer
- Allows tuning autodetected connector priority via device properties (HD 7xxx or newer)
- Fixes an issue in AppleGraphicsDevicePolicy.kext so that we could use a MacPro6,1 board-id/model combination,  without the usual hang with a black screen. [Patching AppleGraphicsDevicePolicy.kext](https://pikeralpha.wordpress.com/2015/11/23/patching-applegraphicsdevicepolicy-kext)
- Modifies macOS to recognize NVIDIA's web drivers as platform binaries. This resolves the issue with transparent windows without content, which appear for applications that use Metal and have Library Validation enabled. Common affected applications are iBooks and Little Snitch Network Monitor, though this patch is universal and fixes them all. [NVWebDriverLibValFix](https://github.com/mologie/NVWebDriverLibValFix)
- Injects IOVARendererID into GPU properties (required for Shiki-based solution for non-freezing Intel and/or any discrete GPU)
- Allows to use ports HDMI, DP, Digital DVI with audio (Injects @0connector-type - @5connector-type properties into GPU)
- Fixes NVIDIA GPU interface stuttering on 10.13 (official and web drivers)

#### Documentation
Read [FAQs](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/) and avoid asking any questions. No support is provided for the time being.

#### Boot arguments
- `-wegdbg` to enable debug printing (available in DEBUG binaries).  
- `-wegoff` to disable WhateverGreen.  
- `-wegbeta` to enable WhateverGreen on unsupported os versions (10.13 and below are enabled by default).  
- `-wegnoegpu` to disable external GPU (or add `disable-external-gpu` property to IGPU)
- `-radvesa` to disable ATI/AMD video acceleration completely.  
- `-igfxvesa` to boot Intel graphics without hardware acceleration (VESA mode).  
- `-rad24` to enforce 24-bit display mode.  
- `-raddvi` to enable DVI transmitter correction (required for 290X, 370, etc.).  
- `radpg=15` to disable several power-gating modes (see FAQ, required for Cape Verde GPUs).
- `agdpmod=cfgmap` enforcing `none` into ConfigMap dictionary for system board-id
- `agdpmod=vit9696` disables check for board-id , enabled by default
- `agdpmod=pikera` replaces `board-id` with `board-ix`
- `ngfxgl=1` boot argument (and `disable-metal` property) to disable Metal support on NVIDIA
- `ngfxcompat=1` boot argument (and `force-compat` property) to ignore compatibility check in NVDAStartupWeb
- `ngfxsubmit=0` boot argument to disable interface stuttering fix on 10.13
- `igfxrst=1` to prefer drawing Apple logo at 2nd boot stage instead of framebuffer copying.  
- `igfxframe=frame` to inject a dedicated framebuffer identifier into IGPU (only for TESTING purposes).  
- `igfxsnb=0` to disable IntelAccelerator name fix for Sandy Bridge CPUs.  
- `igfxgl=0` to disable Metal support on Intel.  
`-igfxnohdmi` to disable DP to HDMI conversion patches for digital sound.  
- `-cdfoff` to disable HDMI 2.0 patches.  

#### Credits
- [Apple](https://www.apple.com) for macOS
- [AMD](https://www.amd.com) for ATOM VBIOS parsing code
- [The PCI ID Repository](http://pci-ids.ucw.cz) for multiple GPU model names
- [Floris497](https://github.com/Floris497) for the CoreDisplay [patches](https://github.com/Floris497/mac-pixel-clock-patch-v2)
- [headkaze](https://github.com/headkaze) Intel framebuffer patching code
- [igork](https://applelife.ru/members/igork.564/) for power-gating patch discovery
- [lvs1974](https://applelife.ru/members/lvs1974.53809) for continuous implementation of Intel and NVIDIA fixing code
- [mologie](https://github.com/mologie/NVWebDriverLibValFix) for creating NVWebDriverLibValFix.kext which forces macOS to recognize NVIDIA's web drivers as platform binaries
- [PMheart](https://github.com/PMheart) for CoreDisplay patching code and Intel fix backporting
- [RemB](https://applelife.ru/members/remb.8064/) for continuing sleep-wake research and finding the right register for AMD issues
- [Vandroiy](https://applelife.ru/members/vandroiy.83653/) for maintaining the GPU model detection database
- [YungRaj](https://github.com/YungRaj) and [syscl](https://github.com/syscl) for Intel fix backporting
- [vit9696](https://github.com/vit9696) for writing the software and maintaining it
