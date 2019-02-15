# Superceded plug-ins nuances  
  
CoreDisplayFixup.kext is deprecated because it is equivalent to use `enable-hdmi20`= `01000000` in `Devices-Properties` or boot-arg: `-cdfon`  
  
AzulPatcher4600.kext is deprecated because it is equivalent to use `framebuffer-patch` in `Devices-Properties`  
For example, patch fCursorMemorySize=9MB:  
`framebuffer-patch-enable` = `01000000`  
`framebuffer-cursormem` = `00009000`  
[Full equivalent AzulPatcher4600.kext](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/AzulPatcher4600_equivalent.plist)  
  
IntelGraphicsDVMTFixup.kext is deprecated because it is equivalent to use `framebuffer-patch` in `Devices-Properties`  
`framebuffer-patch-enable` = `01000000`  
`framebuffer-fbmem` = `00009000`  
`framebuffer-stolenmem` = `00003001`  
  
EnableLidWake.kext is deprecated because it is equivalent to setting `FBAlternatePWMIncrement1/2` bit in flags.  
  
AppleBacklightFixup.kext is deprecated.  
In case of custom AppleBacklight profile is necessary (e.g. via `AppleBacklightInjector.kext`) the following must be performed to accomplish WhateverGreen compatibility:  
- Create your injector with a correct profile  
- Disable WhateverGreen backlight patches via `applbkl=0` boot argument  
- Specify injector `IOProbeScore` (in injector Info.plist) equal to `5500`  
  
