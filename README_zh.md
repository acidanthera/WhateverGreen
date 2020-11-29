WhateverGreen
=============

[![Build Status](https://travis-ci.com/acidanthera/WhateverGreen.svg?branch=master)](https://travis-ci.com/acidanthera/WhateverGreen) [![Scan Status](https://scan.coverity.com/projects/16177/badge.svg?flat=1)](https://scan.coverity.com/projects/16177)

[Lilu](https://github.com/acidanthera/Lilu) 插件为macOS上的特定GPU提供补丁。需要Lilu 1.4.0或更新版本。

[English] (README.md)
简体中文 (当前)

#### 特性

- 修复AMD与NVIDIA显卡的开机黑屏问题
- 修复AMD显卡睡眠唤醒后黑屏的问题
- 修复某些情况下开机画面变形的问题
- 修复自动检测的接口的传输/编码，以支持多显示器 (`-raddvi`)
- 修复HD 7730, 7750, 7770, R7 250, R7 250X的初始化 (`radpg=15`)
- 允许通过ACPI调整aty_config, aty_properties, cail_properties
- 允许在不支持的显示器上执行24位模式 (`-rad24`)
- 允许在没有视频加速的情况下启动 (`-radvesa`)
- 允许为RadeonFramebuffer自动设置GPU型号名称或手动提供
- 允许通过设备属性为RadeonFramebuffer指定自定义接口
- 允许通过设备属性调整自动检测连接器的优先级 (HD 7xxx 或更新显卡)
- 修复了AppleGraphicsDevicePolicy.kext中的一个问题，这样我们就可以使用MacPro6,1的board-id/model组合，而不会出现通常的黑屏挂起。 [Patching AppleGraphicsDevicePolicy.kext](https://pikeralpha.wordpress.com/2015/11/23/patching-applegraphicsdevicepolicy-kext)
- 修改 macOS，使其能够将 NVIDIA 的 Web 驱动程序识别为平台二进制文件。这解决了使用 Metal 并启用了 Library Validation 的应用程序会出现没有内容的透明窗口的问题。常见的受影响应用程序有iBooks和Little Snitch Network Monitor，不过这个补丁是通用的，可以解决所有问题。 [NVWebDriverLibValFix](https://github.com/mologie/NVWebDriverLibValFix)
- 将IOVARendererID注入到GPU属性中 (需要基于Shiki的解决方案，用于非冻结的Intel和/或任何独立显卡)
- 适用于英特尔高清数字音频HDMI、DP、数字DVI (为 connector-type DP -> HDMI 打补丁)
- 修正NVIDIA GPU接口在10.13上的卡顿问题 (官方驱动和Web驱动)
- 修正了在某些使用 Intel IGPU 的笔记本电脑上，DPCD 报告无效链接速率所导致的Kernel Panic。
- 修正在 Skylake, Kaby Lake 和 Coffee Lake 平台上以较高像素时钟速率建立 Intel HDMI 连接时的无限循环问题。
- 实现对板载 LSPCON 芯片的驱动程序支持，以便在某些配备 Intel IGPU 的平台上实现 DisplayPort 到 HDMI 2.0 的输出。
- 在 Kaby Lake 及更新版本的非内置显示器上强制执行完整模式集，以解决开机黑屏问题。
- 允许不支持的显卡使用HW视频编码器 (`-radcodec`)
- 修复英特尔Kaby Lake及更新型号上视频播放不稳定的问题。
- 修正Intel HD自10.15.5以来的黑屏问题。
- 增加了在Intel KBL和CFL上出现的罕见强制唤醒超时panic的解决办法。
- 支持Intel ICL平台上所有有效的核心显示时钟(CDCLK)频率。
- 修正了在 Intel ICL 平台上，由于 DVMT 预分配内存的计算错误而导致的内核恐慌。

#### 文档

阅读 [FAQs](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/) 且避免询问任何问题。 目前不提供任何支持。

#### 启动参数

- `-wegdbg` 启用调试输出 (在DEBUG二进制文件中可用).
- `-wegoff` 禁用 WhateverGreen.
- `-wegbeta` 在不支持的操作系统版本上启用WhateverGreen (11及以下版本默认启用).
- `-wegnoegpu` 禁用外部GPU (或添加 `disable-external-gpu` 属性至IGPU).
- `-radvesa` 完全禁用ATI/AMD视频加速.
- `-rad24` 强制执行24位显示模式.
- `-raddvi` 启用DVI发射器校正 (290X, 370等需要).
- `-radcodec` 强制在AMDRadeonVADriver中使用欺骗的PID
- `radpg=15` 禁用几种 power-gating 模式 (见 FAQ, Cape Verde 显示芯片必须启用).
- `agdpmod=vit9696` 禁用 `board-id` 检测 (或将 `agdpmod` 属性添加到外部 GPU).
- `agdpmod=pikera` 用 `board-ix` 代替 `board-id`
- `agdpmod=ignore` 禁用AGDP补丁 (`vit9696,pikera` 值是外部GPU的默认值)
- `ngfxgl=1` 启动参数 (与 `disable-metal` 属性) 用于在NVIDIA显卡上禁用Metal支持
- `ngfxcompat=1` 启动参数 (与 `force-compat` 属性) 忽略NVDAStartupWeb的兼容性检查
- `ngfxsubmit=0` 启动参数 (与 `disable-gfx-submit` 属性) 用于禁用10.13版本的界面停顿修复。
- `gfxrst=1` 在第二启动阶段优先绘制苹果标志，而不是复制帧缓冲区。
- `gfxrst=4` 在第二启动阶段禁用帧缓冲区启动交互。
- `igfxframe=frame` 给IGPU注入一个专用的framebuffer标识符 (仅用于测试目的)。
- `igfxsnb=0` 禁用Sandy Bridge CPU的IntelAccelerator名称修正。
- `igfxgl=1` 启动参数 (与 `disable-metal` 属性) 禁用英特尔的Metal支持。
- `igfxmetal=1` 启动参数 (与 `enable-metal` 属性) 强制启用英特尔对离线渲染的Metal支持。
- `igfxpavp=1` 启动参数 (与 `igfxpavp` 属性) 强制启用PAVP输出。
- `igfxfw=2` 启动参数 (与 `igfxfw` 属性) 强制加载Apple GuC固件。
- `-igfxvesa` 禁用英特尔图形加速。
- `-igfxnohdmi` 启动参数 (与 `disable-hdmi-patches`) 禁用DP到HDMI的数字声音转换补丁。
- `-igfxtypec` 强制DP连接Type-C平台。
- `-cdfon` (与 `enable-hdmi20` 属性) 启用HDMI 2.0补丁。
- `-igfxdump` 转储IGPU framebuffer kext到 `/var/log/AppleIntelFramebuffer_X_Y` (在DEBUG二进制文件中可用)。
- `-igfxfbdump` 将原生的和经过修补的帧缓冲表转储到 IOService:/IOResources/WhateverGreen 处的ioreg。
- `igfxcflbklt=1` 启动参数 (与 `enable-cfl-backlight-fix` 属性) 启用CFL背光补丁。
- `applbkl=0` 启动参数 (与 `applbkl` 属性) 可以禁用IGPU的AppleBacklight.kext补丁。在自定义AppleBacklight配置文件的情况下- [阅读这里.](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/FAQ.OldPlugins.en.md)
- `-igfxmlr` 启动参数 (与 `enable-dpcd-max-link-rate-fix` 属性) 用于应用最大链接速率修复。
- `-igfxhdmidivs` 启动参数 (与 `enable-hdmi-dividers-fix` 属性) 修复在SKL、KBL和CFL平台上以较高像素时钟速率建立Intel HDMI连接时的无限循环。
- `-igfxlspcon` 启动参数 (与 `enable-lspcon-support` 属性) 用于启用驱动程序对板载LSPCON芯片的支持. [阅读手册](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/FAQ.IntelHD.en.md)
- `-igfxi2cdbg` 启动参数 用于在I2C-over-AUX事务中启用verbose输出 (仅用于调试目的)。
- `igfxagdc=0` 启动参数 (`disable-agdc` 设备属性) 用于禁用AGDC。
- `igfxfcms=1` 启动参数 (`complete-modeset` 设备属性) 用于强制Skylake或Apple固件使用完全模式集。
- `igfxfcmsfbs=` 启动参数 (`complete-modeset-framebuffers` 设备属性) 指定必须强制执行完整模式集的连接器的索引。每个索引是一个字节，在 一个64位字；例如，值 `0x010203` 指定连接器1、2、3。如果一个连接器的值不在列表中，驱动的逻辑判断是否需要完整的模式集。传递`-1`来禁用。
- `igfxonln=1` 启动参数 (`force-online` 设备属性) 用于强制所有显示器的在线状态。
- `igfxonlnfbs=MASK` 启动参数 (`force-online-framebuffers` 设备属性) 用于指定在线状态。执行在线状态的连接器的索引。格式与 `igfxfcmsfbs` 类似。
- `wegtree=1` 启动参数 (`rebuild-device-tree` 属性) 用于强制Apple FW上的设备重命名。
- `igfxrpsc=1` 启动参数 (`rps-control` 属性) 用于启用RPS控制补丁 (提高IGPU性能)。
- `-igfxcdc` 启动参数 (`enable-cdclk-frequency-fix` 属性) 用于支持ICL平台上所有有效的核心显示时钟(CDCLK)频率。 [阅读手册](https://github.com/acidanthera/WhateverGreen/blob/master/Manual/FAQ.IntelHD.en.md)
- `-igfxdvmt` 启动参数 (`enable-dvmt-calc-fix` 属性) 用于修复英特尔ICL平台上DVMT预分配内存计算错误导致的内核恐慌。

#### 致谢

- [Apple](https://www.apple.com) 提供 macOS
- [AMD](https://www.amd.com) 提供 ATOM VBIOS 解析代码
- [The PCI ID Repository](http://pci-ids.ucw.cz) 提供多个GPU型号名称
- [FireWolf](https://github.com/0xFireWolf/) 提供DPCD最大链接速率修复、Intel HDMI连接的无限循环修复、LSPCON驱动支持、ICL平台的Core Display时钟频率修复与ICL平台的DVMT预分配内存计算修复
- [Floris497](https://github.com/Floris497) 提供 CoreDisplay [patches](https://github.com/Floris497/mac-pixel-clock-patch-v2)
- [Fraxul](https://github.com/Fraxul) 提供原版CFL背光补丁
- [headkaze](https://github.com/headkaze) 提供英特尔帧缓冲区补丁代码和CFL背光补丁改进
- [hieplpvip](https://github.com/hieplpvip) 提供初始AppleBacklight补丁插件
- [igork](https://applelife.ru/members/igork.564/) 提供power-gating补丁的发现和各种FP研究
- [lvs1974](https://applelife.ru/members/lvs1974.53809) 提供对于Intel和NVIDIA代码修复的持续改进
- [mologie](https://github.com/mologie/NVWebDriverLibValFix) 提供NVWebDriverLibValFix.kext，用于强制macOS将NVIDIA Web驱动识别为平台二进制文件
- [PMheart](https://github.com/PMheart) 提供CoreDisplay补丁代码与Intel修复补丁向后移植
- [RehabMan](https://github.com/RehabMan) 提供各种增强功能
- [RemB](https://applelife.ru/members/remb.8064/) 一直提供睡眠唤醒研究与为AMD问题找到正确的register
- [Vandroiy](https://applelife.ru/members/vandroiy.83653/) 用于维护GPU模型检测数据库 
- [YungRaj](https://github.com/YungRaj) 和 [syscl](https://github.com/syscl) 提供Intel修复补丁向后移植
- [vit9696](https://github.com/vit9696) 提供软件的编写与维护