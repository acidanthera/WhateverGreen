# Superceded plug-ins nuances  
  
CoreDisplayFixup.kext is deprecated because it is equivalent to use `enable-hdmi20`= `01000000` in `DeviceProperties` or boot-arg: `-cdfon`  
  
AzulPatcher4600.kext is deprecated because it is equivalent to use `framebuffer-patch` in `DeviceProperties`  
For example, patch fCursorMemorySize=9MB:  
`framebuffer-patch-enable` = `01000000`  
`framebuffer-cursormem` = `00009000`  
[Full equivalent AzulPatcher4600.kext](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/AzulPatcher4600_equivalent.plist)  
  
IntelGraphicsDVMTFixup.kext is deprecated because it is equivalent to use `framebuffer-patch` in `DeviceProperties`  
`framebuffer-patch-enable` = `01000000`  
`framebuffer-fbmem` = `00009000`  
`framebuffer-stolenmem` = `00003001`  
  
EnableLidWake.kext is deprecated because it is equivalent to setting `FBAlternatePWMIncrement1/2` bit in flags.  
  
AppleBacklightFixup.kext is deprecated.  
In case of custom AppleBacklight profile if is necessary, add correct profile in `DeviceProperties` `applbkl-name` and `applbkl-data`  
  
NoVPAJpeg.kext is deprecated.  
In case its functionality is needed, add the following boot arguments:  
`shikigva=32 shiki-id=Mac-7BA5B2D9E42DDD94`
