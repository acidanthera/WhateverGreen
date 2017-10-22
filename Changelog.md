WhateverGreen Changelog
=======================
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
