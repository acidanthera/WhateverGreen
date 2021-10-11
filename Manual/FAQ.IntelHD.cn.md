This translation needs updating, be checked with the English version!  

# 英特尔® 核芯显卡 常见问答

**英特尔® 核芯显卡（下文简称“核显”）** 是内置于 英特尔® 中央处理器中的显卡（译者注：即常说的“英特尔核显”）。并非所有的 英特尔® 中央处理器都配备核显。如需查询您的型号是否配备，查阅 [此处](https://en.wikipedia.org/wiki/List_of_Intel_graphics_processing_units)（译者注：该页面暂无中文版本。上次检查日期: 2019/03/25）或在 英特尔® 官方网站上获取更多信息。如：从上表中可以得知 [i7-4770K](https://ark.intel.com/content/www/cn/zh/ark/products/75123/intel-core-i7-4770k-processor-8m-cache-up-to-3-90-ghz.html) 使用了 英特尔® HD 4600 核显；而 [i7-4930K](https://ark.intel.com/content/www/cn/zh/ark/products/77780/intel-core-i7-4930k-processor-12m-cache-up-to-3-90-ghz.html) 则不配备核显。

macOS 对 HD 2000（Sandy Bridge 微架构）及以上提供了相对完整支持。较早的型号则需参考部分特定教程，如 [适用于 英特尔® Arrandale 微架构处理器的一份指南](https://www.insanelymac.com/forum/topic/286092-guide-1st-generation-intel-hd-graphics-qeci)（译者注：原文发布于以英文交流为主的社区，所以并无中文版本）, [GMA 550](https://www.applelife.ru/threads/intel-gma950-32bit-only.22726)（译者注：原文发布于俄文社区，所以并无中文版本）, [GMA X3100](https://www.applelife.ru/threads/intel-gma-x3100-zavod.36617)（译者注：原文发布于俄文社区，所以并无中文版本）。值得注意的是，并非所有的型号都可以正常工作。（详情如下）

使用独立显卡（如 AMD 或 NVIDIA）时，同时开启 英特尔® 核显仍起作用，其可在脱机模式（下注）中用于硬件编码和媒体文件解码等。

注：“脱机模式”，亦称 [空缓冲帧](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923/)，即无输出接口（译者注：原文发布于俄文社区，所以并无中文版本）

正确启用核显的通常步骤：
1. 修正有关设备的 ACPI 名称（核显自身名为 IGPU，英特尔® 管理引擎（英文缩写: IMEI）名为 IMEI）。
2. 如若必要，将 核显 / IMEI 的 设备 ID 仿冒为合适的型号。
3. 指定正确的缓冲帧。（英文: Framebuffer, 下文简称 缓冲帧 为 FB）（即 AAPL,ig-platform-id（适用于 Ivy Bridge 或更新微架构）或 AAPL,snb-platform-id（仅适用于 Sandy Bridge 微架构）) 一组正确的 FB 应当正确地包含了可用的输出端口以及该核显的其他属性。
4. 某些与核显相关的其他设备中已包含相关属性。

其中，第 1 步和第 4 步由 WhateverGreen 自动完成。其可运行在 macOS 10.8 及更高版本，这大大简化了正确启用核显的步骤。

## 建议
1. 在 BIOS 中设置核显所需的内存量（即 预分配 DVMT，英文: DVMT Pre-Allocated）为 32 MB, 64 MB, 96 MB 等，与使用的 FB 值相关。如要使用最大值（英文: DVMT Total），请设为 MAX。
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/bios.png) 

一些有缺陷的 BIOS 可能会有显示值与实际值不符的现象，并且实际值通常小于显示值，此时则需要设定一个更高的显示值。此现象在 戴尔 笔记本中比较常见，显示值为 64 MB，实际却只分配了 32 MB，且无法更改。后续会展示此现象。

2. 将 [Lilu.kext](https://github.com/vit9696/Lilu/releases) 和 [WhateverGreen.kext](https://github.com/acidanthera/WhateverGreen/releases)（下文简称为 WEG）添加到 Clover 的 `kexts/Other` 文件夹中。

3. 移除下列驱动（如果曾经在使用）
- IntelGraphicsFixup
- NvidiaGraphicsFixup 
- CoreDisplayFixup 
- Shiki 
- IntelGraphicsDVMTFixup 
- AzulPatcher4600
- AppleBacklightFixup
- FakePCIID_Intel_HD_Graphics 
- FakePCIID_Intel_HDMI_Audio 
- FakePCIID.kext（不使用其他基于 FakePCIID 的插件时）

4. 关闭以下所有 Clover 的显卡参数注入（注意是 *关闭*，不是注释掉）
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/Clover1.png)

5. 关闭 Clover 的以下 DSDT 补丁
- `AddHDMI`
- `FixDisplay`
- `FixIntelGfx`
- `AddIMEI`
- `FixHDA`
- `AddPNLF`

6. 关闭 Clover 的 `UseIntelHDMI`

7. 禁用 `Devices` - `Inject`参数（通常的 config.plist 中或许没有此参数，如有，则需关闭或删除）
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/Clover2.png)

8. 删除 `-disablegfxfirmware` 启动参数

9. 删除或注释掉 `IntelGFX` 和 `IMEI` 的 `FakeID`（译者注：在相应条目前面加 #，下同）

10. 删除或注释掉 `ig-platform-id`

11. 完全删除 DSDT 和 SSDT 模块 (config.plist) 中的 `Arbitrary`, `AddProperties`，以及 DSDT 或 SSDT 表单中设备 IGPU, IMEI, HDEF 下有关 HDMI 的设定（若存在）。

12. 删除或禁用以下 ACPI 重命名补丁: `GFX0 to IGPU`, `PEGP to GFX0`, `HECI to IMEI`, `MEI to IMEI`, `HDAS to HDEF`, `B0D3 to HDAU`

若要注入属性，请使用 config.plist 中的 `Properties` 功能，并且应当仅添加以下内容：
- `AAPL,ig-platform-id` 或 `AAPL,snb-platform-id`
- 设备 `IGPU` 的 `device-id`（需要仿冒时）
- 设备 `IMEI` 的 `device-id`（需要仿冒时）
- 部分补丁设定（必要时）

