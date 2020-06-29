# Nvidia GeForce FAQs

- _What are the system requirements?_  
while there are no particular limitations, this FAQ does not include the specific information regarding GPUs before Kepler (i.e. older than 6xx series).  
In general it appears to be less convenient to use CPUs newer than Ivy and Haswell with NVIDIA GPUs.  
For GPUs newer than Kepler (e.g. Maxwell or Pascal) you need [NVIDIA Web Driver](http://www.nvidia.com/download/driverResults.aspx/125379/en-us). Use `nv_disable=1` boot argument to install it.

- _What is the general idea?_  
If you have builtin Intel GPU, make sure to rename it to IGPU and enable with connector-less frame first. Then choose a most suitable mac model and install WhateverGreen.kext. It also used for hardware video decoding, please read [Shiki FAQ](./FAQ.Shiki.en.md) carefully to get a good understanding.

- _How to properly choose a mac model?_  
If you have Ivy Bridge or Haswell CPU you should go with iMac13,2 or iMac14,2. Otherwise choose the model you prefer, but keep this in mind:
  - If you have Intel GPU, especially if Ivy Bridge or newer, choose the model (by `board-id`) that has `forceOfflineRenderer` set to YES (true) in /System/Library/PrivateFrameworks/AppleGVA.framework/Versions/A/Info.plist.
  - Models other than iMac13,2 and iMac14,2 require patches, which are though normally automated in WhateverGreen (see below)
- CPUs newer than Haswell require Shiki patches for hardware video decoding (see below).

- _Why should I use Intel GPU with a connector-less frame?_  
Nvidia GPUs newer than 2xx do not implement hardware video decoder in macOS, also starting with 10.13 dual-GPU setups often cause a bootloop. If you absolutely need your IGPU with connector-full frame you will have to determine correct ig-platform-id and  `shikigva=1` bootarg OR a model without `forceOfflineRenderer`.
Read the [Shiki FAQ](./FAQ.Shiki.en.md)

- _How to use Intel GPU with a connector-less frame?_  
Please refer to [Shiki FAQ](./FAQ.Shiki.en.md) for full details. You could use SSDT to rename GFX0 to IGPU by creating a proper IGPU device and setting STA of the existing one to Zero:

```
Scope (GFX0) {
  Name (_STA, Zero)  // _STA: Status
}
```

- _What patches do I need for mac models other than iMac13,2 and iMac14,2?_  
AppleGraphicsDisplayPolicy.kext contains a check against its Info.plist and determines which mode should be used for a specific board-id. It is dependent on the GPU which mode is suitable and is normally determined experimentally. WhateverGreen contains several ways to configure to set power management modes:
  - kext patch enforcing `none` into ConfigMap dictionary for system board-id (agdpmod=cfgmap)
  - kext patch disabling string comparison (`agdpmod=vit9696`, enabled by default)
  - kext patch replacing `board-id` with `board-ix` (`agdpmod=pikera`)

- _What patches do I need for Maxwell or Pascal GPUs?_  
Maxwell GPUs (normally 9xx and some 7xx) no longer supply a correct IOVARendererID to enable hardware video decoder. See more details: [here](https://github.com/vit9696/Shiki/issues/5). You no longer need any changes (e.g. iMac.kext) but WhateverGreen. This fix was added in 1.2.0 branch. Can be switched off by using boot-arg "-ngfxnovarenderer".

- _What patches do processors newer than Haswell need?_  
Apple limits hardware video decoder with NVIDIA to only Haswell and earlier. To get hardware accelerated video decoding you need to patch AppleGVA.framework. To do so you could use [WhateverGreen](https://github.com/acidanthera/WhateverGreen) with `shikigva=4` boot argument. On 10.13 you may currently use a temporary workaround that enables hardware video decoding only for a subset of processes via `shikigva=12` boot argument. Starting with macOS 10.13.4 the problem is gone.

- _What patches do Pascal GPUs need on 10.12?_  
On 10.12 and possibly on 10.13 Pascal GPUs need a team id unlock to avoid glitches like empty transparent windows and so on. This patch is already present in WhateverGreen, and the use of any other kext (e.g. NVWebDriverLibValFix.kext) is not needed.
Can be switched off by using boot-arg "-ngfxlibvalfix".

- _How can I enable digital (HDMI audio)?_  
You must esnure that you do not have any conflicting "fixes" from Clover, SSDT patches, Arbitrary and so on (e.g. FixDisplay, AddHDMI, etc.). AppleALC renames GPU audio device to HDAU, and injects missing layout-id and hda-gfx (starting with onboard-1) properties, and injects audio connectors @0,connector-type - @5,connector-type. Injection can be switched off by using boot-arg "-ngfxnoaudio" or more specific "-ngfxnoaudiocon". You can also use ioreg properties in GPU to disable respective injections: "no-audio-autofix" or "no-audio-fixconn".

- _How can I partially fix Apple Logo during boot?_  
Inject `@X,AAPL,boot-display` GFX0 property with the main screen index instead of X, the value does not matter.

- _Does WhateverGreen fix visual issues on wakeup with Pascal GPUs?_  
Not at the moment. It is also known that HDMI audio may not always work with Pascal GPUs.

- _HDMI audio device only visible after rescan_
[Jamie](https://sourceforge.net/p/nvidiagraphicsfixup/tickets/9/) found out through linux that nvidia graphics on laptops gtx 1060/1070 specifically,
that the audio device is disabled by default. [Bug description](https://bugs.freedesktop.org/show_bug.cgi?id=75985).
He discovered that when the 0x488 magic bit is not set, the gfx device advertises as non-multifunction.
After the bit is set, the device advertises as multi-function.
So, after setting the magic bit, removing the device will cause Linux to re-probe it during the next rescan
taking note at that point that it is a multi-function device
on linux theres a fix use: setpci -s 01:00.0 0x488.l=0x2000000:0x2000000" on mac os he added:

```
Device (PEG0)
{
	Name (_ADR, 0x00010000)  // _ADR: Address
	Method (_PRT, 0, NotSerialized)  // _PRT: PCI Routing Table
	{
		** Store (One, ^GFX0.NHDA)**
		If (PICM)
		{
		    Return (AR01)
		}

	    Return (PR01)
	}
}
```

NHDA is declared here:

```
Scope (_SB.PCI0.PEG0)
{
	Device (GFX0)
	{
		Name (HDAU, Zero)
		OperationRegion (PCI2, SystemMemory, 0xE0100000, 0x0500)
		Field (PCI2, DWordAcc, Lock, Preserve)
		{
			Offset (0x48B),
			,   1,
			NHDA,   1
		}
	}
}
```
