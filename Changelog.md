WhateverGreen Changelog
=======================
#### v1.3.6
- Enabled CoreLSKD streaming patches by default for AMD hardware DRM on Ivy Bridge
- Repurposed 64 bit for FP 2.x streaming hardware accelerated streaming patches (can be used as `shikigva=80`)
- Fixed accelerator name update logic for X4xxx kexts
- Fixed Verde IOGVACodec injection to make hardware video decoder work
- Enable software TV+ decoding on all CPUs without IGPU (`shikigva=256`)

#### v1.3.5
- Added Lilu 1.4.0 support, which is now the minimum supported version
- Dropped legacy boot arguments (`-shikigva`, `-shikifps`)
- Fixed handling `agdpmod` GPU property (in IGPUs and in conjunction with boot-arg)
- Added `-wegtree` boot argument to force device renaming
- Fixed FairPlay DRM playback patches on 10.15
- Added `shikigva` and `shiki-id` aliases in IORegistry
- Added `applbkl` aliases to IORegistry (data, 32-bit)
- Added `applbkl-name` and `applbkl-data` IORegistry data keys to provide custom backlight data
- Fixed applying CoreFP patches on Apple firmware, when they are not needed
- Added `shikigva=16` (repurposed) property to use AMD hardware DRM decoder in select apps
- Added `shikigva=128` (repurposed) property to use hardware decoder for FairPlay 1.0 (can be used as `shikigva=144`)
- Do not disable DRM patches when `shikigva` is used even on Apple hardware for MacPro5,1 support

#### v1.3.4
- Added support for disabled AppleGraphicsDevicePolicy in AMD drivers on 10.15.1
- Added basic support for `-radcfg` and `-radgl` on AMD Navi GPUs

#### v1.3.3
- Rework backlight panel info injection to fix Mac issues

#### v1.3.2
- Added more GPUs for detection
- Enable IGPU graphics kernel panic workaround on 10.14.4+ on SKL

