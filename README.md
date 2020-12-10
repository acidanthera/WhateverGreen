WhateverGreen
=============

[![Build Status](https://github.com/acidanthera/WhateverGreen/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/WhateverGreen/actions) [![Scan Status](https://scan.coverity.com/projects/16177/badge.svg?flat=1)](https://scan.coverity.com/projects/16177)

[Lilu](https://github.com/acidanthera/Lilu) plugin providing patches to select GPUs on macOS. Requires Lilu 1.4.0 or newer.

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
- For Intel HD digital audio HDMI, DP, Digital DVI (Patches connector-type DP -> HDMI)
- Fixes NVIDIA GPU interface stuttering on 10.13 (official and web drivers)
- Fixes the kernel panic caused by an invalid link rate reported by DPCD on some laptops with Intel IGPU.
- Fixes the infinite loop on establishing Intel HDMI connections with a higher pixel clock rate on Skylake, Kaby Lake and Coffee Lake platforms.
- Implements the driver support for onboard LSPCON chips to enable DisplayPort to HDMI 2.0 output on some platforms with Intel IGPU.
- Enforces complete modeset on non-built-in displays on Kaby Lake and newer to fix booting to black screen.
- Allows non-supported cards to use HW video encoder (`-radcodec`)
- Fixes choppy video playback on Intel Kaby Lake and newer.
- Fixes black screen on Intel HD since 10.15.5.
- Adds workaround for rare force wake timeout panics on Intel KBL and CFL.
- Supports all valid Core Display Clock (CDCLK) freqencies on Intel ICL platforms.
- Fixes the kernel panic caused by an incorrectly calculated amount of DVMT pre-allocated memory on Intel ICL platforms.

#### Documentation

Read [FAQs](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/) and avoid asking any questions. No support is provided for the time being.

#### Boot arguments

- `-wegdbg` to enable debug printing (available in DEBUG binaries).
- `-wegoff` to disable WhateverGreen.
- `-wegbeta` to enable WhateverGreen on unsupported OS versions (11 and below are enabled by default).
- `-wegnoegpu` to disable external GPU (or add `disable-external-gpu` property to IGPU).
- `-radvesa` to disable ATI/AMD video acceleration completely.
- `-rad24` to enforce 24-bit display mode.
- `-raddvi` to enable DVI transmitter correction (required for 290X, 370, etc.).
- `-radcodec` to force the spoofed PID to be used in AMDRadeonVADriver
- `radpg=15` to disable several power-gating modes (see FAQ, required for Cape Verde GPUs).
- `agdpmod=vit9696` disables check for `board-id` (or add `agdpmod` property to external GPU).
- `agdpmod=pikera` replaces `board-id` with `board-ix`
- `agdpmod=ignore` disables AGDP patches (`vit9696,pikera` value is implicit default for external GPUs)
- `ngfxgl=1` boot argument (and `disable-metal` property) to disable Metal support on NVIDIA
- `ngfxcompat=1` boot argument (and `force-compat` property) to ignore compatibility check in NVDAStartupWeb
- `ngfxsubmit=0` boot argument (and `disable-gfx-submit` property) to disable interface stuttering fix on 10.13
- `gfxrst=1` to prefer drawing Apple logo at 2nd boot stage instead of framebuffer copying.
- `gfxrst=4` to disable framebuffer init interaction during 2nd boot stage.
- `igfxframe=frame` to inject a dedicated framebuffer identifier into IGPU (only for TESTING purposes).
- `igfxsnb=0` to disable IntelAccelerator name fix for Sandy Bridge CPUs.
- `igfxgl=1` boot argument (and `disable-metal` property) to disable Metal support on Intel.
- `igfxmetal=1` boot argument (and `enable-metal` property) to force enable Metal support on Intel for offline rendering.
- `igfxpavp=1` boot argument (and `igfxpavp` property) to force enable PAVP output
- `igfxfw=2` boot argument (and `igfxfw` property) to force loading of Apple GuC firmware
- `-igfxvesa` to disable Intel Graphics acceleration.
- `-igfxnohdmi` boot argument (and `disable-hdmi-patches`) to disable DP to HDMI conversion patches for digital sound.
- `-igfxtypec` to force DP connectivity for Type-C platforms.
- `-cdfon` (and `enable-hdmi20` property) to enable HDMI 2.0 patches.
- `-igfxdump` to dump IGPU framebuffer kext to `/var/log/AppleIntelFramebuffer_X_Y` (available in DEBUG binaries).
- `-igfxfbdump` to dump native and patched framebuffer table to ioreg at IOService:/IOResources/WhateverGreen
- `igfxcflbklt=1` boot argument (and `enable-cfl-backlight-fix` property) to enable CFL backlight patch
- `applbkl=0` boot argument (and `applbkl` property) to disable AppleBacklight.kext patches for IGPU. In case of custom AppleBacklight profile- [read here.](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/FAQ.OldPlugins.en.md)
- `-igfxmlr` boot argument (and `enable-dpcd-max-link-rate-fix` property) to apply the maximum link rate fix.
- `-igfxhdmidivs` boot argument (and `enable-hdmi-dividers-fix` property) to fix the infinite loop on establishing Intel HDMI connections with a higher pixel clock rate on SKL, KBL and CFL platforms.
- `-igfxlspcon` boot argument (and `enable-lspcon-support` property) to enable the driver support for onboard LSPCON chips. [Read the manual](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/FAQ.IntelHD.en.md)
- `-igfxi2cdbg` boot argument to enable verbose output in I2C-over-AUX transactions (only for debugging purposes).
- `igfxagdc=0` boot argument (`disable-agdc` device property) to disable AGDC.
- `igfxfcms=1` boot argument (`complete-modeset` device property) to force complete modeset on Skylake or Apple firmwares.
- `igfxfcmsfbs=` boot argument (`complete-modeset-framebuffers` device property) to specify
indices of connectors for which complete modeset must be enforced. Each index is a byte in
a 64-bit word; for example, value `0x010203` specifies connectors 1, 2, 3. If a connector is
not in the list, the driver's logic is used to determine whether complete modeset is needed. Pass `-1` to disable.
- `igfxonln=1` boot argument (`force-online` device property) to force online status on all displays.
- `igfxonlnfbs=MASK` boot argument (`force-online-framebuffers` device property) to specify
indices of connectors for which online status is enforced. Format is similar to `igfxfcmsfbs`.
- `wegtree=1` boot argument (`rebuild-device-tree` property) to force device renaming on Apple FW.
- `igfxrpsc=1` boot argument (`rps-control` property) to enable RPS control patch (improves IGPU performance).
- `-igfxcdc` boot argument (`enable-cdclk-frequency-fix` property) to support all valid Core Display Clock (CDCLK) frequencies on ICL platforms. [Read the manual](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/FAQ.IntelHD.en.md)
- `-igfxdvmt` boot argument (`enable-dvmt-calc-fix` property) to fix the kernel panic caused by an incorrectly calculated amount of DVMT pre-allocated memory on Intel ICL platforms.

#### Credits

- [Apple](https://www.apple.com) for macOS
- [AMD](https://www.amd.com) for ATOM VBIOS parsing code
- [The PCI ID Repository](http://pci-ids.ucw.cz) for multiple GPU model names
- [FireWolf](https://github.com/0xFireWolf/) for the DPCD maximum link rate fix, infinite loop fix for Intel HDMI connections, LSPCON driver support, Core Display Clock frequency fix for ICL platforms, and DVMT pre-allocated memory calculation fix for ICL platforms.
- [Floris497](https://github.com/Floris497) for the CoreDisplay [patches](https://github.com/Floris497/mac-pixel-clock-patch-v2)
- [Fraxul](https://github.com/Fraxul) for original CFL backlight patch
- [headkaze](https://github.com/headkaze) for Intel framebuffer patching code and CFL backlight patch improvements
- [hieplpvip](https://github.com/hieplpvip) for initial AppleBacklight patching plugin
- [igork](https://applelife.ru/members/igork.564/) for power-gating patch discovery and various FP research
- [lvs1974](https://applelife.ru/members/lvs1974.53809) for continuous implementation of Intel and NVIDIA fixing code
- [mologie](https://github.com/mologie/NVWebDriverLibValFix) for creating NVWebDriverLibValFix.kext which forces macOS to recognize NVIDIA's web drivers as platform binaries
- [PMheart](https://github.com/PMheart) for CoreDisplay patching code and Intel fix backporting
- [RehabMan](https://github.com/RehabMan) for various enhancements
- [RemB](https://applelife.ru/members/remb.8064/) for continuing sleep-wake research and finding the right register for AMD issues
- [Vandroiy](https://applelife.ru/members/vandroiy.83653/) for maintaining the GPU model detection database
- [YungRaj](https://github.com/YungRaj) and [syscl](https://github.com/syscl) for Intel fix backporting
- [vit9696](https://github.com/vit9696) for writing the software and maintaining it
