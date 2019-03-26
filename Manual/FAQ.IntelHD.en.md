# Intel® HD Graphics FAQ  
  
Discussion: [Russian](https://www.applelife.ru/threads/intel-hd-graphics-3000-4000-4400-4600-5000-5500-5600-520-530-630.1289648/), [English](https://www.insanelymac.com/forum/topic/334899-intel-framebuffer-patching-using-whatevergreen/)  
  
**Intel® HD Graphics** are video cards built into Intel processors. Not all processors are equipped with integrated graphics. To find out if yours is - use [this table](https://en.wikipedia.org/wiki/List_of_Intel_graphics_processing_units) or see the characteristics of your processor on Intel’s website. For example, the table shows Intel® HD 4600 integrated graphics for [i7-4770k](https://ark.intel.com/products/75123/Intel-Core-i7-4770K-Processor-8M-Cache-up-to-3_90-GHz), whereas the [i7-4930k](https://ark.intel.com/products/77780/Intel-Core-i7-4930K-Processor-12M-Cache-up-to-3_90-GHz) has none.  
  
macOS has quite acceptable support for Intel® HD Graphics 2000 (Sandy Bridge) and newer. For older generation graphics see the appropriate threads / instructions ( [Intel HD in Arrandale processors](https://www.insanelymac.com/forum/topic/286092-guide-1st-generation-intel-hd-graphics-qeci/?hl=%20vertek) , [GMA950](https://www.applelife.ru/threads/intel-gma950-32bit-only.22726/) , [GMA X3100](https://www.applelife.ru/threads/intel-gma-x3100-zavod.36617/)). Attention, not all Intel graphics cards can be successfully enabled in macOS (more below).  
  
If you use a discrete graphics card (AMD or NVIDIA), having integrated Intel graphics enabled is still useful, as it can be used in offline mode (also known as ["empty framebuffer", 0 connectors](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/)) for hardware encoding and decoding of media files and so forth.  
  
The general concept of enabling Intel graphics cards:  
1. Correct the names of all related devices (`IGPU` for the video card itself, `IMEI` for the Intel Management Engine).  
2. If necessary, fake the `device-id`'s of the video card and Intel Management Engine to compatible ones.  
3. Specify the correct framebuffer (`AAPL,ig-platform-id` or `AAPL,snb-platform-id`) describing available outputs and other properties of the video card.  
4. Add some other additional properties to devices related to Intel® HD Graphics.  
  
At this point, paragraphs 1 and 4 are automated by [WhateverGreen](https://github.com/acidanthera/WhateverGreen). It works in OS X 10.8 and later, and greatly simplifies graphics enabling in macOS.  
  
## General recommendations  
1. Select the required memory amount in BIOS for the needs of the video system (DVMT Pre-Allocated): 32MB, 64MB, 96MB, etc. depending on the framebuffer.  
For the total amount of memory DVMT (DVMT Total) select: MAX.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/bios.png)  
Some faulty BIOSes show a higher value, but actually allocate less. In such cases select a value a step higher. This is common with Dell laptops, their BIOS reports 64MB, but actually allocates 32MB and there is no way to change it. Such case will be shown in this manual.  
2. Add [Lilu.kext](https://github.com/vit9696/Lilu/releases) and [WhateverGreen.kext](https://github.com/acidanthera/WhateverGreen/releases)(hereinafter referred to as the **WEG**) to Clover's "Other" folder.  
3. Remove (if used previously) these kexts:  
— IntelGraphicsFixup.kext  
— NvidiaGraphicsFixup.kext  
— CoreDisplayFixup.kext  
— Shiki.kext  
— IntelGraphicsDVMTFixup.kext  
— AzulPatcher4600.kext  
— AppleBacklightFixup.kext,  
— FakePCIID_Intel_HD_Graphics.kext  
— FakePCIID_Intel_HDMI_Audio.kext  
— and FakePCIID.kext (if there are no other FakePCIID plugins)  

4. Turn off all Clover's graphic injects (and I mean turn off and not comment out).  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/Clover1.png)  
5. Turn off Clover's DSDT fixes:  
— `AddHDMI`  
— `FixDisplay`  
— `FixIntelGfx`  
— `AddIMEI`  
— `FixHDA` 
6. Turn off `UseIntelHDMI`.  
7. Disable `Devices` - `Inject` (usually this parameter is absent and that is good, but if it is there, turn off or delete).  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/Clover2.png)  
  
8. Delete the `-disablegfxfirmware` boot argument.  
9. Delete or comment # `FakeID` for `IntelGFX` and `IMEI`.  
10. Delete or comment # `ig-platform-id`.  
11. Completely remove `Arbitrary`, `AddProperties`, as well as IGPU, IMEI, HDEF and HDMI audio definitions from `SSDT` and `DSDT` (if you added them).  
12. Delete or disable binary patches DSDT: `GFX0 to IGPU`, `PEGP to GFX0`, `HECI to IMEI`, `MEI to IMEI`, `HDAS to HDEF`, `B0D3 to HDAU`.  
  
To inject properties, use `Properties` section in config.plist.  
Only the following may be added:  
— `AAPL,ig-platform-id` or `AAPL,snb-platform-id` framebuffer  
— `device-id` for `IGPU` (if faking is necessary)  
— `device-id` for `IMEI` (if faking is necessary)  
— properties for patches (if necessary)  
And `layout-id` for `HDEF` (In more detail [AppleALC](https://github.com/acidanthera/AppleALC). HDEF device locations `PciRoot`, [gfxutil](https://github.com/acidanthera/gfxutil) may be used: `gfxutil -f HDEF`)  
  
Adding these is not mandatory. An example: the default framebuffer is good enough or it is set with a boot argument (boot-arg), and faking the `device-id` is not required.  
The bytes in `Properties` must be put in reversed order. For example: `0x0166000B` would be put in as `0B006601`.  
  
Common `Properties` templates for `IGPU` and `IMEI` sections are described below for each processor family separately.  
**Attention!** Do not leave empty property values. For example, if a certain property is not required, delete the entire line! Remove `PciRoot` dictionary entirely if it has no properties.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/basic.png)
  
**Choosing a framebuffer.** First try the recommended one. If it is not successful, then try the others one by one, excluding the "empty framebuffers" (0 connectors), which are described in a separate [topic](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/).  
When looking for a suitable framebuffer you can set it with a boot argument (boot-arg), in which case the framebuffer set in the `Properties` section is ignored.
For example: `igfxframe=0x0166000B`  
**Attention!** Unlike in `Properties` the normal byte order and the `0x` prefix are to be used.  
— If a framebuffer is not specified explicitly in any way, the default framebuffer will be injected.  
— If a framebuffer is not set and the system has a discrete graphics card, an "empty framebuffer" will be injected.  
  
  
## Intel HD Graphics 2000/3000 ([Sandy Bridge](https://en.wikipedia.org/wiki/Sandy_Bridge) processors)  
Supported from Mac OS X 10.7.x to macOS 10.13.6. The instructions are for OS X 10.8.x - macOS 10.13.6. On older operating systems follow the "ancient ways". On newer operating systems these are not supported. [But if you really want to - read this.](https://applelife.ru/posts/744431) Metal support is absent.  
SNB framebuffer list:  
— 0x00010000 (mobile, 4 connectors, no fbmem)  
— 0x00020000 (mobile, 1 connectors, no fbmem)  
— 0x00030010 (desktop, 3 connectors, no fbmem)  
— 0x00030020 (desktop, 3 connectors, no fbmem)  
— 0x00030030 (desktop, 0 connectors, no fbmem)  
— 0x00040000 (mobile, 3 connectors, no fbmem)  
— 0x00050000 (desktop, 0 connectors, no fbmem)  
  
<details>
<summary>Spoiler: SNB connectors</summary>
AppleIntelSNBGraphicsFB.kext  
  
ID: SNB0 0x10000, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 1 MB, MAX STOLEN: 0 bytes, MAX OVERALL: 1 MB (1064960 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 2, PortCount: 4, FBMemoryCount: 0  
[5] busId: 0x03, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000009 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000400, flags: 0x00000009 - ConnectorDP  
05030000 02000000 30000000  
02050000 00040000 07000000  
03040000 00040000 09000000  
04060000 00040000 09000000  
  
ID: SNB1 0x20000, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 1 MB, MAX STOLEN: 0 bytes, MAX OVERALL: 1 MB (1052672 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 2, PortCount: 1, FBMemoryCount: 0  
[5] busId: 0x03, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
05030000 02000000 30000000  

ID: SNB2 0x30010 or 0x30020, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 1 MB, MAX STOLEN: 0 bytes, MAX OVERALL: 1 MB (1060864 bytes)  
Camelia: CameliaUnsupported (255), Freq: 0 Hz, FreqMax: -1 Hz  
Mobile: 0, PipeCount: 2, PortCount: 3, FBMemoryCount: 0  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000009 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
02050000 00040000 07000000  
03040000 00040000 09000000  
04060000 00080000 06000000  
  
ID: SNB3 0x30030, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 0 bytes, MAX STOLEN: 0 bytes, MAX OVERALL: 0 bytes  
Camelia: CameliaUnsupported (255), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: SNB4 0x40000, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 1 MB, MAX STOLEN: 0 bytes, MAX OVERALL: 1 MB (1060864 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 2, PortCount: 3, FBMemoryCount: 0  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000009 - ConnectorDP  
01000000 02000000 30000000  
02050000 00040000 07000000  
03040000 00040000 09000000  
  
ID: SNB5 0x50000, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 0 bytes, MAX STOLEN: 0 bytes, MAX OVERALL: 0 bytes  
Camelia: CameliaUnsupported (255), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: SNB6 Not addressible, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 512 KB, MAX STOLEN: 0 bytes, MAX OVERALL: 512 KB  
Camelia: CameliaUnsupported (255), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 1, PortCount: 0, FBMemoryCount: 0  
  
ID: SNB7 Not addressible, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 0 bytes, MAX OVERALL: 1 MB (1589248 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 4, FBMemoryCount: 0  
[1] busId: 0x00, pipe: 0, type: 0x00000400, flags: 0x00000030 - ConnectorDP  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000009 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
01000000 00040000 30000000  
02050000 00040000 07000000  
03040000 00040000 09000000  
04060000 00080000 06000000  
  
Default SNB, DVMT: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 4, FBMemoryCount: 0  
[1] busId: 0x00, pipe: 0, type: 0x00000400, flags: 0x00000030 - ConnectorDP  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000009 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
01000000 00040000 30000000  
02050000 00040000 07000000  
03040000 00040000 09000000  
04060000 00080000 06000000  
  
Note, that without AAPL,snb-platform-id the following models will use predefined IDs:  
Mac-94245B3640C91C81 -> SNB0 (MacBookPro8,1)  
Mac-94245AF5819B141B -> SNB0  
Mac-94245A3940C91C80 -> SNB0 (MacBookPro8,2)  
Mac-942459F5819B171B -> SNB0 (MacBookPro8,3)  
Mac-8ED6AF5B48C039E1 -> SNB2 (Macmini5,1)  
Mac-7BA5B2794B2CDB12 -> SNB2 (Macmini5,3)  
Mac-4BC72D62AD45599E -> SNB3 (Macmini5,2) -> no ports  
Mac-742912EFDBEE19B3 -> SNB4 (MacBookAir4,2)  
Mac-C08A6BB70A942AC2 -> SNB4 (MacBookAir4,1)  
Mac-942B5BF58194151B -> SNB5 (iMac12,1) -> no ports  
Mac-942B5B3A40C91381 -> SNB5 -> no ports  
Mac-942B59F58194171B -> SNB5 (iMac12,2) -> no ports   
</details>
  
####
*Recommended framebuffers:* for desktop - 0x00030010 (default); for laptop - 0x00010000 (default).  

HD2000 doesn't work as a full-featured graphics card in macOS, but you can (and should) use it with an "empty framebuffer" (0 connectors) for [IQSV](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/). Only the HD3000 can work with a display.  
  
Sandy Bridge doesn't usually require framebuffer specifying, the default framebuffer for your board-id will automatically be used. Specifying the framebuffer is required if you are using a non Sandy Brige Mac model.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/snb.png)  
Keep in mind that the framebuffer property name for `Sandy Bridge` — `AAPL,snb-platform-id` — differs from others IGPUs.  
  
Desktops require a fake `device-id` `26010000` for `IGPU`:  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/snb_igpu.png)  
(for an "empty framebuffer" a different device-id is required, more in this [thread](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/))  
  
*Attention!* If you are using a motherboard with a  [7 series](https://ark.intel.com/products/series/98460/Intel-7-Series-Chipsets) chipset, it is necessary to fake the `device-id` `3A1C0000` for `IMEI`:  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/snb_imei.png)  


## Intel HD Graphics 2500/4000 ([Ivy Bridge](https://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture)) processors)  
Supported since OS X 10.8.x  
Capri framebuffer list:  
— 0x01660000 (desktop, 4 connectors, 24 MB)  
— 0x01620006 (desktop, 0 connectors, no fbmem, 0 bytes)  
— 0x01620007 (desktop, 0 connectors, no fbmem, 0 bytes)  
— 0x01620005 (desktop, 3 connectors, 16 MB)  
— 0x01660001 (mobile, 4 connectors, 24 MB)  
— 0x01660002 (mobile, 1 connectors, 24 MB)  
— 0x01660008 (mobile, 3 connectors, 16 MB)  
— 0x01660009 (mobile, 3 connectors, 16 MB)  
— 0x01660003 (mobile, 4 connectors, 16 MB)  
— 0x01660004 (mobile, 1 connectors, 16 MB)  
— 0x0166000A (desktop, 3 connectors, 16 MB)  
— 0x0166000B (desktop, 3 connectors, 16 MB)  
  
<details>
<summary>Spoiler: Capri connectors</summary>
AppleIntelFramebufferCapri.kext  
  
ID: 01660000, STOLEN: 96 MB, FBMEM: 24 MB, VRAM: 1024 MB, Flags: 0x00000000  
TOTAL STOLEN: 24 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 72 MB, MAX OVERALL: 73 MB (77086720 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 3, PortCount: 4, FBMemoryCount: 3  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000400, flags: 0x00000007 - ConnectorDP  
01000000 02000000 30000000  
02050000 00040000 07000000  
03040000 00040000 07000000  
04060000 00040000 07000000  
  
ID: 01620006, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 256 MB, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 0 bytes, MAX STOLEN: 0 bytes, MAX OVERALL: 0 bytes  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 01620007, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 256 MB, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 0 bytes, MAX STOLEN: 0 bytes, MAX OVERALL: 0 bytes  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 01620005, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 32 MB, MAX OVERALL: 33 MB (34615296 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 2, PortCount: 3, FBMemoryCount: 2  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000011 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
02050000 00040000 11000000  
03040000 00040000 07010000  
04060000 00040000 07010000  
  
ID: 01660001, STOLEN: 96 MB, FBMEM: 24 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 24 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 72 MB, MAX OVERALL: 73 MB (77086720 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 4, FBMemoryCount: 3  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01000000 02000000 30000000  
02050000 00080000 06000000  
03040000 00040000 07010000  
04060000 00040000 07010000  
  
ID: 01660002, STOLEN: 64 MB, FBMEM: 24 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 24 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 24 MB, MAX OVERALL: 25 MB (26742784 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
01000000 02000000 30000000  
  
ID: 01660008, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 48 MB, MAX OVERALL: 49 MB (51916800 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01000000 02000000 30000000  
02050000 00040000 07010000  
03040000 00040000 07010000  
  
ID: 01660009, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 48 MB, MAX OVERALL: 49 MB (51916800 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01000000 02000000 30000000  
02050000 00040000 07010000  
03040000 00040000 07010000  
  
ID: 01660003, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 32 MB, MAX OVERALL: 33 MB (34619392 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 2, PortCount: 4, FBMemoryCount: 2  
[5] busId: 0x03, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000407 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000081 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000400, flags: 0x00000081 - ConnectorDP  
05030000 02000000 30000000  
02050000 00040000 07040000  
03040000 00040000 81000000  
04060000 00040000 81000000  
  
ID: 01660004, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 16 MB, MAX OVERALL: 17 MB (18354176 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[5] busId: 0x03, pipe: 0, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
05030000 02000000 30020000  
  
ID: 0166000A, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 32 MB, MAX OVERALL: 33 MB (34615296 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 2, PortCount: 3, FBMemoryCount: 2  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
02050000 00040000 07010000  
03040000 00040000 07010000  
04060000 00080000 06000000  
  
ID: 0166000B, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 32 MB, MAX OVERALL: 33 MB (34615296 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 2, PortCount: 3, FBMemoryCount: 2  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
02050000 00040000 07010000  
03040000 00040000 07010000  
04060000 00080000 06000000  
</details>
  
####
*Recommended framebuffers* : for desktop - 0x0166000A (default), 0x01620005; for laptop - 0x01660003 (default), 0x01660009, 0x01660004.  

HD2500 doesn't work as a full-featured graphics card in macOS, but you can (and should) use it with an "empty framebuffer" (0 connectors) for [IQSV](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/). Only the HD4000 can work with a display.  

*Attention!* If you are using a motherboard with a  [6-series](https://ark.intel.com/products/series/98461/Intel-6-Series-Chipsets) chipset, it is necessary to fake the `device-id` `3A1E0000` of `IMEI`:  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/ivy_imei.png)  
  
  
## Intel HD Graphics 4200-5200 ([Haswell](https://en.wikipedia.org/wiki/Haswell_(microarchitecture)) processors)  
Supported since OS X 10.9.x  
Azul framebuffer list:  
— 0x0C060000 (desktop, 3 connectors, 209 MB)  
— 0x0C160000 (desktop, 3 connectors, 209 MB)  
— 0x0C260000 (desktop, 3 connectors, 209 MB)  
— 0x04060000 (desktop, 3 connectors, 209 MB)  
— 0x04160000 (desktop, 3 connectors, 209 MB)  
— 0x04260000 (desktop, 3 connectors, 209 MB)  
— 0x0D260000 (desktop, 3 connectors, 209 MB)  
— 0x0A160000 (desktop, 3 connectors, 209 MB)  
— 0x0A260000 (desktop, 3 connectors, 209 MB)  
— 0x0A260005 (mobile, 3 connectors, 52 MB)  
— 0x0A260006 (mobile, 3 connectors, 52 MB)  
— 0x0A2E0008 (mobile, 3 connectors, 99 MB)  
— 0x0A16000C (mobile, 3 connectors, 99 MB)  
— 0x0D260007 (mobile, 4 connectors, 99 MB)  
— 0x0D220003 (desktop, 3 connectors, 52 MB)  
— 0x0A2E000A (desktop, 3 connectors, 52 MB)  
— 0x0A26000A (desktop, 3 connectors, 52 MB)  
— 0x0A2E000D (desktop, 2 connectors, 131 MB)  
— 0x0A26000D (desktop, 2 connectors, 131 MB)  
— 0x04120004 (desktop, 0 connectors, no fbmem, 1 MB)  
— 0x0412000B (desktop, 0 connectors, no fbmem, 1 MB)  
— 0x0D260009 (mobile, 1 connectors, 99 MB)  
— 0x0D26000E (mobile, 4 connectors, 131 MB)  
— 0x0D26000F (mobile, 1 connectors, 131 MB)  
  
<details>
<summary>Spoiler: Azul connectors</summary>
AppleIntelFramebufferAzul.kext  
  
ID: 0C060000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0C160000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0C260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 04060000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 04160000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 04260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0D260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0A160000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0A260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0A260005, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x0000000F  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 87000000  
02040900 00040000 87000000  
  
ID: 0A260006, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x0000000F  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 87000000  
02040900 00040000 87000000  
  
ID: 0A2E0008, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000021E  
TOTAL STOLEN: 99 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239611904 bytes)  
Camelia: CameliaV1 (1), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 0A16000C, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000001E  
TOTAL STOLEN: 99 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239611904 bytes)  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 0D260007, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000031E  
TOTAL STOLEN: 99 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239616000 bytes)  
Camelia: CameliaDisabled (0), Freq: 1953 Hz, FreqMax: 1953 Hz  
Mobile: 1, PipeCount: 3, PortCount: 4, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 11, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 11, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x06, pipe: 3, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
00000800 02000000 30000000  
01050B00 00040000 07010000  
02040B00 00040000 07010000  
03060300 00080000 06000000  
  
ID: 0D220003, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x00000402  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[3] busId: 0x06, pipe: 8, type: 0x00000400, flags: 0x00000011 - ConnectorDP  
01050900 00040000 87000000  
02040A00 00040000 87000000  
03060800 00040000 11000000  
  
ID: 0A2E000A, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x000000D6  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000011 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 11000000  
01050900 00040000 87000000  
02040A00 00040000 87000000  
  
ID: 0A26000A, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x000000D6  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000011 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 11000000  
01050900 00040000 87000000  
02040A00 00040000 87000000  
  
ID: 0A2E000D, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000040E  
TOTAL STOLEN: 131 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239607808 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 0A26000D, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000040E  
TOTAL STOLEN: 131 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239607808 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 04120004, STOLEN: 32 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0412000B, STOLEN: 32 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0D260009, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000001E
TOTAL STOLEN: 99 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 99 MB, MAX OVERALL: 100 MB (105385984 bytes)  
Camelia: CameliaDisabled (0), Freq: 1953 Hz, FreqMax: 1953 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
00000800 02000000 30000000  
  
ID: 0D26000E, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000031E  
TOTAL STOLEN: 131 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 323 MB, MAX OVERALL: 324 MB (340279296 bytes)  
Camelia: CameliaV2 (2), Freq: 1953 Hz, FreqMax: 1953 Hz  
Mobile: 1, PipeCount: 3, PortCount: 4, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 11, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 11, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x06, pipe: 3, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
00000800 02000000 30000000  
01050B00 00040000 07010000  
02040B00 00040000 07010000  
03060300 00080000 06000000  
  
ID: 0D26000F, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000001E  
TOTAL STOLEN: 131 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 131 MB, MAX OVERALL: 132 MB (138940416 bytes)  
Camelia: CameliaV2 (2), Freq: 1953 Hz, FreqMax: 1953 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
00000800 02000000 30000000  
</details>
  
####
*Recommended framebuffers* : for desktop - 0x0D220003 (default); for laptop - 0x0A160000 (default), 0x0A260005 (recommended), 0x0A260006 (recommended).  
  
For desktop HD4400 and all the mobile fake the `device-id` `12040000` for `IGPU`.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/hsw_igpu.png)  


## Intel HD Graphics 5300-6300 ([Broadwell](https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)) processors)  
Supported since OS X 10.10.2  
BDW framebuffer list:  
— 0x16060000 (desktop, 3 connectors, 32 MB)  
— 0x160E0000 (desktop, 3 connectors, 32 MB)  
— 0x16160000 (desktop, 3 connectors, 32 MB)  
— 0x161E0000 (desktop, 3 connectors, 32 MB)  
— 0x16260000 (desktop, 3 connectors, 32 MB)  
— 0x162B0000 (desktop, 3 connectors, 32 MB)  
— 0x16220000 (desktop, 3 connectors, 32 MB)  
— 0x160E0001 (mobile, 3 connectors, 60 MB)  
— 0x161E0001 (mobile, 3 connectors, 60 MB)  
— 0x16060002 (mobile, 3 connectors, 56 MB)  
— 0x16160002 (mobile, 3 connectors, 56 MB)  
— 0x16260002 (mobile, 3 connectors, 56 MB)  
— 0x16220002 (mobile, 3 connectors, 56 MB)  
— 0x162B0002 (mobile, 3 connectors, 56 MB)  
— 0x16120003 (mobile, 4 connectors, 56 MB)  
— 0x162B0004 (desktop, 3 connectors, 56 MB)  
— 0x16260004 (desktop, 3 connectors, 56 MB)  
— 0x16220007 (desktop, 3 connectors, 77 MB)  
— 0x16260005 (mobile, 3 connectors, 56 MB)  
— 0x16260006 (mobile, 3 connectors, 56 MB)  
— 0x162B0008 (desktop, 2 connectors, 69 MB)  
— 0x16260008 (desktop, 2 connectors, 69 MB)  
  
<details>
<summary>Spoiler: BDW connectors</summary>
AppleIntelBDWGraphicsFramebuffer.kext  
  
ID: 16060000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 160E0000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000706  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 16160000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 161E0000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000716  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 16260000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 162B0000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 16220000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x0000110E  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 160E0001, STOLEN: 38 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000702  
TOTAL STOLEN: 60 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 136 MB, MAX OVERALL: 137 MB (144191488 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00001001 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00003001 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 01100000  
02040A00 00040000 01300000  
  
ID: 161E0001, STOLEN: 38 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000702  
TOTAL STOLEN: 60 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 136 MB, MAX OVERALL: 137 MB (144191488 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00001001 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00003001 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 01100000  
02040A00 00040000 01300000  
  
ID: 16060002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B02  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 16160002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B02  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 16260002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B0A  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 16220002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B0A  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 162B0002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B0A  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 16120003, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00001306  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131612672 bytes)  
Camelia: CameliaV1 (1), Freq: 1953 Hz, FreqMax: 1953 Hz  
Mobile: 1, PipeCount: 3, PortCount: 4, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[3] busId: 0x06, pipe: 3, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
00000800 02000000 30020000  
01050B00 00040000 07050000  
02040B00 00040000 07050000  
03060300 00080000 06000000  
  
ID: 162B0004, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00040B46  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000211 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 11020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 16260004, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00040B46  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000211 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 11020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 16220007, STOLEN: 38 MB, FBMEM: 38 MB, VRAM: 1536 MB, Flags: 0x000BB306  
TOTAL STOLEN: 77 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 153 MB, MAX OVERALL: 154 MB (162017280 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[3] busId: 0x06, pipe: 8, type: 0x00000400, flags: 0x00000011 - ConnectorDP  
01050900 00040000 07050000  
02040A00 00040000 07050000  
03060800 00040000 11000000  
  
ID: 16260005, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000B0B  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050B00 00040000 07050000  
02040B00 00040000 07050000  
  
ID: 16260006, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000B0B  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050B00 00040000 07050000  
02040B00 00040000 07050000  
  
ID: 162B0008, STOLEN: 34 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x00002B0E  
TOTAL STOLEN: 69 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109060096 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 2, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 16260008, STOLEN: 34 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x00002B0E  
TOTAL STOLEN: 69 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109060096 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 2, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
01050900 00040000 07050000  
02040A00 00040000 07050000  
</details>
  
####
*Recommended framebuffers*: for desktop - 0x16220007 (default); for laptop - 0x16260006 (default).  
  
  
## Intel HD Graphics 510-580 ([Skylake](https://en.wikipedia.org/wiki/Skylake_(microarchitecture)))  
Supported since OS X 10.11.4  
SKL framebuffer list:  
— 0x191E0000 (mobile, 3 connectors, 56 MB)  
— 0x19160000 (mobile, 3 connectors, 56 MB)  
— 0x19260000 (mobile, 3 connectors, 56 MB)  
— 0x19270000 (mobile, 3 connectors, 56 MB)  
— 0x191B0000 (mobile, 3 connectors, 56 MB)  
— 0x193B0000 (mobile, 3 connectors, 56 MB)  
— 0x19120000 (mobile, 3 connectors, 56 MB)  
— 0x19020001 (desktop, 0 connectors, no fbmem, 1 MB)  
— 0x19170001 (desktop, 0 connectors, no fbmem, 1 MB)  
— 0x19120001 (desktop, 0 connectors, no fbmem, 1 MB)  
— 0x19320001 (desktop, 0 connectors, no fbmem, 1 MB)  
— 0x19160002 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x19260002 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x191E0003 (mobile, 3 connectors, no fbmem, 41 MB)  
— 0x19260004 (mobile, 3 connectors, no fbmem, 35 MB)  
— 0x19270004 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x193B0005 (mobile, 4 connectors, no fbmem, 35 MB)  
— 0x191B0006 (mobile, 1 connectors, no fbmem, 39 MB)  
— 0x19260007 (mobile, 3 connectors, no fbmem, 35 MB)  

<details>
<summary>Spoiler: SKL connectors</summary>
AppleIntelSKLGraphicsFramebuffer.kext  
  
ID: 191E0000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000050F  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Model name: Intel HD Graphics SKL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 19160000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000090F  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Model name: Intel HD Graphics SKL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 19260000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000090F  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Model name: Intel HD Graphics SKL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 19270000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000090F  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Model name: Intel HD Graphics SKL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 191B0000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000110F  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Model name: Intel HD Graphics SKL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 193B0000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00001187  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Model name: Intel HD Graphics SKL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[2] busId: 0x04, pipe: 10, type: 0x00000800, flags: 0x00000187 - ConnectorHDMI  
[3] busId: 0x06, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
02040A00 00080000 87010000  
03060A00 00040000 87010000  
  
ID: 19120000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000110F  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Model name: Intel HD Graphics SKL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[255] busId: 0x00, pipe: 0, type: 0x00000001, flags: 0x00000020 - ConnectorDummy  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
FF000000 01,000,000 20,000,000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 19020001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 19170001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 19120001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 19320001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 19160002, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00830B02  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel Iris Graphics 540  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
00000800 02000000 98040000  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
  
ID: 19260002, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel Iris Graphics 540  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
00000800 02000000 98040000  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
  
ID: 191E0003, STOLEN: 40 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x002B0702  
TOTAL STOLEN: 41 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 121 MB, MAX OVERALL: 122 MB (128462848 bytes)  
Model name: Intel HD Graphics 515  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000181 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000181 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 81010000  
02040A00 00040000 81010000  
  
ID: 19260004, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B0A  
TOTAL STOLEN: 35 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109588480 bytes)  
Model name: Intel Iris Graphics 550  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
00000800 02000000 98040000  
01050900 00040000 C7010000  
02040A00 00040000 C7010000  
  
ID: 19270004, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel Iris Graphics 550  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
00000800 02000000 98040000  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
  
ID: 193B0005, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0023130A  
TOTAL STOLEN: 35 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 137 MB, MAX OVERALL: 138 MB (145244160 bytes)  
Model name: Intel Iris Pro Graphics 580  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 4, FBMemoryCount: 4  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
[3] busId: 0x06, pipe: 10, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 C7010000  
02040A00 00040000 C7010000  
03060A00 00040000 C7010000  
  
ID: 191B0006, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00131302  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 512 KB, MAX STOLEN: 39 MB, MAX OVERALL: 39 MB (41422848 bytes)  
Model name: Intel HD Graphics 530  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 1, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
00000800 02000000 98040000  
  
ID: 19260007, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00031302  
TOTAL STOLEN: 35 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109588480 bytes)  
Model name: Intel Iris Pro Graphics 580  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 C7010000  
02040A00 00040000 C7010000  
  
Note, that without AAPL,ig-platform-id the following ID is assumed: 19120000  
</details>
  
####
*Recommended framebuffers* : for desktop - 0x19120000 (default); for laptop - 0x19160000 (default).  
  
  
## Intel HD Graphics 610-650 ([Kaby Lake](https://en.wikipedia.org/wiki/Kaby_Lake) processors)  
Supported since macOS 10.12.6  
KBL framebuffer list:  
— 0x591E0000 (mobile, 3 connectors, no fbmem, 35 MB)  
— 0x59160000 (mobile, 3 connectors, no fbmem, 35 MB)  
— 0x59230000 (desktop, 3 connectors, no fbmem, 39 MB)  
— 0x59260000 (desktop, 3 connectors, no fbmem, 39 MB)  
— 0x59270000 (desktop, 3 connectors, no fbmem, 39 MB)  
— 0x59270009 (mobile, 3 connectors, no fbmem, 39 MB)  
— 0x59120000 (desktop, 3 connectors, no fbmem, 39 MB)  
— 0x591B0000 (mobile, 3 connectors, 39 MB)  
— 0x591E0001 (mobile, 3 connectors, no fbmem, 39 MB)  
— 0x59180002 (mobile, 0 connectors, no fbmem, 1 MB)  
— 0x59120003 (mobile, 0 connectors, no fbmem, 1 MB)  
— 0x59260007 (desktop, 3 connectors, 79 MB)  
— 0x59270004 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x59260002 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x591B0006 (mobile, 1 connectors, no fbmem, 39 MB)  

<details>
<summary>Spoiler: KBL connectors</summary>
AppleIntelKBLGraphicsFramebuffer.kext  
  
ID: 591E0000, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000078B  
TOTAL STOLEN: 35 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109588480 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 59160000, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00000B0B  
TOTAL STOLEN: 35 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109588480 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000800, flags: 0x00000187 - ConnectorHDMI  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00080000 87010000  
  
ID: 59230000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B8B  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 115 MB, MAX OVERALL: 116 MB (122171392 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 59260000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B8B  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 115 MB, MAX OVERALL: 116 MB (122171392 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 59270000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B8B  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 115 MB, MAX OVERALL: 116 MB (122171392 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 59270009, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00830B0A  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 115 MB, MAX OVERALL: 116 MB (122171392 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 C7010000  
02040A00 00040000 C7010000  
  
ID: 59120000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000110B  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 115 MB, MAX OVERALL: 116 MB (122171392 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[3] busId: 0x06, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
01050900 00040000 87010000  
02040A00 00040000 87010000  
03060A00 00040000 87010000  
  
ID: 591B0000, STOLEN: 38 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000130B  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 136 MB, MAX OVERALL: 137 MB (144191488 bytes)  
Model name: Intel HD Graphics KBL CRB  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[2] busId: 0x04, pipe: 10, type: 0x00000800, flags: 0x00000187 - ConnectorHDMI  
[3] busId: 0x06, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
02040A00 00080000 87010000  
03060A00 00040000 87010000  
  
ID: 591E0001, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x002B0702  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 115 MB, MAX OVERALL: 116 MB (122171392 bytes)  
Model name: Intel HD Graphics 615  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000181 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000181 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 81010000  
02040A00 00040000 81010000  
  
ID: 59180002, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics KBL  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 59120003, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics KBL  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 59260007, STOLEN: 57 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00830B0E  
TOTAL STOLEN: 79 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 193 MB, MAX OVERALL: 194 MB (203960320 bytes)  
Model name: Intel Iris Plus Graphics 640  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
  
ID: 59270004, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel Iris Plus Graphics 650  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
00000800 02000000 98040000  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
  
ID: 59260002, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel Iris Plus Graphics 640  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
00000800 02000000 98040000  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
  
ID: 591B0006, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00031302  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 512 KB, MAX STOLEN: 39 MB, MAX OVERALL: 39 MB (41422848 bytes)  
Model name: Intel HD Graphics 630  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 1, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
00000800 02000000 98040000  
  
Note, that without AAPL,ig-platform-id the following ID is assumed: 59160000  
</details>
  
####
*Recommended framebuffers*: for desktop - 0x59160000 (default), 0x59120000 (recommended); for laptop - 0x591B0000 (default).  
  
For UHD620 ([Kaby Lake Refresh](https://en.wikipedia.org/wiki/Kaby_Lake#List_of_8th_generation_Kaby_Lake_R_processors)) fake `device-id` `16590000` for `IGPU`.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/kbl-r_igpu.png)  
  
  
## Intel UHD Graphics 630 ([Coffee Lake](https://en.wikipedia.org/wiki/Coffee_Lake) processors)  
Supported since macOS 10.14  
CFL framebuffer list:  
— 0x3EA50009 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3E920009 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3E9B0009 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3EA50000 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3E920000 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3E000000 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3E9B0000 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3EA50004 (mobile, 3 connectors, no fbmem, 58 MB)  
— 0x3E9B0006 (mobile, 1 connectors, no fbmem, 39 MB)  
— 0x3E9B0007 (desktop, 3 connectors, no fbmem, 58 MB)  
— 0x3E920003 (desktop, 0 connectors, no fbmem, 1 MB)  
— 0x3E910003 (desktop, 0 connectors, no fbmem, 1 MB)  

<details>
<summary>Spoiler: CFL connectors</summary>
AppleIntelCFLGraphicsFramebuffer.kext  
  
ID: 3EA50009, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00830B0A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL CRB  
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000001C7 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 C7010000  
02040A00 00040000 C7010000  
  
ID: 3E920009, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0083130A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL CRB  
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[255] busId: 0x00, pipe: 0, type: 0x00000001, flags: 0x00000020 - ConnectorDummy  
[255] busId: 0x00, pipe: 0, type: 0x00000001, flags: 0x00000020 - ConnectorDummy  
00000800 02000000 98000000  
FF000000 01000000 20000000  
FF000000 01000000 20000000  
  
ID: 3E9B0009, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0083130A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL CRB  
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 3EA50000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B0B  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL CRB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 3E920000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000130B  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL CRB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 3E000000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000130B  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL CRB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 3E9B0000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000130B  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL CRB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000187 - ConnectorDP  
00000800 02000000 98000000  
01050900 00040000 87010000  
02040A00 00040000 87010000  
  
ID: 3EA50004, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel Iris Plus Graphics 655  
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
00000800 02000000 98040000  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
  
ID: 3E9B0006, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00131302  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 512 KB, MAX STOLEN: 39 MB, MAX OVERALL: 39 MB (41422848 bytes)  
Model name: Intel Graphics UHD 630  
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 1, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
00000800 02000000 98040000  
  
ID: 3E9B0007, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00801302  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel HD Graphics CFL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[3] busId: 0x06, pipe: 8, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
03060800 00040000 C7030000  
  
ID: 3E920003, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics CFL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 3E910003, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics CFL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
Note, that without AAPL,ig-platform-id the following ID is assumed: 3EA50000  
</details>
  
####
*Recommended framebuffers*: for desktop - 0x3EA50000 (default), 0x3E9B0007 (recommended); for laptop - 0x3EA50009 (default).  
  
If you are using a 9th generation [Coffee Lake Refresh](https://en.wikipedia.org/wiki/Coffee_Lake#List_of_9th_generation_Coffee_Lake_processors) processor, it is necessary to fake `device-id` `923E0000` for `IGPU`. Starting with macOS 10.14.4 the fake is not necessary.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/cfl-r_igpu.png)  
  
<details>
<summary>Spoiler: macOS 10.13 and CFL</summary>
Installing 10.13 on the Coffee Lake platform makes sense only if there is a reason, like having discrete NVIDIA Maxwell / Pascal graphics with absent 10.14 web drivers.  
  
There is a special build of macOS High Sierra 10.13.6 (17G2208), which has native support for Coffee Lake graphics: [link1](https://drive.google.com/file/d/1FyPvo81K8qEXhiEuwDX3mAHMg1ZMdiYS/view), [link2](https://mega.nz/#!GNgDTDob!N3jediG_xrzJPRFi9bQ0MtAFCKbOl33QvQp9tRUSwhQ). This version has no [empty framebuffers](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/) (0 connectors) and there is no dev.id 0x3E91.  
To have [empty framebuffer](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/) (0 connectors) this special build requires installing AppleIntelCFLGraphicsFramebuffer.kext from 10.14 or newer.  
For `IGPU` with dev.id 0x3E91 fake the id with 0x3E92 (`device-id` `923E0000`).  
The 3025 and newer updates with Coffee Lake support are as limited as the initial special build.  
  
And you can always enable UHD630 in macOS 10.13 using the fake `device-id` of a Kaby Lake HD630.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/kbl.png)  
Use the Kaby Lake HD630 framebuffer (specify the framebuffer explicitly!)  
</details>
  
## Adjusting the brightness on a laptop  
Enable Clover DSDT fix `AddPNLF`. Enable `SetIntelBacklight` and `SetIntelMaxBacklight`. A specific value is not necessary - it will be automatically injected according to the processor installed.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/ibl.png)  
  
  
## Digital Audio (HDMI / DVI / DP)  
To enable digital audio it is necessary to set the necessary properties and, usually, patch the connectors.  
To enable audio in general and HDMI in particular use *WEG* along with [AppleALC.kext](https://github.com/acidanthera/AppleALC).  
On 10.10.5 and above, *WEG* automatically changes the `connector-type` of DP (00040000) to HDMI (00080000) if no custom patches are used.  
The actual connection may be of any type (HDMI / DVI / DP), but for the digital audio to function the `connector-type` must explicitly be HDMI.  
  
  
## Custom framebuffer and connectors patches with WEG  
In most cases, no patches are required!  
In 10.14 for SKL and newer it is impossible to obtain information about the framebuffers and connectors directly from the kext binary - it is necessary to dump the binary from memory, so binary framebuffer patches in Clover are impossible. It is, however, possible to make semantic (prefered) and binary patches by using *WEG*. On older OS'es and older IGPU - this works too. By default, the current framebuffer is patched.  
Patches are placed in the `Properties` section of IGPU.  

Example of a binary patch using WEG.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/bin.png)  
  
Example of a semantic patch: HDMI type connector (connector-type=00080000 for connectors with index 1, 2 and 3).  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/con.png)  
  
Example of a semantic patch for bios with DVMT Pre-Alloc 32MB when higher is required. (stolenmem=19MB, fbmem=9MB)  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/sem.png)  
  
[This series of patches](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/AzulPatcher4600_equivalent.plist) are the full equivalent of AzulPatcher4600.kext, for those who have previously used it. (on [some](https://github.com/coderobe/AzulPatcher4600#tested-on) Haswell laptops with framebuffer `0x0A260006` helps to get rid of the artifacts).  
  
  
**All possible WEG custom patches**  
Semantic:  
*framebuffer-patch-enable (enabling the semantic patches in principle)  
framebuffer-framebufferid (the framebuffer that we're patching, the current by default)*  

*framebuffer-mobile  
framebuffer-pipecount  
framebuffer-portcount  
framebuffer-memorycount  
framebuffer-stolenmem  
framebuffer-fbmem  
framebuffer-unifiedmem (VRAM, it is not recommended to use this patch)  
framebuffer-cursormem (Haswell specific)  
framebuffer-flags  
framebuffer-camellia (integrated display controller, this field is relevant only for real macs)*  
  
*framebuffer-conX-enable (enabling patches for connector X)  
framebuffer-conX-index  
framebuffer-conX-busid  
framebuffer-conX-pipe  
framebuffer-conX-type  
framebuffer-conX-flags  
framebuffer-conX-alldata (completely replace the connector)  
framebuffer-conX-YYYYYYYY-alldata (completely replace the connector, if the current framebuffer matches YYYYYYYY)  
Where X is the connector index.  
Alldata patches can patch multiple connectors in sequence by putting them in a single string and specifying the index of a connector to start with. The string length should be a multiple of 12 bytes (the length of a single connector).*  
  
Binary:  
*framebuffer-patchN-enable (enabling patch number N)  
framebuffer-patchN-framebufferid (the framebuffer that we're patching, the current by default)  
framebuffer-patchN-find  
framebuffer-patchN-replace  
framebuffer-patchN-count (the amount of pattern iterations to search for, the default is 1)  
N stands for the number of the patch: 0, 1, 2, ... 9*  
  
Detailed information about framebuffers and connectors can be extracted with [010 Editor](http://www.sweetscape.com/010editor/) and the [IntelFramebuffer.bt](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/IntelFramebuffer.bt) script.  
This information is useful for those who make custom patches.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/ifbt.png)  
In 10.14 for SKL and newer to get a dump suitable for the script you can use the debug version of *WEG* with the  
`-igfxdump` boot-argument. The dump will be saved in the root of the system partition.  
The original and patched dumps can be obtained with IOReg when using a debug version of *WEG* and booting with the  
`-igfxfbdump` boot-argument from `IOService:/IOResources/WhateverGreen`.  
  
  
## VGA support  
In most cases with Intel Skylake and newer it works by default.  
For Ivy Bridge and possibly other generations there are the options to patch your connectors with the following:  
06020000 02000000 30000000 // Option 1  
06020000 01000000 30000000 // Option 2  
On OS X 10.8.2 and newer it is impossible to have VGA on Ivy Bridge systems.  
Hot-plugging VGA usually does not work.  
In case this doesn't help - there are no other known solutions at this time.  
  
  
## EDID  
EDID is usually correctly identified, so no actions are required. In rare cases, EDID needs to be injected manually.  
An EDID dump can be obtained, for example, [with Linux](https://unix.stackexchange.com/questions/114359/how-to-get-edid-for-a-single-monitor). The correct EDID must be put into *AAPL0**0**,override-no-connect* property for `IGPU`, where the second ***0*** stands for the display number.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/edid.png)  
In some cases the EDID dump may be incompatible with macOS and leads to distortions. For some EDID in such cases you can use [this script](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/edid-gen.sh), which corrects a provided EDID and saves it to your desktop.  
  
  
## HDMI in UHD resolution with 60 fps  
Add the `enable-hdmi20` property to `IGPU`, otherwise you will get a black screen.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/hdmi20.png)  
Or instead of this property use the boot-arg `-cdfon`  
  
  
## Disabling a discrete graphics card  
Add the `disable-external-gpu` property to `IGPU`.  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/dGPU_off.png)  
Or instead of this property, use the boot-arg `-wegnoegpu`  
  
  
## Known Issues  
*Compatibility*:  
- Limited cards: HD2000, HD2500 can only be used for [IQSV](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/) (they are used in real Macs only for this), there are no solutions.  
- Intel Pentium / Celeron Graphics can't be enabled, there are no solutions.  
- HDMI black screen on Haswell platforms. Resolved by using *WEG* or macOS 10.13.4 and later.  
- Support for 2 or more displays on Intel Skylake and newer desktops is missing or buggy. In macOS 10.14.x there is an improvement tendency.  
- Displays do not wake up on Intel Skylake desktops and later, connecting via DisplayPort or upgrading to macOS 10.14.x may help.  
  
*Glitches and settings* :  
- HD3000 can sometimes have interface glitches. Since the amount of video memory in Sandy depends on the overall system memory - 8 GB is the minimum to have, but there are no guaranteed solutions. It is also recommended to install [Max TOLUD to Dynamic](https://applelife.ru/posts/595326/) in the BIOS. Perhaps you can benefit from these [patches](https://www.applelife.ru/posts/730496).  
- "8 apples" and the disappearance of the background image with File Vault 2 during the transition from UEFI GOP drivers to macOS drivers (due to incompatible EDID). Partially solved in *WEG*.  
- PAVP freezes (freezes during video playback, broken QuickLook, etc.) are solved with *WEG* at the cost of disabling HDCP.  
- Haswell glitches for some framebuffers are resolved with a semantic `framebuffer-cursormem` patch.  
- In macOS 10.14 оn some laptops with KBL graphics one may face visual artifacts on the gradients. For a temporary solution try to fake IGPU to use SKL drivers.  
- The several minutes black screen upon OS boot with mobile CFL is fixed by *WEG*.  
- The absence in BIOS of an option to change the amount of memory for the frame buffer is resolved with either semantic `framebuffer-stolenmem` and `framebuffer-fbmem` patches, by modifying the BIOS or by manually inputting the values in UEFI Shell. **Otherwise you get a panic.** [Explanation](https://www.applelife.ru/posts/750369)  
  
*Performance and media content* :  
- Compatibility with discrete cards in unsupported configurations (NVIDIA + SNB/SKL/KBL; AMD + IVY), for some applications is fixed by *WEG*. Starting with macOS 10.13.4 the problem is gone.  
- Viewing protected iTunes content is fixed by *WEG*. Starting with macOS 10.12 on Ivy Bridge and newer viewing HD movies on iTunes is not possible without a discrete card.  

A [VDADecoderChecker](https://i.applelife.ru/2018/12/442759_VDADecoderChecker.zip) output for integrated graphics using non-empty connectors must look like this:  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/vda.png)  
  
In case of special IGPU, IMEI and HDEF device locations, [gfxutil](https://github.com/acidanthera/gfxutil) may be used: `gfxutil -f IGPU`, `gfxutil -f IMEI`, `gfxutil -f HDEF`. IGPU and IMEI device locations - usually standardly.  
  
The WWHC team is looking for talented Steve's reincarnations, so if you feel like you might be one - please report to the local looney bin.  
  