以及设备 `HDEF` 的 `Layout-ID`（细节详见 [AppleALC](https://github.com/acidanthera/AppleALC)。HDEF 设备的 `PciRoot` 位置可通过 [gfxutil](https://github.com/acidanthera/gfxutil) 来获取，在终端输入: `gfxutil -f HDEF` 即可）

这些或许并不需要，比如：默认的 FB 已经几乎完美，或是已经通过启动参数（英文: `boot-arg`）设置，并且无需仿冒相关设备的  `device-id`。

注意：上述属性应使用十六进制代码表示，并且需要倒序输入。如 `0x0166000B` 应该用 `0B006601` 表示。

下面分别提供了适用于不同微架构的常用 `IGPU` 和 `IMEI` 属性模版。

**注意！** 如果某个属性不是必需的，请完全删除掉；如果某个 `PciRoot` 位置不存在，也请彻底删除！

![](https://raw.githubusercontent.com/acidanthera/WhateverGreen/master/Manual/Img/basic.png)

**选择一个适合的 FB。** 首先试试推荐值，如果失败，则逐个尝试其他值，除了 “空 FB”（无可用端口），详见另一个 [专题](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923)。（译者注：原文发布于俄文社区，所以并无中文版本）在寻找合适的 FB 时，可以临时通过启动参数（英文: `boot-arg`）设置，此时 `Properties` 部分中的 FB 设置将被忽略。如: `igfxframe=0x0166000B`

**注意！** 此处格式与 `Properties` 部分的格式不同，这里应正序输入并保留 `0x` 前缀，如上例所示。
- 如未指定 FB，将会使用默认值；
- 如未设定 FB 并且存在独立显卡，将使用一组空 FB。

## HD 2000/3000（[Sandy Bridge](https://zh.wikipedia.org/zh-cn/Sandy_Bridge微架構) 微架构，下文简称 SNB）
支持 macOS 10.7 至 10.13.6，本文适用于 10.8 到 10.13.6。在旧版本系统上请使用传统驱动方式。从 macOS 10.14 起，HD 2000/3000 已经不再支持。[此处有强制驱动的方法](https://applelife.ru/posts/744431)。（译者注：原文发布于俄文社区，所以并无中文版本）当然，此方法无法开启 Metal。

SNB 微架构可用的 FB 列表:
- 0x00010000 (移动版，4 端口，无 FBMEM)
- 0x00020000 (移动版，1 端口，无 FBMEM)
- 0x00030010 (桌面版，3 端口，无 FBMEM)
- 0x00030020 (桌面版，3 端口，无 FBMEM)
- 0x00030030 (桌面版，无端口，无 FBMEM)
- 0x00040000 (移动版，3 端口，无 FBMEM)
- 0x00050000 (桌面版，无端口，无 FBMEM)

译者注：此处部分内容（“SNB 平台详细信息”）可能不必翻译至中文。
<details>
<summary>SNB 平台详细信息（点击此处以展开）</summary>
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
  
ID: SNB6（地址不可用）, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 512 KB, MAX STOLEN: 0 bytes, MAX OVERALL: 512 KB  
Camelia: CameliaUnsupported (255), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 1, PortCount: 0, FBMemoryCount: 0  
  
ID: SNB7（地址不可用）, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
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
  
缺省 SNB ID, DVMT: 0 bytes, FBMEM: 0 bytes, VRAM: 0 bytes, Flags: 0x00000000  
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
  
注意：以下机型在不指定 `AAPL,snb-platform-id` 时，将默认使用 `->` 后的相应 ID。

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
*推荐的 FB 配置*：0x00030010（桌面版，缺省值）或 0x00010000（移动版，缺省值）。

注意：HD 2000 无法单独工作，不过它可以（并且应该）配合一组 “空 FB”（无可用端口）使用以使 [IQSV](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923) 正常工作。（译者注：原文发布于俄文社区，所以并无中文版本）SNB 平台下，仅 HD 3000 可单独工作。

通常 SNB 平台无需指定 FB，与 `board-id` 相对应的一组 FB 将会被自动使用。不过，在使用不基于 SNB 平台的 SMBios 时，则需指定 FB。（译者注：如使用 `HD 3000` + 基于 `Ivy Bridge` 平台的 `MacBookPro9,1` 时，则需指定 FB）

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/snb.png)

注意！为 SNB 平台指定 FB 时，属性名应为 `AAPL,snb-platform-id`，这与其他平台不同。

对于桌面版，需设定（仿冒）`device-ID` 为 `26010000`。（如下所示）

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/snb_igpu.png)

（注：对于“空 FB”，需要设定不同的 ID，阅读 [此处](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923) 以获取更多详情）（译者注：原文发布于俄文社区，所以并无中文版本）

*注意！* 在基于 [7 系列芯片组](https://ark.intel.com/content/www/cn/zh/ark/products/series/98460/intel-7-series-chipsets.html?_ga=2.100876037.569501178.1553421075-527540512.1553334841) 的主板上使用基于 `SNB` 微架构的处理器时（译者注：如在 `Z77` 芯片组上使用基于 `SNB` 微架构的 `i7-2600` 时），需设定（仿冒）`IMEI` 的 `device-ID` 为 `3A1C000`。（如下所示）

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/snb_imei.png)

## HD 2500/4000（[Ivy Bridge](https://zh.wikipedia.org/zh-cn/Ivy_Bridge微架構) 微架构，下文简称 Ivy）
支持 macOS 10.8 或更新版本。

Capri (Ivy) 可用的 FB 列表：
- 0x01660000 (桌面版，4 端口，24 MB)
- 0x01260006 (桌面版，无端口，无 FBMEM，0 字节)
- 0x01260007 (桌面版，无端口，无 FBMEM，0 字节)
- 0x01260005 (桌面版，3 端口，16 MB)
- 0x01660001 (移动版，4 端口，24 MB)
- 0x01660002 (移动版，1 端口，24 MB)
- 0x01660008 (移动版，3 端口，16 MB)
- 0x01660009 (移动版，3 端口，16 MB)
- 0x01660003 (移动版，4 端口，16 MB)
- 0x01660004 (移动版，1 端口，16 MB)
- 0x0166000A (桌面版，3 端口，16 MB)
- 0x0166000B (桌面版，3 端口，16 MB)

译者注：此处内容（“Ivy 平台详细信息”）可能不必翻译至中文。
<details>
<summary>Ivy 平台详细信息（点击此处以展开）</summary>
AppleIntelFramebufferCapri.kext  
  
ID: 0x01660000, STOLEN: 96 MB, FBMEM: 24 MB, VRAM: 1024 MB, Flags: 0x00000000  
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
  
ID: 0x01620006, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 256 MB, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 0 bytes, MAX STOLEN: 0 bytes, MAX OVERALL: 0 bytes  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x01620007, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 256 MB, Flags: 0x00000000  
TOTAL STOLEN: 0 bytes, TOTAL CURSOR: 0 bytes, MAX STOLEN: 0 bytes, MAX OVERALL: 0 bytes  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x01620005, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 32 MB, MAX OVERALL: 33 MB (34615296 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 2, PortCount: 3, FBMemoryCount: 2  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000011 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
02050000 00040000 11000000  
03040000 00040000 07010000  
04060000 00040000 07010000  
  
ID: 0x01660001, STOLEN: 96 MB, FBMEM: 24 MB, VRAM: 1536 MB, Flags: 0x00000000  
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
  
ID: 0x01660002, STOLEN: 64 MB, FBMEM: 24 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 24 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 24 MB, MAX OVERALL: 25 MB (26742784 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
01000000 02000000 30000000  
  
ID: 0x01660008, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 48 MB, MAX OVERALL: 49 MB (51916800 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01000000 02000000 30000000  
02050000 00040000 07010000  
03040000 00040000 07010000  
  
ID: 0x01660009, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 48 MB, MAX OVERALL: 49 MB (51916800 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x00, pipe: 0, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01000000 02000000 30000000  
02050000 00040000 07010000  
03040000 00040000 07010000  
  
ID: 0x01660003, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
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
  
ID: 0x01660004, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 16 MB, MAX OVERALL: 17 MB (18354176 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[5] busId: 0x03, pipe: 0, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
05030000 02000000 30020000  
  
ID: 0x0166000A, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 16 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 32 MB, MAX OVERALL: 33 MB (34615296 bytes)  
Camelia: CameliaUnsupported (255), Freq: 1808 Hz, FreqMax: 1808 Hz  
Mobile: 0, PipeCount: 2, PortCount: 3, FBMemoryCount: 2  
[2] busId: 0x05, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[3] busId: 0x04, pipe: 0, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[4] busId: 0x06, pipe: 0, type: 0x00000800, flags: 0x00000006 - ConnectorHDMI  
02050000 00040000 07010000  
03040000 00040000 07010000  
04060000 00080000 06000000  
  
ID: 0x0166000B, STOLEN: 32 MB, FBMEM: 16 MB, VRAM: 1536 MB, Flags: 0x00000000  
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
*推荐的 FB 设置*：0x0166000A（桌面版，缺省值）或 0x01620005（桌面版）；0x01660003（移动版，缺省值）或 0x01660009（移动版）或 0x01660004（移动版）。

注意：HD 2500 无法单独工作，不过它可以（并且应该）配合一组 “空 FB”（无可用端口）使用以使 [IQSV](https://www.applelife.ru/threads/zavod-intel-quick-sync-video.817923) 正常工作。（译者注：原文发布于俄文社区，所以并无中文版本）Ivy 平台下，仅 HD 4000 可单独工作。

*注意！* 在基于 [6 系列芯片组](https://ark.intel.com/content/www/cn/zh/ark/products/series/98461/intel-6-series-chipsets.html?_ga=2.2193906.333725926.1553422863-527540512.1553334841) 的主板上使用基于 `Ivy` 微架构的处理器时（译者注：如在 `Z68` 芯片组上使用基于 `Ivy` 微架构的 `i7-3770` 时），需设定（仿冒）`IMEI` 的 `device-ID` 为 `3A1E0000`。（如下所示）

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/ivy_imei.png)  
 
## Intel HD Graphics 4200-5200（[Haswell](https://zh.wikipedia.org/zh-cn/Haswell微架構) 微架构）
支持 macOS 10.9 或更新版本。

Azul (Haswell) 可用的 FB 列表：
- 0x0C060000 (桌面版，3 端口，209 MB)
- 0x0C160000 (桌面版，3 端口，209 MB)
- 0x0C260000 (桌面版，3 端口，209 MB)
- 0x04060000 (桌面版，3 端口，209 MB)
- 0x04160000 (桌面版，3 端口，209 MB)
- 0x04260000 (桌面版，3 端口，209 MB)
- 0x0D260000 (桌面版，3 端口，209 MB)
- 0x0A160000 (桌面版，3 端口，209 MB)
- 0x0A260000 (桌面版，3 端口，209 MB)
- 0x0A260005 (移动版，3 端口，52 MB)
- 0x0A260006 (移动版，3 端口，52 MB)
- 0x0A2E0008 (移动版，3 端口，99 MB)
- 0x0A16000C (移动版，3 端口，99 MB)
- 0x0D260007 (移动版，4 端口，99 MB)
- 0x0D220003 (桌面版，3 端口，52 MB)
- 0x0A2E000A (桌面版，3 端口，52 MB)
- 0x0A26000A (桌面版，3 端口，52 MB)
- 0x0A2E000D (桌面版，2 端口，131 MB)
- 0x0A26000D (桌面版，2 端口，131 MB)
- 0x04120004 (桌面版，无端口，无 FBMEM，1 MB)
- 0x0412000B (桌面版，无端口，无 FBMEM，1 MB)
- 0x0D260009 (移动版，1 端口，99 MB)
- 0x0D26000E (移动版，4 端口，131 MB)
- 0x0D26000F (移动版，1 端口，131 MB)

译者注：此处内容（“Haswell 平台详细信息”）可能不必翻译至中文。
<details>
<summary>Haswell 平台详细信息（点击此处以展开）</summary>
AppleIntelFramebufferAzul.kext  
  
ID: 0x0C060000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x0C160000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x0C260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x04060000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x04160000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x04260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x0D260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x0A160000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x0A260000, STOLEN: 64 MB, FBMEM: 16 MB, VRAM: 1024 MB, Flags: 0x00000004  
TOTAL STOLEN: 209 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 209 MB, MAX OVERALL: 210 MB (220737536 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30000000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x0A260005, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x0000000F  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 87000000  
02040900 00040000 87000000  
  
ID: 0x0A260006, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x0000000F  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 87000000  
02040900 00040000 87000000  
  
ID: 0x0A2E0008, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000021E  
TOTAL STOLEN: 99 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239611904 bytes)  
Camelia: CameliaV1 (1), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 0x0A16000C, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000001E  
TOTAL STOLEN: 99 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239611904 bytes)  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
00000800 02000000 30000000  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 0x0D260007, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000031E  
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
  
ID: 0x0D220003, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x00000402  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[3] busId: 0x06, pipe: 8, type: 0x00000400, flags: 0x00000011 - ConnectorDP  
01050900 00040000 87000000  
02040A00 00040000 87000000  
03060800 00040000 11000000  
  
ID: 0x0A2E000A, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x000000D6  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000011 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 11000000  
01050900 00040000 87000000  
02040A00 00040000 87000000  
  
ID: 0x0A26000A, STOLEN: 32 MB, FBMEM: 19 MB, VRAM: 1536 MB, Flags: 0x000000D6  
TOTAL STOLEN: 52 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 116 MB, MAX OVERALL: 117 MB (123219968 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000011 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000087 - ConnectorDP  
00000800 02000000 11000000  
01050900 00040000 87000000  
02040A00 00040000 87000000  
  
ID: 0x0A2E000D, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000040E  
TOTAL STOLEN: 131 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239607808 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 0x0A26000D, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000040E  
TOTAL STOLEN: 131 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 227 MB, MAX OVERALL: 228 MB (239607808 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000107 - ConnectorDP  
01050900 00040000 07010000  
02040A00 00040000 07010000  
  
ID: 0x04120004, STOLEN: 32 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x0412000B, STOLEN: 32 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00000000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x0D260009, STOLEN: 64 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000001E
TOTAL STOLEN: 99 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 99 MB, MAX OVERALL: 100 MB (105385984 bytes)  
Camelia: CameliaDisabled (0), Freq: 1953 Hz, FreqMax: 1953 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
00000800 02000000 30000000  
  
ID: 0x0D26000E, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000031E  
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
  
ID: 0x0D26000F, STOLEN: 96 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x0000001E  
TOTAL STOLEN: 131 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 131 MB, MAX OVERALL: 132 MB (138940416 bytes)  
Camelia: CameliaV2 (2), Freq: 1953 Hz, FreqMax: 1953 Hz  
Mobile: 1, PipeCount: 3, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000030 - ConnectorLVDS  
00000800 02000000 30000000  
</details>

####
*推荐的 FB 设置*：0x0D220003（桌面版，缺省值）；0x0A160000（移动版，缺省值）或 0x0A260005（移动版，推荐）或 0x0A260006（移动版，推荐）。

对于 桌面版 HD 4400 以及移动版 HD4200/HD4400/HD4600 ，需设定（仿冒）`IGPU` 的 `device-id` 为 `12040000`。（如下所示）

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/hsw_igpu.png) 

## HD 5300-6300（[Broadwell](https://zh.wikipedia.org/zh-cn/Broadwell微架構) 微架构，下文简称 BDW）
支持 macOS 10.10.2 或更新版本。

BDW 平台可用的 FB 列表：
- 0x16060000 (桌面版，3 端口，32 MB)
- 0x160E0000 (桌面版，3 端口，32 MB)
- 0x16160000 (桌面版，3 端口，32 MB)
- 0x161e0000 (桌面版，3 端口，32 MB)
- 0x16260000 (桌面版，3 端口，32 MB)
- 0x162B0000 (桌面版，3 端口，32 MB)
- 0x16220000 (桌面版，3 端口，32 MB)
- 0x160E0001 (移动版，3 端口，60 MB)
- 0x161E0001 (移动版，3 端口，60 MB)
- 0x16060002 (移动版，3 端口，56 MB)
- 0x16160002 (移动版，3 端口，56 MB)
- 0x16260002 (移动版，3 端口，56 MB)
- 0x16220002 (移动版，3 端口，56 MB)
- 0x162B0002 (移动版，3 端口，56 MB)
- 0x16120003 (移动版，4 端口，56 MB)
- 0x162B0004 (桌面版，3 端口，56 MB)
- 0x16260004 (桌面版，3 端口，56 MB)
- 0x16220007 (桌面版，3 端口，77 MB)
- 0x16260005 (移动版，3 端口，56 MB)
- 0x16260006 (移动版，3 端口，56 MB)
- 0x162B0008 (桌面版，2 端口，69 MB)
- 0x16260008 (桌面版，2 端口，69 MB)

译者注：此处内容（“BDW 平台详细信息”）可能不必翻译至中文。
<details>
<summary>BDW 平台详细信息（点击此处以展开）</summary>
AppleIntelBDWGraphicsFramebuffer.kext  
  
ID: 0x16060000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x160E0000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000706  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x16160000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x161E0000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000716  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x16260000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x162B0000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x00000B06  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x16220000, STOLEN: 16 MB, FBMEM: 15 MB, VRAM: 1024 MB, Flags: 0x0000110E  
TOTAL STOLEN: 32 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 64 MB, MAX OVERALL: 65 MB (68694016 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000004, flags: 0x00000004 - ConnectorDigitalDVI  
[2] busId: 0x04, pipe: 9, type: 0x00000800, flags: 0x00000082 - ConnectorHDMI  
00000800 02000000 30020000  
01050900 04000000 04000000  
02040900 00080000 82000000  
  
ID: 0x160E0001, STOLEN: 38 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000702  
TOTAL STOLEN: 60 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 136 MB, MAX OVERALL: 137 MB (144191488 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00001001 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00003001 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 01100000  
02040A00 00040000 01300000  
  
ID: 0x161E0001, STOLEN: 38 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000702  
TOTAL STOLEN: 60 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 136 MB, MAX OVERALL: 137 MB (144191488 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00001001 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00003001 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 01100000  
02040A00 00040000 01300000  
  
ID: 0x16060002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B02  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x16160002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B02  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x16260002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B0A  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x16220002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B0A  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x162B0002, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00004B0A  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaV2 (2), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x16120003, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00001306  
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
  
ID: 0x162B0004, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00040B46  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000211 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 11020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x16260004, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00040B46  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000211 - ConnectorLVDS  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 11020000  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x16220007, STOLEN: 38 MB, FBMEM: 38 MB, VRAM: 1536 MB, Flags: 0x000BB306  
TOTAL STOLEN: 77 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 153 MB, MAX OVERALL: 154 MB (162017280 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[3] busId: 0x06, pipe: 8, type: 0x00000400, flags: 0x00000011 - ConnectorDP  
01050900 00040000 07050000  
02040A00 00040000 07050000  
03060800 00040000 11000000  
  
ID: 0x16260005, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000B0B  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050B00 00040000 07050000  
02040B00 00040000 07050000  
  
ID: 0x16260006, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00000B0B  
TOTAL STOLEN: 56 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 124 MB, MAX OVERALL: 125 MB (131608576 bytes)  
Camelia: CameliaDisabled (0), Freq: 2777 Hz, FreqMax: 2777 Hz  
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000230 - ConnectorLVDS  
[1] busId: 0x05, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 11, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
00000800 02000000 30020000  
01050B00 00040000 07050000  
02040B00 00040000 07050000  
  
ID: 0x162B0008, STOLEN: 34 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x00002B0E  
TOTAL STOLEN: 69 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109060096 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 2, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
01050900 00040000 07050000  
02040A00 00040000 07050000  
  
ID: 0x16260008, STOLEN: 34 MB, FBMEM: 34 MB, VRAM: 1536 MB, Flags: 0x00002B0E  
TOTAL STOLEN: 69 MB, TOTAL CURSOR: 1 MB, MAX STOLEN: 103 MB, MAX OVERALL: 104 MB (109060096 bytes)  
Camelia: CameliaDisabled (0), Freq: 5273 Hz, FreqMax: 5273 Hz  
Mobile: 0, PipeCount: 2, PortCount: 2, FBMemoryCount: 2  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x00000507 - ConnectorDP  
01050900 00040000 07050000  
02040A00 00040000 07050000  
</details>
 
####
*推荐的 FB 设置*：0x16220007（桌面版，缺省值）；0x16260006（移动版，缺省值）。


## HD 510-580（[Skylake](https://zh.wikipedia.org/zh-cn/Skylake微架構) 微架构，下文简称 SKL）
支持 macOS 10.11.4 或更新版本。

SKL 平台可用的 FB 列表：
- 0x191E0000 (移动版，3 端口，56 MB)
- 0x19160000 (移动版，3 端口，56 MB)
- 0x19260000 (移动版，3 端口，56 MB)
- 0x19270000 (移动版，3 端口，56 MB)
- 0x191B0000 (移动版，3 端口，56 MB)
- 0x193B0000 (移动版，3 端口，56 MB)
- 0x19120000 (移动版，3 端口，56 MB)
- 0x19020001 (桌面版，无端口，无 FBMEM，1 MB)
- 0x19170001 (桌面版，无端口，无 FBMEM，1 MB)
- 0x19120001 (桌面版，无端口，无 FBMEM，1 MB)
- 0x19320001 (桌面版，无端口，无 FBMEM，1 MB)
- 0x19160002 (移动版，无端口，无 FBMEM，58 MB)
- 0x19260002 (移动版，3 端口，无 FBMEM，58 MB)
- 0x191E0003 (移动版，3 端口，无 FBMEM，41 MB)
- 0x19260004 (移动版，3 端口，无 FBMEM，35 MB)
- 0x19270004 (移动版，3 端口，无 FBMEM，58 MB)
- 0x193B0005 (移动版，4 端口，无 FBMEM，35 MB)
- 0x191B0006 (移动版，1 端口，无 FBMEM，39 MB)
- 0x19260007 (移动版，3 端口，无 FBMEM，35 MB)

译者注：此处内容（“SKL 平台详细信息”）可能不必翻译至中文。
<details>
<summary>SKL 平台详细信息（点击此处以展开）</summary>
AppleIntelSKLGraphicsFramebuffer.kext  
  
ID: 0x191E0000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000050F  
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
  
ID: 0x19160000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000090F  
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
  
ID: 0x19260000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000090F  
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
  
ID: 0x19270000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000090F  
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
  
ID: 0x191B0000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000110F  
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
  
ID: 0x193B0000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00001187  
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
  
ID: 0x19120000, STOLEN: 34 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000110F  
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
  
ID: 0x19020001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x19170001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x19120001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x19320001, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00040800  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics SKL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x19160002, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00830B02  
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
  
ID: 0x19260002, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
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
  
ID: 0x191E0003, STOLEN: 40 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x002B0702  
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
  
ID: 0x19260004, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B0A  
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
  
ID: 0x19270004, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
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
  
ID: 0x193B0005, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0023130A  
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
  
ID: 0x191B0006, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00131302  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 512 KB, MAX STOLEN: 39 MB, MAX OVERALL: 39 MB (41422848 bytes)  
Model name: Intel HD Graphics 530  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 1, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
00000800 02000000 98040000  
  
ID: 0x19260007, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00031302  
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

注意：在不指定 `AAPL,ig-platform-id` 时，默认使用 `0x19120000`。
</details>
 
####
*推荐的 FB 设置*：0x19120000（桌面版，缺省值）；0x19160000（移动版，缺省值）。

## HD 610-650（[Kaby Lake](https://zh.wikipedia.org/zh-cn/Kaby_Lake微架構) 微架构，下文简称 KBL）
支持 macOS 10.12.6 或更新版本。

KBL 平台可用的 FB 列表：
- 0x591E0000 (移动版，3 端口，无FBMEM，35 MB)
- 0x59160000 (移动版，3 端口，无FBMEM，35 MB)
- 0x59230000 (桌面版，3 端口，无FBMEM，39 MB)
- 0x59260000 (桌面版，3 端口，无FBMEM，39 MB)
- 0x59270000 (桌面版，3 端口，无FBMEM，39 MB)
- 0x59270009 (移动版，3 端口，无FBMEM，39 MB)
- 0x59120000 (桌面版，3 端口，无FBMEM，39 MB)
- 0x591B0000 (移动版，3 端口，39 MB)
- 0x591E0001 (移动版，3 端口，无FBMEM，39 MB)
- 0x59180002 (移动版，无端口，无FBMEM，1 MB)
- 0x59120003 (移动版，无端口，无FBMEM，1 MB)
- 0x59260007 (桌面版，3 端口，79 MB)
- 0x59270004 (移动版，3 端口，无FBMEM，58 MB)
- 0x59260002 (移动版，3 端口，无FBMEM，58 MB)
- 0x591B0006 (移动版，1 端口，无FBMEM，39 MB)
 
译者注：此处内容（“KBL 平台详细信息”）可能不必翻译至中文。
<details>
<summary>KBL 平台详细信息（点击此处以展开）</summary>
AppleIntelKBLGraphicsFramebuffer.kext  
  
ID: 0x591E0000, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000078B  
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
  
ID: 0x59160000, STOLEN: 34 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00000B0B  
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
  
ID: 0x59230000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B8B  
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
  
ID: 0x59260000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B8B  
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
  
ID: 0x59270000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B8B  
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
  
ID: 0x59270009, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00830B0A  
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
  
ID: 0x59120000, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000110B  
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
  
ID: 0x591B0000, STOLEN: 38 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x0000130B  
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
  
ID: 0x591E0001, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x002B0702  
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
  
ID: 0x59180002, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics KBL  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x59120003, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics KBL  
Camelia: CameliaDisabled (0), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x59260007, STOLEN: 57 MB, FBMEM: 21 MB, VRAM: 1536 MB, Flags: 0x00830B0E  
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
  
ID: 0x59270004, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
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
  
ID: 0x59260002, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
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
  
ID: 0x591B0006, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00031302  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 512 KB, MAX STOLEN: 39 MB, MAX OVERALL: 39 MB (41422848 bytes)  
Model name: Intel HD Graphics 630  
Camelia: CameliaV3 (3), Freq: 1388 Hz, FreqMax: 1388 Hz  
Mobile: 1, PipeCount: 1, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
00000800 02000000 98040000  
  
注意：在不指定 `AAPL,ig-platform-id` 时，默认使用 `0x59160000`。
</details>
 
####
*推荐的 FB 设置*：0x59160000（桌面版，缺省值）或 0x59120000（桌面版，推荐）；0x591B0000（移动版，缺省值）。

对于 UHD 620 ([Kaby Lake Refresh](https://en.wikipedia.org/wiki/Kaby_Lake#List_of_8th_generation_Kaby_Lake_R_processors)，译者注：该页面暂无中文版本，上次检查日期：2019/03/25)，需设定（仿冒）`IGPU` 的 `device-id` 为 `16590000`。（如下所示）

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/kbl-r_igpu.png)
 

## UHD 610-655（[Coffee Lake](https://zh.wikipedia.org/zh-cn/Coffee_Lake微架构) 微架构，下文简称 CFL）
支持 macOS 10.14 或更新版本。

CFL 平台可用的 FB 列表：
- 0x3EA50009 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3E920009 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3E9B0009 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3EA50000 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3E920000 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3E000000 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3E9B0000 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3EA50004 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3EA50005 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3EA60005 (移动版，3 端口，无 FBMEM，58 MB)
- 0x3E9B0006 (移动版，1 端口，无 FBMEM，39 MB)
- 0x3E9B0008 (移动版，1 端口，无 FBMEM, 58 MB)
- 0x3E9B0007 (桌面版，3 端口，无 FBMEM，58 MB)
- 0x3E920003 (桌面版，无端口，无 FBMEM，1 MB)
- 0x3E910003 (桌面版，无端口，无 FBMEM，1 MB)
- 0x3E980003 (桌面版，无端口，无 FBMEM，1 MB)

译者注：此处内容（“CFL 平台详细信息”）可能不必翻译至中文。
<details>
<summary>CFL 平台详细信息（点击此处以展开）</summary>
AppleIntelCFLGraphicsFramebuffer.kext  
  
ID: 0x3EA50009, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00830B0A  
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
  
ID: 0x3E920009, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0083130A  
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
  
ID: 0x3E9B0009, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0083130A  
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
  
ID: 0x3EA50000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00030B0B  
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
  
ID: 0x3E920000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000130B  
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
  
ID: 0x3E000000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000130B  
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
  
ID: 0x3E9B0000, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x0000130B  
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
  
ID: 0x3EA50004, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A  
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

ID: 0x3EA50005, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A
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
  
ID: 0x3EA60005, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00E30B0A
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)
Model name: Intel Iris Plus Graphics 645
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz
Mobile: 1, PipeCount: 3, PortCount: 3, FBMemoryCount: 3
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP
00000800 02000000 98040000
01050900 00040000 C7030000
02040A00 00040000 C7030000

ID: 0x3E9B0006, STOLEN: 38 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00131302  
TOTAL STOLEN: 39 MB, TOTAL CURSOR: 512 KB, MAX STOLEN: 39 MB, MAX OVERALL: 39 MB (41422848 bytes)  
Model name: Intel UHD Graphics 630  
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 1, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000498 - ConnectorLVDS  
00000800 02000000 98040000  

ID: 0x3E9B0008, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00031302  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 512 KB, MAX STOLEN: 58 MB, MAX OVERALL: 58 MB (61345792 bytes)  
Model name: Intel HD Graphics CFL  
Camelia: CameliaV3 (3), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 1, PipeCount: 1, PortCount: 1, FBMemoryCount: 1  
[0] busId: 0x00, pipe: 8, type: 0x00000002, flags: 0x00000098 - ConnectorLVDS  
00000800 02000000 98000000

ID: 0x3E9B0007, STOLEN: 57 MB, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00801302  
TOTAL STOLEN: 58 MB, TOTAL CURSOR: 1 MB (1572864 bytes), MAX STOLEN: 172 MB, MAX OVERALL: 173 MB (181940224 bytes)  
Model name: Intel UHD Graphics 630  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 3, PortCount: 3, FBMemoryCount: 3  
[1] busId: 0x05, pipe: 9, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[2] busId: 0x04, pipe: 10, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
[3] busId: 0x06, pipe: 8, type: 0x00000400, flags: 0x000003C7 - ConnectorDP  
01050900 00040000 C7030000  
02040A00 00040000 C7030000  
03060800 00040000 C7030000  
  
ID: 0x3E920003, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics CFL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0  
  
ID: 0x3E910003, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000  
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB  
Model name: Intel HD Graphics CFL  
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz  
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0

ID: 0x3E980003, STOLEN: 0 bytes, FBMEM: 0 bytes, VRAM: 1536 MB, Flags: 0x00001000
TOTAL STOLEN: 1 MB, TOTAL CURSOR: 0 bytes, MAX STOLEN: 1 MB, MAX OVERALL: 1 MB
Model name: Intel HD Graphics CFL
Camelia: CameliaDisabled (0), Freq: 0 Hz, FreqMax: 0 Hz
Mobile: 0, PipeCount: 0, PortCount: 0, FBMemoryCount: 0
  
注意：在不指定 `AAPL,ig-platform-id` 时，默认使用 `0x3EA50000`。
</details>

####
*推荐的 FB 设置*：0x3EA50000（桌面版，缺省值）或 0x3E9B0007（桌面版，推荐）；0x3EA50009（移动版，缺省值）。

注意：使用第九代 Coffee Lake R 处理器时，需设定（仿冒）`IGPU` 的 `device-id` 为 `923E0000`。（如下所示）

*从 macOS Mojave 10.14.4 起，无需再设定此参数！*

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/cfl-r_igpu.png)  

<details>
<summary>更多：CFL 平台在 macOS 10.13 的适配情况（点击此处以展开）</summary>

通常无需在 CFL 平台上安装 macOS 10.13，除非有特殊原因，比如：在 10.13 下使用 Web Driver 以驱动 麦克斯韦 (Maxwell) / 帕斯卡 (Pascal) 架构的 NVIDIA 显卡。

macOS High Sierra 10.13.6 的特别版本 17G2208 包含对 CFL 平台核显的原生支持。([下载地址一](https://drive.google.com/file/d/1FyPvo81K8qEXhiEuwDX3mAHMg1ZMdiYS/view) [下载地址二](https://mega.nz/#!GNgDTDob!N3jediG_xrzJPRFi9bQ0MtAFCKbOl33QvQp9tRUSwhQ))

注意：此版本不包含“空 FB”（无可用端口）支持，亦不提供对硬件 ID 为 `0x3E91` 的设备支持。

如需“空 FB”支持，请使用来自 10.14 的 `AppleIntelCFLGraphicsFramebuffer.kext`。

对于硬件 ID 为 `0x3E91` 的设备，请将 ID 设定（仿冒）为 `0x3E92`。（即设定 `device-id` 为 `923E0000`）。

与 17G2208 相同的是，后续的 17G3025 以及更新版本，存在同样的问题。

对于 UHD 630，或许直接将硬件 ID 设定（仿冒）为 HD 630 的 ID 更好一些。（如下所示）

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/kbl.png)

同时，请明确指定一组适配 `HD 630` 的 FB！
</details> 

## 调节笔记本亮度
使用此 ACPI 表 [SSDT-PNLF](https://raw.githubusercontent.com/acidanthera/WhateverGreen/master/Manual/SSDT-PNLF.dsl)

## 数字音频支持 (HDMI / DVI / DP)
若要启用数字音频，需要设置必要的属性，通常还需要修正端口信息。

音频部分，尤其是 HDMI 的音频部分，通常可共同使用 WEG 与 [AppleALC.kext](https://github.com/acidanthera/AppleALC) 以使其工作。

macOS 10.10.5 或更新版本中，在不使用自定义补丁时，WEG 会自动将 `connector-type` 中的 DP (00040000) 端口修改为 HDMI (00080000) 端口。

实际端口可以是任意类型 (HDMI / DVI / DP)，不过，数字音频工作时，`connector-type` 必须是 HDMI。

## 使用 WEG 自定义 FB 和 端口 补丁
大多情况下，不需要额外补丁！

macOS 10.14 下，对于 SKL 或更新平台，无法直接从 kext 二进制文件中取得 FB 和 端口信息：所以必须从内存中导出二进制文件，因此无法再使用 Clover 来修改 FB。不过，可以通过 WEG 制作 语义补丁（推荐）或 二进制补丁，其在较早的 macOS 版本和较早的核显设备上亦适用。默认情况下，当前使用的 FB 将会被修改。

补丁需放在 IGPU 的 `Properties` 部分。

二进制补丁示例：

![](./Img/bin.png)  

语义补丁示例一：修改端口索引为 1, 2, 3 的 `connector-type` 为 HDMI：(connector-type=00080000)

![](./Img/connector.png)  

语义补丁示例二：对于 DVMT 为 32 MB 且需要更大值时：(stolenmem=19MB, fbmem=9MB)

![](./Img/sem.png)  

[此部分补丁](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/AzulPatcher4600_equivalent.plist) 完全等同于使用 AzulPatcher4600.kext，如曾在使用，请改用这些补丁。（在 [某些](https://github.com/coderobe/AzulPatcher4600#tested-onHaswell) Haswell 微架构的笔记本上，使用 `0x0A260006` 这组 FB 会改善花屏的情况）

**WEG 支持的自定义补丁列表**  
语义补丁部分：

*framebuffer-patch-enable (**启用语义补丁的总开关**)*

*framebuffer-framebufferid (**要修改的 FB，一般保持默认即可**)*

*framebuffer-mobile  
framebuffer-pipecount   
framebuffer-portcount   
framebuffer-memorycount   
framebuffer-stolenmem   
framebuffer-fbmem*

*framebuffer-unifiedmem (**VRAM，不推荐使用**)*

*framebuffer-cursormem (**Haswell 专用补丁**)*

*framebuffer-flags*

*framebuffer-camellia (**集成显示控制器，仅与白苹果相关**)*

*framebuffer-conX-enable (**启用端口为 X 的修改**)*   
*framebuffer-conX-index  
framebuffer-conX-busid  
framebuffer-conX-pipe  
framebuffer-conX-type  
framebuffer-conX-flags  
framebuffer-conX-alldata (**完全替换端口信息**)  
framebuffer-conX-YYYYYYYY-alldata (**在当前 FB 与 YYYYYY 匹配时完全替换端口信息**)*

*X 是端口索引。*

**Alldata 补丁可按序修改多个端口：将所有数据放在一个字符串中，并指定一个起始端口索引即可。字符串长度应为 12 的倍数字节。（单个端口长度）**


二进制补丁部分：

*framebuffer-patchN-enable (**启用第 N 项补丁**)*

*framebuffer-patchN-framebufferid (**要修改的 FB，一般保持默认即可**)*

*framebuffer-patchN-find*

*framebuffer-patchN-replace*

*framebuffer-patchN-count (要搜索的补丁号迭代数，默认为 1)*

*N 为补丁索引号: 0, 1, 2, ... 9*

可以使用 [010 Editor](http://www.sweetscape.com/010editor) 和 [IntelFramebuffer.bt](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/IntelFramebuffer.bt) 脚本来提取有关 FB 和 端口 的详细信息。

这些信息可帮助制作自定义补丁。

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/ifbt.png)

macOS 10.14 下，对于 SKL 或更新平台，要取得适合脚本的导出数据，可以使用 *WEG* 的 Debug 版本并加入 `-igfxdump` 启动参数。导出的数据将保存在 `/var/log`目录中。

原始与修补的导出数据可通过使用 *WEG* 的 Debug 版本、使用 `-igfxfbdump` 启动参数，并在 IOReg 中的 `IOService:/IOResources/WhateverGreen` 位置取得。

## VGA 输出支持
大多情况下，在 SKL 或更新平台可直接工作。
  
对于 Ivy 或其他平台，可使用以下选项来修正端口：

06020000 02000000 30000000 //选项1  
06020000 01000000 30000000 //选项2 

在 macOS 10.8.2 或更新版本，Ivy 平台无法支持 VGA。

热插拔功能通常无法工作。

也许这些没有帮助，但目前也没有其他已知解决方案。  

## EDID
EDID 通常会被正确识别，因此不需要执行其他操作。但在极少数情况下，需要手动注入 EDID。

EDID 信息可以通过诸如使用 [Linux](https://unix.stackexchange.com/questions/114359/how-to-get-edid-for-a-single-monitor)（译者注：原文发布于英文社区，所以并无中文版本）等方法获得。

正确的 EDID 必须放入 IGPU 的 *AAPL0**0**,override-no-connect* 字段中，其中第二个 ***0*** 代表显示器编号。

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/edid.png) 

某些时候，导出的 EDID 可能与 macOS 不兼容并导致失真。这时，对于一些 EDID 可以使用 [此脚本](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/edid-gen.sh)，它能修正 EDID 并保存到桌面上。

## HDMI 高分屏 60 fps 方案
#### 除了解决 HDMI 问题，此方案或许对某些型号如 ThinkPad P71/7700HQ/HD630/4K 卡死在 `gIOScreenLockState3` 的情况有所帮助。
为核显添加 `enable-hdmi20` 属性，或使用 `-cdfon` 启动参数代替，**否则将会黑屏**。


![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/hdmi20.png) 

## 禁用独显
为核显添加 `disable-external-gpu` 属性，或使用 `-wegnoegpu` 启动参数代替。

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/dGPU_off.png)  

## 修复笔记本内屏返回错误的最大链路速率值的问题 (Dell XPS 15 9570 等高分屏笔记本)
为核显添加 `enable-dpcd-max-link-rate-fix` 属性或者直接使用 `-igfxmlr` 启动参数以解决系统在点亮内屏时直接崩溃的问题。  
从 1.3.7 版本开始，此补丁同时修正从屏幕扩展属性里读取的错误速率值问题以解决在 Dell 灵越 7590 系列等新款笔记本上内核崩溃的问题。  
从 1.4.4 版本开始，如果用户未定义 `dpcd-max-link-rate` 属性的话，此补丁将自动从 DPCD 寻找内屏支持的最大链路速率值。此外此补丁已适配 Ice Lake 平台。
  
![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/dpcd_mlr.png)  
另外可使用 `dpcd-max-link-rate` 这个属性来为笔记本内屏指定一个最大链路速率值。  
4K 内屏一般使用 `0x14`，1080p 内屏使用 `0x0A` 即可。  
可选值为 `0x06` (RBR)，`0x0A` (HBR)，`0x14` (HBR2) 以及 `0x1E` (HBR3)。  
若指定了其他值，或者未定义此属性的话，则补丁默认自动寻找内屏所支持的链路最大值。  
若显卡驱动不支持找到的链路最大值的话，那么之后会触发内核崩溃，因此你需要按照上述方法手动指定一个合法的值。（这个情况理论上应该很少见。）  

<details>
<summary>调试</summary>
当驱动自动寻找最大链路速率值时，你会在内核日志里发现如下的日志。  
在此例中，Dell XPS 15 9570 的 4K 内屏所支持的最大链路速率值为 5.4 Gbps，因此补丁写入对应的 `0x14` 值。

```
igfx: @ (DBG) MLR: Found CFL- platforms. Will setup the fix for the CFL- graphics driver.
igfx: @ (DBG) MLR: [CFL-] Functions have been routed successfully.
igfx: @ (DBG) MLR: [CFL-] wrapReadAUX() Called with controller at 0xffffff802ca6e000 and framebuffer at 0xffffff81aa5a3000.
igfx: @ (DBG) MLR: [COMM] orgReadAUX() Routed to CFL IMP with Address = 0x0; Length = 16.
igfx: @ (DBG) MLR: [COMM] GetFBIndex() Port at 0x0; Framebuffer at 0xffffff81aa5a3000.
igfx: @ (DBG) MLR: [COMM] wrapReadAUX() Will probe the maximum link rate from the table.
igfx: @ (DBG) MLR: [COMM] orgReadAUX() Routed to CFL IMP with Address = 0x700; Length = 1.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Found eDP version 1.4+ (Value = 0x4).
igfx: @ (DBG) MLR: [COMM] orgReadAUX() Routed to CFL IMP with Address = 0x10; Length = 16.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Table[0] =  8100; Link Rate = 1620000000; Decimal Value = 0x06.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Table[1] = 10800; Link Rate = 2160000000; Decimal Value = 0x08.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Table[2] = 12150; Link Rate = 2430000000; Decimal Value = 0x09.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Table[3] = 13500; Link Rate = 2700000000; Decimal Value = 0x0a.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Table[4] = 16200; Link Rate = 3240000000; Decimal Value = 0x0c.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Table[5] = 21600; Link Rate = 4320000000; Decimal Value = 0x10.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() Table[6] = 27000; Link Rate = 5400000000; Decimal Value = 0x14.
igfx: @ (DBG) MLR: [COMM] ProbeMaxLinkRate() End of table.
igfx: @ (DBG) MLR: [COMM] wrapReadAUX() Maximum link rate 0x14 has been set in the DPCD buffer.
igfx: @ (DBG) MLR: [CFL-] wrapReadAUX() Called with controller at 0xffffff802ca6e000 and framebuffer at 0xffffff81aa5a3000.
igfx: @ (DBG) MLR: [COMM] orgReadAUX() Routed to CFL IMP with Address = 0x2200; Length = 16.
igfx: @ (DBG) MLR: [COMM] GetFBIndex() Port at 0x0; Framebuffer at 0xffffff81aa5a3000.
igfx: @ (DBG) MLR: [COMM] wrapReadAUX() Will use the maximum link rate specified by user or cached by the previous probe call.
igfx: @ (DBG) MLR: [COMM] wrapReadAUX() Maximum link rate 0x14 has been set in the DPCD buffer.
```
</details>


## 修复核显驱动在尝试点亮外接 HDMI 高分辨率显示器时造成的死循环问题
**适用平台：** 第六代酷睿 Skylake 核显，第七代酷睿 Kaby Lake 核显以及第八代酷睿 Coffee Lake。  
为核显添加 `enable-hdmi-dividers-fix` 属性或者直接使用 `-igfxhdmidivs` 启动参数以解决核显驱动在试图点亮外接 HDMI 高分辨率显示器时造成的系统死机问题。  
具体症状表现为插入 HDMI 线后，笔记本内屏变黑但有背光，系统无响应，并且外屏也无输出。
#### 关于使用此修复补丁的一些建议
- 如果你的笔记本或台式机主板有 HDMI 1.4 接口，并且想使用 2K 或 4K HDMI 显示器的话，你可能需要这个补丁。
- 如果你的笔记本或台式机主板有 HDMI 2.0 接口，并且当前 HDMI 输出有问题，那么建议你启用 LSPCON 驱动支持以获得更好的 HDMI 2.0 体验。（详情请阅读下方 LSPCON 章节）

## 启用 LSPCON 驱动以支持核显 DisplayPort 转 HDMI 2.0 输出
#### 简述
近几年的笔记本都开始配备了 HDMI 2.0 输出端口。这个端口可能直接连到核显上也有可能连在独显上。  
如果连在了独显上，那么在 macOS 下这个 HDMI 2.0 端口直接废掉了，因为苹果不支持 Optimus 等双显卡切换技术。  
如果连在了核显上，那么笔记本厂商需要在主板上安装额外的信号转换器来把 DP 信号转换成 HDMI 2.0 信号，  
这是因为现阶段英特尔的核显并不能原生提供 HDMI 2.0 信号输出。（类似主板厂商使用第三方芯片以提供 USB 3.0 功能）  
这个信号转换器名为 LSPCON，全称 **L**evel **S**hifter and **P**rotocol **Con**verter，并且有两种工作模式。  
当工作在 LS 模式下，它可以把 DP 转换成 HDMI 1.4 信号。在 PCON 模式下，它可以把 DP 转换成 HDMI 2.0 信号。  
然而有些厂商在转换器的固件里把 LS 设为了默认的工作模式，这就导致在 macOS 下 HDMI 2.0 连接直接黑屏或者根本不工作。    
从 1.3.0 版本开始，WhateverGreen 提供了对 LSPCON 的驱动支持。驱动会自动将转换器调为 PCON 模式以解决 HDMI 2.0 输出黑屏问题。  

#### 使用前必读
- LSPCON 驱动适用于所有配备 HDMI 2.0 接口并接在核显上的笔记本和台式机。
- 目前来看，英特尔的新处理器所配备的核显仍然不支持原生 HDMI 2.0 输出，所以在新平台上你可能仍然需要此驱动。
- 适用的英特尔平台: Skylake, Kaby Lake, Coffee Lake 以及以后。  
      Skylake 平台案例: 英特尔在 Skull Canyon NUC 上搭载了 HDMI 2.0 接口，使用了型号为 Parade PS175 的 LSPCON 信号转换器。  
  Coffee Lake 平台案例: 部分笔记本如 Dell XPS 15 搭载了 HDMI 2.0 接口，同样使用了型号为 Parade PS175 的 LSPCON 信号转换器。  
- 如果你已确认你的 HDMI 2.0 接口是连在核显上并且目前输出没有任何问题，那么你不需要特意启用此驱动。你的转换器可能已经出厂时就把 PCON 设为了默认的工作模式。

#### 如何使用
- 为核显添加 `enable-lspcon-support` 属性或者直接使用 `-igfxlspcon` 启动参数来启用驱动。
- 接下来你需要知道 HDMI 2.0 对应的端口号是多少。这个你可以直接在 IORegistryExplorer 里看到。也就是在 `AppleIntelFramebuffer@0/1/2/3` 下面找到你的外接显示器。  
*如果你身边只有 2K/4K HDMI 显示器的话，你可能需要先启用上面的死循环修复补丁，否则当你连接显示器时系统直接死机，所以就看不到对应的端口号了。*  
- 为核显添加 `framebuffer-conX-has-lspcon` 属性来通知驱动哪个接口下面有 LSPCON 信号转换器。  
把 `conX` 里的 X 替换成你在上一步找到的端口值。  
这个属性的对应值请设为 `Data` 类型。如果接口下存在转换器的话，请设为 `01000000`，反之设为 `00000000`。  
若不定义这个属性的话，驱动默认认为对应接口下**不存在**转换器。
- *(可选)* 为核显添加 `framebuffer-conX-preferred-lspcon-mode` 属性以指定 LSPCON 应该工作在何种模式下。  
这个属性的对应值请设为 `Data` 类型。  
如果希望转换器工作在 PCON (DP 转 HDMI 2.0) 模式下的话，请设为 `01000000`。  
如果希望转换器工作在 &nbsp;&nbsp;LS (DP 转 HDMI 1.4) 模式下的话，请设为 `00000000`。  
若指定其他值的话，驱动默认认为转换器应工作在 PCON 模式下。  
若不定义此属性的话，同上。  
![](Img/lspcon.png)

#### 排查错误
完成上述步骤后，重建缓存重启电脑，插上 HDMI 2.0 线和 HDMI 2.0 显示器应该可以正常看到输出的图像了。  
如果提取内核日志的话，你应该可以看到类似如下的日志。  

```
// 插入 HDMI 2.0 线
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] called with controller at 0xffffff81a8680000 and framebuffer at 0xffffff81a868c000.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] No LSPCON chip associated with this framebuffer.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] Will call the original method.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] Returns 0x0.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] called with controller at 0xffffff81a8680000 and framebuffer at 0xffffff81a869a000.
igfx @ (DBG) SC:   LSPCON::probe() DInfo: [FB2] Found the LSPCON adapter: Parade PS1750.
igfx @ (DBG) SC:   LSPCON::probe() DInfo: [FB2] The current adapter mode is Level Shifter (DP++ to HDMI 1.4).
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] LSPCON driver has detected the onboard chip successfully.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] LSPCON driver has been initialized successfully.
igfx @ (DBG) SC: LSPCON::getMode() DInfo: [FB2] The current mode value is 0x00.
igfx @ (DBG) SC: LSPCON::getMode() DInfo: [FB2] The current mode value is 0x00.
igfx @ (DBG) SC: LSPCON::getMode() DInfo: [FB2] The current mode value is 0x00.
igfx @ (DBG) SC: LSPCON::getMode() DInfo: [FB2] The current mode value is 0x01.
igfx @ (DBG) SC: LSPCON::setMode() DInfo: [FB2] The new mode is now effective.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] The adapter is running in preferred mode [Protocol Converter (DP++ to HDMI 2.0)].
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] Will call the original method.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] Returns 0x0.

// 拔出 HDMI 2.0 线
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] called with controller at 0xffffff81a8680000 and framebuffer at 0xffffff81a868c000.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] No LSPCON chip associated with this framebuffer.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] Will call the original method.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB0] Returns 0x0.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] called with controller at 0xffffff81a8680000 and framebuffer at 0xffffff81a869a000.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] LSPCON driver (at 0xffffff802ba3afe0) has already been initialized for this framebuffer.
igfx @ (DBG) SC: LSPCON::setModeIfNecessary() DInfo: [FB2] The adapter is already running in Protocol Converter (DP++ to HDMI 2.0) mode. No need to update.
igfx @ (DBG) SC: LSPCON::wakeUpNativeAUX() DInfo: [FB2] The native AUX channel is up. DPCD Rev = 0x12.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] Will call the original method.
igfx @ (DBG) SC:     GetDPCDInfo() DInfo: [FB2] Returns 0x0.
```

另外你也能在 IORegistryExplorer 下找到驱动在对应的 Framebuffer 下注入的属性。**（此功能仅限 DEBUG 版驱动）**  

`fw-framebuffer-has-lspcon` 显示当前端口是否存在 LSPCON 信号转换器，为布尔值类型。  
`fw-framebuffer-preferred-lspcon-mode` 显示当前指定的 LSPCON 工作模式，为数据类型。1 为 PCON 模式，0 为 LS 模式。  
  
![](Img/lspcon_debug.png)

## 修复 Ice Lake 平台上因 Core Display Clock (CDCLK) 频率过低而导致的内核崩溃问题

为核显添加 `enable-cdclk-frequency-fix` 属性或者直接使用 `-igfxcdc` 启动参数以解决 Core Display Clock (CDCLK) 频率过低而导致的内核崩溃问题。  

核显的显示引擎是由这个 Core Display Clock 时钟来驱动的。苹果的显卡驱动假定 BIOS 或者固件已设定好时钟频率为 652.8 MHz 或者 648 MHz，而有些 Ice Lake 笔记本在开机时 BIOS 自动设定频率为最低的 172.8 MHz，所以会触发频率检查的函数而导致内核崩溃。内核崩溃后，你能看到类似 "Unsupported CD clock decimal frequency 0x158" 这样的错误信息。  

这个补丁通过重新设定 Core Display Clock 的频率来通过上述检查以避免内核崩溃。补丁生效后，时钟速率会被设为一个苹果支持的值，具体是哪个值取决于你的硬件。补丁会基于你当前硬件的配置选择一个最佳的频率。  

<details>
<summary>调试</summary>
补丁生效后，你会在内核日志中发现类似下面的字眼。在调整前，时钟速率为 172.8 MHz，而在调整后速率变为 652.8 MHz。

```
igfx: @ (DBG) CDC: Functions have been routed successfully.
igfx: @ (DBG) CDC: ProbeCDClockFrequency() DInfo: Called with controller at 0xffffff8035933000.
igfx: @ (DBG) CDC: ProbeCDClockFrequency() DInfo: The currrent core display clock frequency is 172.8 MHz.
igfx: @ (DBG) CDC: ProbeCDClockFrequency() DInfo: The currrent core display clock frequency is not supported.
igfx: @ (DBG) CDC: sanitizeCDClockFrequency() DInfo: Reference frequency is 38.4 MHz.
igfx: @ (DBG) CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock frequency will be set to 652.8 MHz.
igfx: @ (DBG) CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock PLL frequency will be set to 1305600000 Hz.
igfx: @ (DBG) CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock PLL has been disabled.
igfx: @ (DBG) CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock has been reprogrammed and PLL has been re-enabled.
igfx: @ (DBG) CDC: sanitizeCDClockFrequency() DInfo: Core Display Clock frequency is 652.8 MHz now.
igfx: @ (DBG) CDC: ProbeCDClockFrequency() DInfo: The core display clock has been switched to a supported frequency.
igfx: @ (DBG) CDC: ProbeCDClockFrequency() DInfo: Will invoke the original function.
igfx: @ (DBG) CDC: ProbeCDClockFrequency() DInfo: The original function returns 0x4dd1e000.
```
</details>

## 修复 Ice Lake 平台上因驱动错误地计算 DVMT 预分配内存大小而导致的内核崩溃问题

为核显添加 `enable-dvmt-calc-fix` 属性或者直接使用 `-igfxdvmt` 启动参数以修复因核显驱动错误地计算当前 DVMT 预分配内存的实际大小而导致后期加速器驱动提示 `Unsupported ICL SKU` 错误并崩溃的问题。

苹果的核显驱动在读取 BIOS 或者 UEFI 固件设定的 DVMT 预分配内存值后，用了一个公式来计算以字节为单位的实际可用内存大小。然而，这个公式只有在预分配内存值为 32MB 的整数倍时才会计算出正确的结果。Ice Lake 平台的笔记本出厂时 DVMT 一般设为了 60MB，所以核显驱动无法正确地初始化内存管理器。即使部分用户可以通过 `EFI Shell` 或者 `RU.EFI` 等特殊手段来修改 BIOS 中设定的 DVMT 预分配内存值，但有些使用特定固件的笔记本比如 Surface Pro 7 以及因厂商安全策略而无法修改 BIOS 设置的笔记本是无法修改 DVMT 设置的。本补丁通过预先计算当前平台的可用 DVMT 预分配内存后，将正确的数值传递给核显驱动。这样驱动可以正常初始化内存管理器，后期的因 DVMT 内存导致的崩溃问题迎刃而解。

苹果在 Ice Lake 的核显驱动中移出了 DVMT Stolen Memory 断言相关的崩溃语句，只会在内核日志中打印出 `Insufficient Stolen Memory`。
请注意，虽然本补丁可让核显的内存管理器正确地初始化，我们仍然建议你给 Framebuffer 打上必要的补丁以规避上述预分配内存不足的问题。  

此外，你可以使用 IORegistryExplorer 在 `IGPU` 下找到 `fw-dvmt-preallocated-memory` 属性来查看当前 BIOS 中设定的 DVMT 预分配内存大小。（仅限 `DEBUG` 版本）
比如下图中的数值为 `0x3C`，对应的十进制为 `60`，即当前 DVMT 预分配内存为 60MB。

![](./Img/dvmt.png)

<details>
<summary>调试</summary>
补丁生效后，你会在内核日志中发现类似下面的字眼。

```
igfx: @ (DBG) DVMT: Found the shll instruction. Length = 3; DSTReg = 0.
igfx: @ (DBG) DVMT: Found the andl instruction. Length = 5; DSTReg = 0.
igfx: @ (DBG) DVMT: Calculation patch has been applied successfully.
```
</details>

## 调整亮度丝滑器设置以提升用户体验

为核显添加 `enable-backlight-smoother` 属性或者直接使用 `-igfxbls` 启动参数以使核显亮度调节变得更丝滑。

核显驱动通过修改亮度相关的寄存器的值来调整笔记本内屏的亮度。亮度丝滑器通过拦截这些写入操作并循循渐进地修改寄存器的值来实现亮度调节更丝滑的效果。
打个比方的话，核显驱动的工作方式犹如走楼梯让屏幕一下子变亮或变暗，而亮度丝滑器好比坐扶梯来让屏幕慢慢地变亮或变暗。
亮度丝滑器首先读取当前亮度档位对应的寄存器值 `SRC` 并计算到目标值 `DST` 的距离 `D`。 而后每 `T` 毫秒向目标值走一步，并在 `N` 步之内走完。
默认情况下，`N` 为 35 且 `T` 为 7，但可通过设备属性 `backlight-smoother-steps` 以及 `backlight-smoother-interval` 来修改它们的值。
然而我们建议 `T` 的值不要高于 10 毫秒，并且达到目标值所需要的时间 `N * T` 不要高于 350 毫秒以避免调节亮度时产生阶梯式卡顿现象。
此外，用户可通过 `backlight-smoother-threshold` 属性来指定一个最小的距离 `DM`，以让驱动检测到 `D` 小于 `DM` 时跳过丝滑器直接向寄存器写入目标值。
默认情况下，`DM` 为 0。

如果不希望笔记本内屏在亮度最低时黑屏，用户可通过 `backlight-smoother-lowerbound` 属性来自定义最低亮度档位对应的寄存器值。
同理，`backlight-smoother-upperbound` 属性控制最高亮度档位对应的寄存器值。请参考下面的例子来找到适合你笔记本的值。
若用户未注入这两个属性的话，BLS 使用默认的区间 [0, 2^32-1]。

<details>
<summary>样例: 为一台 Haswell 笔记本定制亮度丝滑器</summary>

如下的内核日志是从一台 Haswell 笔记本上提取的，显卡型号为 Intel HD Graphics 4600。  
日志反映了用户通过亮度快捷键从最低档位调到最高档位时寄存器值的变化。  
因为每个亮度档位之间对应的寄存器值的距离较短, 我们采用 `N = 25` 以及 `T = 8` 来让内屏在 200 毫秒左右达到下一个档位。  

|          设备属性名称          | 类型  |    值    |             备注            |
|:----------------------------:|:----:|:--------:|:--------------------------:|
|   enable-backlight-smoother  | Data | 01000000 |         启用亮度丝滑器       |
|   backlight-smoother-steps   | Data | 19000000 |  25 (0x19 使用小字节序编码)  |
|  backlight-smoother-interval | Data | 08000000 |  08 (0x08 使用小字节序编码)  |
| backlight-smoother-threshold | Data | 00000000 |  00 (0x00 使用小字节序编码)  |


```
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000000; Target = 0x00000036; Distance = 0054; Steps = 25; Stride = 3.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000036; Target = 0x00000036; Distance = 0000; Steps = 25; Stride = 0.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000036; Target = 0x00000054; Distance = 0030; Steps = 25; Stride = 2.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000054; Target = 0x0000007d; Distance = 0041; Steps = 25; Stride = 2.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x0000007d; Target = 0x000000b2; Distance = 0053; Steps = 25; Stride = 3.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000000b2; Target = 0x000000e7; Distance = 0053; Steps = 25; Stride = 3.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000000e7; Target = 0x000000f5; Distance = 0014; Steps = 25; Stride = 1.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000000f5; Target = 0x00000137; Distance = 0066; Steps = 25; Stride = 3.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000137; Target = 0x00000149; Distance = 0018; Steps = 25; Stride = 1.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000149; Target = 0x000001b1; Distance = 0104; Steps = 25; Stride = 5.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000001b1; Target = 0x0000022b; Distance = 0122; Steps = 25; Stride = 5.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x0000022b; Target = 0x00000271; Distance = 0070; Steps = 25; Stride = 3.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000271; Target = 0x000002b8; Distance = 0071; Steps = 25; Stride = 3.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000002b8; Target = 0x00000359; Distance = 0161; Steps = 25; Stride = 7.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000359; Target = 0x000003a4; Distance = 0075; Steps = 25; Stride = 3.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000003a4; Target = 0x00000401; Distance = 0093; Steps = 25; Stride = 4.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000401; Target = 0x00000413; Distance = 0018; Steps = 25; Stride = 1.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000413; Target = 0x0000046b; Distance = 0088; Steps = 25; Stride = 4.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x0000046b; Target = 0x000004ec; Distance = 0129; Steps = 25; Stride = 6.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000004ec; Target = 0x00000588; Distance = 0156; Steps = 25; Stride = 7.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000588; Target = 0x000005f3; Distance = 0107; Steps = 25; Stride = 5.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000005f3; Target = 0x000006b1; Distance = 0190; Steps = 25; Stride = 8.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000006b1; Target = 0x00000734; Distance = 0131; Steps = 25; Stride = 6.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000734; Target = 0x00000815; Distance = 0225; Steps = 25; Stride = 9.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000815; Target = 0x000008af; Distance = 0154; Steps = 25; Stride = 7.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000008af; Target = 0x000009f7; Distance = 0328; Steps = 25; Stride = 14.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000009f7; Target = 0x00000ad9; Distance = 0226; Steps = 25; Stride = 10.
```

从上面的内核日志可得知，内屏亮度最低时，对应的亮度寄存器的值为 `0x00`。
当用户通过快捷键调高亮度时，寄存器值变为 `0x36`，因此你可以指定寄存器最低值为 `0x18` 或 [0x00, 0x36] 区间内的任意一个值来阻止显示器在最低档位时直接黑屏。
你可能需要安装 DEBUG 版本的 WhateverGreen 并提取内核日志来找到一个适合你笔记本的值。

</details>

<details>
<summary>样例: 为一台 Coffee Lake 笔记本定制亮度丝滑器</summary>

如下的内核日志是从一台 Coffee Lake 笔记本上提取的，显卡型号为 Intel UHD Graphics 630。  
日志反映了用户通过亮度快捷键从最低档位调到最高档位时寄存器值的变化。  
因为每个亮度档位之间对应的寄存器值的距离较短, 我们采用 `N = 35` 以及 `T = 7` 来让内屏在 250 毫秒左右达到下一个档位。  

|          设备属性名称          | 类型  |    值    |             备注            |
|:----------------------------:|:----:|:--------:|:--------------------------:|
|   enable-backlight-smoother  | Data | 01000000 |         启用亮度丝滑器        |
|   backlight-smoother-steps   | Data | 23000000 |   35 (0x23 使用小字节序编码)  |
|  backlight-smoother-interval | Data | 07000000 |   07 (0x07 使用小字节序编码)  |
| backlight-smoother-threshold | Data | 2C010000 | 300 (0x012C 使用小字节序编码) |


```
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000000; Target = 0x000004ae; Distance = 1198; Steps = 35; Stride = 35.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000004ae; Target = 0x00000613; Distance = 0357; Steps = 35; Stride = 11.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000613; Target = 0x000007f3; Distance = 0480; Steps = 35; Stride = 14.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000007f3; Target = 0x00000a4b; Distance = 0600; Steps = 35; Stride = 18.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000a4b; Target = 0x00000e0c; Distance = 0961; Steps = 35; Stride = 28.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00000e0c; Target = 0x000012bb; Distance = 1199; Steps = 35; Stride = 35.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000012bb; Target = 0x000019c6; Distance = 1803; Steps = 35; Stride = 52.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000019c6; Target = 0x0000239b; Distance = 2517; Steps = 35; Stride = 72.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x0000239b; Target = 0x00003043; Distance = 3240; Steps = 35; Stride = 93.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00003043; Target = 0x00004216; Distance = 4563; Steps = 35; Stride = 131.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00004216; Target = 0x000050d5; Distance = 3775; Steps = 35; Stride = 108.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x000050d5; Target = 0x00005aea; Distance = 2581; Steps = 35; Stride = 74.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00005aea; Target = 0x00007d21; Distance = 8759; Steps = 35; Stride = 251.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00007d21; Target = 0x0000acf3; Distance = 12242; Steps = 35; Stride = 350.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x0000acf3; Target = 0x0000effc; Distance = 17161; Steps = 35; Stride = 491.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x0000effc; Target = 0x0001328e; Distance = 17042; Steps = 35; Stride = 487.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x0001328e; Target = 0x00014ead; Distance = 7199; Steps = 35; Stride = 206.
igfx: @ (DBG) BLS: [COMM] Processing the request: Current = 0x00014ead; Target = 0x0001d3cc; Distance = 34079; Steps = 35; Stride = 974.
```

</details>

## 修复 Ice Lake 平台上笔记本开机持续花屏7到15秒的问题

为核显添加 `enable-dbuf-early-optimizer` 属性或者直接使用 `-igfxdbeo` 启动参数以修复 Ice Lake 笔记本开机后内屏短暂花屏的问题。
若发现内核日志记录了如下 DBUF 以及 Pipe Underrun 相关的错误信息，请启用此补丁来修复这些错误。

此补丁通过提前调用优化显示缓冲区分配的函数来修复 DBUF 相关错误。
用户可通过 `dbuf-optimizer-delay` 设备属性来指定具体的延迟时间（单位为秒，值类型为`Data`）。
若用户未指定此属性，*WEG* 将使用如下所述的默认值。

社区用户反馈 1 到 3 秒的延迟可在不影响外接显示器的情况下修复 DBUF 相关错误，而 *WEG* v1.5.4 所用的默认 0 秒延迟可能会导致部分笔记本外接显示器时内屏外屏同时花屏的问题。
从 v1.5.5 开始，默认的延迟时间改为 1 秒，这样用户在通常情况下无需手动添加设备属性来修改延迟时间。

<details>
<summary>包含 DBUF 以及 Pipe Underrun 错误信息的内核日志</summary>

```
[IGFB][ERROR][DISPLAY   ] Display Pipe Underrun occurred on pipe(s) A
[IGFB][ERROR][DISPLAY   ] Internal cached DBuf values are not set. Failed to distribute DBufs
```

</details>

## 已知问题
*兼容性*：
- 受限制的显卡：HD2000 和 HD2500，它们只能用于 IQSV (因为在白苹果中它们只用来干这个)，无解。
- 奔腾/赛扬系列核显无解。
- Haswell 平台的 HDMI 黑屏：请使用 WEG 或使用 macOS 10.13.4 及以上版本。
- SKL 或更新平台的桌面版核显：对两个或更多显示器的支持不完整。在 macOS 10.14 中有改善趋势。
- 显示器无法在 SKL 或更新平台的桌面版核显上唤醒：通过 DP 连接或升级到 macOS 10.14 或许可以解决。

## 花屏和设置
- HD 3000 偶见 UI 花屏：由于 SNB 平台中的视频内存量取决于整个系统内存 —— 因此至少需要 8 GB，但没有稳妥的解决方案。推荐在 BIOS 中安装 [Max TOLUD to Dynamic](https://applelife.ru/posts/595326)（译者注：原文发布于俄文社区，所以并无中文版本）。此外，[这些补丁](https://www.applelife.ru/posts/730496)（译者注：原文发布于俄文社区，所以并无中文版本）也可能有所帮助。
- “八个苹果问题”、在 UEFI GOP 驱动与 macOS 驱动过度阶段，File Vault 2 背景消失问题（由于 EDID 不兼容）：在 *WEG* 中部分解决。
- PAVP 冻屏问题（视频播放期间冻结、快速查看中断等）：通过 *WEG* 解决，代价是禁用 HDCP。
- 部分 Haswell 独有问题可通过语义补丁 `framebuffer-cursormem` 解决。
- macOS 10.14 下，某些 KBL 核显可能会遇到图像文字发虚问题，可临时仿冒核显设备 ID 为 SKL 平台解决。
- 移动版 CFL 核显的数分钟黑屏问题已由 *WEG* 修复。
- BIOS 中缺失更改 FB 内存量选项时：可通过使用 `framebuffer-stolenmem` 和 `framebuffer-fbmem` 语义补丁；或在 UEFI Shell 中手动赋值解决。**否则将会内核崩溃（Kernel Panic）**。[更多解释](https://www.applelife.ru/posts/750369)（译者注：原文发布于俄文社区，所以并无中文版本）
- 一些核显（如 KBL 和 CFL）在低电压模式 (low power state) 下可能会引发系统不稳定的问题，有时 NVMe 驱动引发的崩溃或许与此有关。目前可行的方案是加入 `forceRenderStandby=0` 启动参数以关闭 RC6 Render Standby。请参阅[这个 issue](https://github.com/acidanthera/bugtracker/issues/1193)以获取更多信息。

*性能和媒体内容*：
- 在不受支持的配置（NVIDIA + SNB/SKL/KBL; AMD + Ivy）上，与独显的兼容问题已由 *WEG* 修复，从 macOS 10.13.4 起，该问题已被 Apple 官方解决。
- 受保护的 iTunes 内容问题已由 *WEG* 修复。从 macOS 10.12 起，在 Ivy 或更新平台的核显上，在 iTunes 上观看高清电影时不能没有独显。

在核显非空端口输出下使用 [VDADecoderChecker](https://i.applelife.ru/2019/05/451893_10.12_VDADecoderChecker.zip) 的输出必须类似如下图：

![](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/Img/vda.png)  

对于特殊的 IGPU, IMEI 和 HDEF 设备位置，可使用 [gfxutil](https://github.com/acidanthera/gfxutil): `gfxutil -f IGPU`, `gfxutil -f IMEI`, `gfxutil -f HDEF` 定位。通常来说，IGPU 和 IMEI 的设备位置很标准。

WWHC团队正在寻找天才史蒂夫的转世（引义天才程序员，译者注）加入我们，如果你觉得你可能是，请告诉我们。
