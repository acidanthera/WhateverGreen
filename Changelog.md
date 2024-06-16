 WhateverGreen Changelog
=======================
#### v1.6.7
- Added constants for macOS 15 support

#### v1.6.6
- Extended the Backlight Registers Alternative Fix (BLT) submodule to support both KBL and CFL platforms. (by @0xFireWolf)
- Revised the Backlight Registers Fix (BLR) submodule to make it compatible with the Backlight Smoother (BLS) on KBL platforms. (by @0xFireWolf)

#### v1.6.5
- Added constants for macOS 14 support
- Added a new boot argument `-igfxblt` to revert the optimizations done by the compiler in backlight related functions, fixing the 3-minute dark screen issue and making Backlight Smoother (BLS) work on mobile Coffee Lake platforms running macOS 13.4 or later. (by @0xFireWolf)

#### v1.6.4
- Fixed Radeon RX 5500 XT identification regression

#### v1.6.3
- Added various GPU identifiers from different Macs
- Added `disable-telemetry-load` to disable iGPU telemetry loading that may cause a freeze during startup on certain laptops such as Chromebooks.

#### v1.6.2
- Added W7170M/S7100X ID

#### v1.6.1 
- Improved Skylake graphics spoofing support by removing profile 2 from VTSupportedProfileArray on macOS 13+, thanks @abenraj and @dhinakg

#### v1.6.0
- Added constants required for macOS 13 update
- Added Skylake graphics spoofing support on macOS 13+ by @dhinakg
- Modified brightness change requests to replace previous requests instead of queuing

#### v1.5.9
- Add AMD prefix for all Radeon cards to follow latest Apple naming scheme

#### v1.5.8
- Inverted logic for GVA support, which is now disabled by default and can be enabled by `enable-gva-support`.

#### v1.5.7
- Fixed maximum backlight level on Ice Lake IGPUs

#### v1.5.6
- Fixed deprecated code in unfairgva

#### v1.5.5
- Changed the default delay of optimizing display data buffer allocations from 0 to 1 second to fix the issue that both internal and external displays flicker on some Ice Lake-based laptops. (Thanks @m0d16l14n1)
- Disabled the black screen fix on Ice Lake platforms as it is only applicable to SKL/KBL/CFL/CML platforms.
- Disabled the force complete modeset submodule on Ice Lake platforms as HDMI/DVI connections are not supported by the driver.
- Added AMD Radeon RX 5000 series PWM backlight control support. (Thanks to @kingo132)

#### v1.5.4
- Added the fix for the short period garbled screen after the system boots on Ice Lake platforms. (by @0xFireWolf, also thanks @m0d16l14n1 and @kingo132)

#### v1.5.3
- Added `no-gfx-spoof` to avoid forcing `device-id` values from PCI I/O.
- Added the backlight smoother submodule that makes brightness transitions smoother on Intel IVB+ platforms. (by @0xFireWolf)
- MMIO Register Access submodules are now available on Intel IVB+ platforms. (by @0xFireWolf)
- Improved ASUS-made AMD R9 380 GPU identification
- Fixed `applbkl` property with `<00 00 00 00>` value failing to disable backlight patches
- *Note:* This release requires Lilu v1.5.6 or later.

#### v1.5.2
- Added `device-id` spoofing support for AMD graphics

#### v1.5.1
- Added constants required for macOS 12 update
- Added Intel Arrandale graphics support on 10.6 and 10.7 64-bit

#### v1.5.0
- Fixed AMD WX-4170 name for 67E0 device id
- Added NVIDIA driver error logging with `-ngfxdbg`

#### v1.4.9
- Added per-GPU disabling API: inject `disable-gpu` to disable
- Added per-GPU disabling kernel version specification: inject `disable-gpu-min` / `disable-gpu-max` to select kernel version to disable (inclusive range)
- Added IGPU disabling API: inject `disable-gpu` to disable or use `-wegnoigpu` boot argument
- Optimised Rocket Lake startup as IGPU is unsupported

#### v1.4.8
- Fixed debug messages from cursor manipulation with NVIDIA GPUs on macOS 11

#### v1.4.7
- Implemented `unfairgva` device property (use `<01 00 00 00>` value for MP5,1 to enable streaming DRM)

#### v1.4.6
- Backlight registers fix replaces the previous Coffee Lake backlight fix and is now available on Intel Ice Lake platforms.
- Boot argument `igfxcflbklt=1` as well as device property `enable-cfl-backlight-fix` are deprecated and replaced by `-igfxblr` and `enable-backlight-registers-fix`.
- Add max pixel clock override through `-igfxmpc` boot argument or `enable-max-pixel-clock-override` and `max-pixel-clock-frequency` device properties
- Moved PNLF samples to OpenCore

