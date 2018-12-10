WhateverGreen Changelog
=======================
#### v1.2.5
- Added support for specifying `agpmod` in external GPU properties
- Added fatal error on `agpmod=cfgmap` on 10.14 and newer, which had no effect since 10.13.4
- Added `igfxcflbklt` boot argument and `enable-cfl-backlight-fix` property to fix CFL backlight
- Added `framebuffer-camellia` and `framebuffer-flags` patching

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