#### v1.3.1
- Fixed an issue that LSPCON driver fails to set the mode after the adapter power is off, i.e. sleep/wake up cycle.
- Unified release archive names
- Enforce complete IGPU modeset on Kaby Lake and newer (overridable by igfxfcmsfbs bootarg or
complete-modeset-framebuffers device property)
- Disable VRAM testing on AMD GPUs on 10.14.4+ (based on vladie's patch)

#### v1.3.0
- Fixed custom connector support for Radeon GPUs, thx @lwfitzgerald
- Added `disable-gfx-submit` property to back `ngfxsubmit=0` boot argument
- Added GuC firmware loading patch for latest SKL+ drivers
- Allow loading on 10.15 without `-lilubetaall`
- Disabled NVIDIA performance fix on 10.15, as it now is built-in
- Enable HDMI 2.0 patches on 10.14+ (Use at own risk in case of undiscovered change)
- Added CFL graphics kernel panic workaround on 10.14.4+
- Added infinite loop fix when calculating dividers for Intel HDMI connections on SKL, KBL and CFL platforms.
- Added driver support for onboard LSPCON chips to enable DisplayPort to HDMI 2.0 output on Intel IGPUs (by @0xFireWolf)

#### v1.2.9
- Added AMD Radeon VII to detected list
- Disabled automatic framebuffer usage on Polaris GPUs
- Added `preserve-names` device property to preserve current GPU names
- Added `AppleVPA` patching support, replaces NoVPAJpeg.kext (thx CMMChris and uglyJoe)

#### v.1.2.8
- Added KBL graphics kernel panic workaround on 10.14.4+
- Added IGPU DPCD link incompatible rate patch (thanks @0xFireWolf)

#### v1.2.7
- Added more IGPU device-ids to detected list

#### v1.2.6
- Added `applbkl=0` boot argument to termporarily disable `AppleBacklight` patching code
- Fixed AMD Verde CAIL injection logic
- Fixed breaking backlight on Apple hardware, on laptops with AMD GPUs, and on 10.11 or earlier
- Changed CFL backlight patches to enable by default on CFL drivers only (avoids issues with faux "KBL" 8xxx CPUs)

#### v1.2.5
- Added support for specifying `agpmod` in external GPU properties
- Added fatal error on `agpmod=cfgmap` on 10.14 and newer, which had no effect since 10.13.4
- Added `igfxcflbklt` boot argument and `enable-cfl-backlight-fix` property to fix CFL backlight
- Added max backlight frequency override via  `max-backlight-freq` IGPU property on CFL
- Added `framebuffer-camellia` and `framebuffer-flags` patching
- Added `AppleBacklight` patching code (based on `AppleBacklightFixup` by `hieplpvip`)
- Added NVIDIA HDMI enabling code for firmwares that disable it by default (thanks @Fraxul)
- Enabled CFL backlight patches by default on laptops with CFL graphics
- Fixed SNB IGPU HDMI automatic patching
- Fix multiple AMD GPU support improperly handling configuration properties

#### v1.2.4
- Added platform list dumping to ioreg (at IOService:/IOResources/WhateverGreen), debug build only with -igfxfbdump
- Fixed 10.14.1 IGPU KBL/CFL support without external GPU
- Fixed warning about legacy processors (e.g. Xeon)
- Fixed the support for providing custom names on `Radeon RX` models

#### v1.2.3
- Added `framebuffer-cursormem` IGPU patch support (Haswell specific)
- Added `framebuffer-conX-XXXXXXXX-alldata` IGPU patch support (platform-id specific conX-alldata)
- Changed AGDP patch defaults to vit+pikera patch
- Fixed semantic patches for Coffee Lake when it pretends to be Kaby

#### v1.2.2
- Added `framebuffer-conX-alldata` IGPU patch support
- Fixed automatic frame selection with `-wegnoegpu` boot-arg or `disable-external-gpu` IGPU property (Lilu 1.2.7 or newer)

#### v1.2.1
- Added `-wegnoegpu` boot-arg and `disable-external-gpu` IGPU property to kill external GPU
- Fixed IGPU framebuffer patches (requires Lilu 1.2.6)
- Fixed `-shikioff` not working (note, `-liluuseroff` may be more handy)
- Fixed loading on 10.8 and 10.9
- Fixed device property reading for AMD CAIL overrides
- Recovered GuC loading for internal usage (do NOT use, causes freezes and crashes)
- Disabled HDMI 2.0 by default, use `-cdfon` boot-arg or `enable-hdmi20` IGPU/GFX0 property to enable

#### v1.2.0
- Merged GPU kexts into one (AMD, Intel, NVIDIA) including H/W acceleration (Shiki)
- Added binary and structural Intel framebuffer patches (thx to headkaze)
- Added Intel CFL support
- Fixed certain AMD multimonitor issues
- Enabled 10.14 support by default

#### v1.1.8
- Added more GPU models to automatic detection
- Hardened solved symbol verification to avoid panics with broken kext cache
- Fixed compiling with older Xcode

#### v1.1.7
- Added `-radgl` boot argument to disable Metal support
- Disabled a few more unnecessary patches for 10.13.4

In order to boot with `-radgl` you may need to set the defaults:
```
sudo defaults write /Library/Preferences/com.apple.CoreDisplay useMetal -boolean no
sudo defaults write /Library/Preferences/com.apple.CoreDisplay useIOP -boolean no
```

#### v1.1.6
- Ensure proper GFX0 and HDAU renaming

#### v1.1.5
- Added more GPU models to automatic detection (including new names from 10.13.4)
- Added automatic `CFG_USE_AGDC` disabling to avoid constant issues with 4K display sleep and broken HDMI/DP ports

#### v1.1.4
- Added automatic screen boot artifact correction (`-radlogo` is no longer necessary and is removed)
- Added automatic `CFG_FB_LIMIT` correction to avoid issues on several Polaris GPUs on 10.13

#### v1.1.3
- Added more GPU models to automatic detection
- Added `-rad4200` option to fix freezes and possibly improve the performance of Radeon Pro 560 on 10.13

#### v1.1.2
- Enabled the kext in installer and recovery by default
- Improved controller start debugging

#### v1.1.1
- Added more GPU models to automatic detection
- Rename GPU name to GFX0 only if it does not start with GFX prefix
- Added `-radnoaudio` boot argument to avoid DP/HDMI audio autoenabling
- Added `no-audio-autofix` gpu controller property to avoid DP/HDMI audio autoenabling

#### v1.1.0
- Requires Lilu 1.2.0 or newer
- Added more GPU models to automatic detection
- Fixed GPU controller name to GFX0 if not already changed
- Fixed HDMI audio initialising even when the kext is disabled

#### v1.0.4
- Added more GPU models to automatic detection
- Initial Vega series support
- Fixed minor issues in HDMI audio enabling code

#### v1.0.3
- Fixed `radpg` bit mask working incorrectly
- Added manual tuning of `aty_config`, `aty_properties`, and `cail_properties` via ACPI
- Changed HDMI audio layout-id to match HDEF layout-id if available

#### v1.0.2
- Added more GPU models to automatic detection
- Added basic automatic HDMI audio correction
- Fixed WhateverName os version requirements
- Disabled connector ordering by default (use `connector-priority` if needed)
- Changed `connector-priority` default type importance
- Added version info for easier debugging

#### v1.0.1
- Added more GPU models to automatic detection
- Disabled DVI transmitter changes by default (use `-raddvi` boot-arg)
- Added IOAccelDeviceGetName correction
- Added libWhateverName.dylib with an app for GPU GL/Compute/Metal engine name correction

#### v1.0.0
- Initial release