#### v1.4.5
- Enabled loading in safe mode (mainly for AGDP fixes)
- Resolved an issue that the maximum link rate fix is not working properly on Intel Comet Lake platforms. (Thanks @CoronaHack)
- Allowed enabling `igfxrpsc` on Comet Lake
- Fixed failed to route IsTypeCOnlySystem warning from Skylake to Ice Lake

#### v1.4.4
- Extended the maximum link rate fix: Now probe the rate from DPCD automatically and support Intel ICL platforms. (by @0xFireWolf)
- Fixed an issue that LSPCON driver causes a page fault if the maximum link rate fix is not enabled. (by @0xFireWolf)

#### v1.4.3
- Added CFL and CML P630
- Added MacKernelSDK with Xcode 12 compatibility
- Fixed loading on macOS 10.11 and earlier

#### v1.4.2
- Fixed `disable-external-gpu` (`-wegnoegpu`) on some systems
- Disabled RPS control patch by default due to a bug in 10.15.6 IGPU drivers
- Replaced `igfxnorpsc=1` with `igfxrpsc=1` to opt-in RPS control patch
- Support all valid Core Display Clock (CDCLK) frequencies to avoid the kernel panic of "Unsupported CD clock decimal frequency" on Intel ICL platforms. (by @0xFireWolf)
- Fix the kernel panic caused by an incorrectly calculated amount of DVMT pre-allocated memory on Intel ICL platforms. (by @0xFireWolf)

#### v1.4.1
- Added `igfxmetal=1` boot argument (and `enable-metal` property) to enable Metal on offline IGPU
- Fixed applying patches on CometLake IGPUs, thx @apocolipse
- Added constants required for 11.0 update
- Added the use of RPS control for all the command streamers on IGPU (disabled via `igfxnorpsc=1`)
- Add `-igfxvesa` to disable Intel Graphics acceleration.
- Fix black screen on igfx since 10.15.5
- Add workaround for rare force wake timeout panics on Intel KBL and CFL.
- Add Intel Westmere graphics support.

#### v1.4.0
- Added 0x3EA6, 0x8A53, 0x9BC4, 0x9BC5, 0x9BC8 IGPU device-id
- Fixed `framebuffer-conX-alldata` patching regression
- Added `disable-hdmi-patches` device property alias to `-igfxnohdmi`

#### v1.3.9
- Added `igfxdumpdelay` boot argument to delay `-igfxdump` in ms
- Partially fix ICL framebuffer patching
- Add support to injecting `Force_Load_FalconSMUFW` from OpenCore
- Disabled automatic enabling of GVA for Polaris on 10.13 and lower
- Replaced -radnogva argument with radgva=0/1 to force GVA for Polaris
- Added `wegtree=1` boot argument (`rebuild-device-tree` property) to force device renaming on Apple FW

#### v1.3.8
- Added `igfxfw=2` boot argument and `igfxfw` IGPU property to load Apple GuC firmware
- Added `igfxpavp=1` boot argument (and `igfxpavp` property) to force enable PAVP output
- Added `igfxfcms=1` boot argument (and `complete-modeset` property) on Skylake and Apple
- Improved performance with Lilu 1.4.3 APIs
- Added `-igfxfbdbg` boot argument to debug IGPU framebuffer (debug builds only)
- Added `igfxagdc=0` boot argument and `disable-agdc` IGPU property to disable AGDC
- Added `igfxonln=1` boot argument and `force-online` IGPU property force online status for all displays
- Added `igfxonlnfbs=MASK` boot argument and `force-online-framebuffers` IGPU property to override display status

#### v1.3.7
- Improved the maximum link rate fix: Now correct the value read from extended DPCD as well. (by @0xFireWolf)
- Improved firmware loading handling on 10.15.4 (may fix booting issues on KBL+)
- Improved support for Comet Lake IGPUs (thx @stormbirds)

#### v1.3.6
- Enabled CoreLSKD streaming patches by default for AMD hardware DRM on Ivy Bridge
- Repurposed 64 bit for FP 2.x streaming hardware accelerated streaming patches (can be used as `shikigva=80`)
- Fixed accelerator name update logic for X4xxx kexts
- Fixed Verde IOGVACodec injection to make hardware video decoder work
- Enable software TV+ decoding on all CPUs without IGPU (`shikigva=256`)
- Added HEVC capabilities to AMD6 decoders for all GPUs (disabled by `-radnogva` or `disable-gva-support`)
- Added HW decoder device-id spoofing via `-radcodec` boot-arg, by @osy86

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
