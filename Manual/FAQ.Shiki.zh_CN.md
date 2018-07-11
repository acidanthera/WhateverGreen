#### 常见问题:
- _我需要 Shiki 吗?_  
如果您有一台 Ivy Bridge 或者更新的平台机器，并且无法使用 iTunes DRM 播放但您的显卡可以在 HDCP 模式下工作的话，您可以尝试使用 Shiki 。
有时 Shiki 也可以修复 Sandy Bridge 平台的这些问题。

- _如何禁用 Shiki?_  
请加入 `-shikioff` 启动系统。另外，在使用 -x (安全模式) 或 -s (单用户模式) 启动时 Shiki 也不会载入。

- _如何打开 Shiki 的排错模式? (需配合 DEBUG 版本 Shiki 使用)_  
请加入 `-shikidbg` 启动系统。

- _Shiki 能配合什么版本的 OS X/macOS 工作?_  
理论上来说 10.9 或者更新的系统即可，不过，建议在 10.10 或者更新的系统上使用。  
注意: Shiki 需要配合相对新的 iTunes 版本工作。 
如果您在 10.10 或者更新的系统上使用 Shiki 遇到了一些问题，请尝试使用 `-liluslow` 或 `-lilufast` 启动系统。

- _如何使用 Shiki?_  
建议使用引导器提供的注入 kext 功能来加载 Shiki 。目前已知 Shiki 无法通过 `kextload` 加载；将其安装在 /System/Library/Extensions 或者 /Library/Extensions 也可能会失败。

- _使用 Shiki 很危险吗?_  
目前 Shiki 比较稳定，不过仍在测试阶段。

- _Shiki 修改了磁盘上的文件吗?_  
近期版本的 Shiki 不会修改磁盘上的内容，但一些 I/O 指令可能会提示某些文件被修改。(这与一些 API hook 技术有关)

- _需要修改 SIP (rootless) 配置吗?_  
不，不需要。

- _在测试 Shiki 之前，如何确定系统已正确配置?_
您可以查阅[配置列表](https://github.com/vit9696/Shiki/blob/master/Manual/FAQ.zh_CN.md#配置列表)，这里面列出了所有的配置要求。

- _在使用 Shiki 之前，为什么应该正确配置 VDA ?_  
理论上来说这可能不必要，不过这会带来未知的结果。

- _哪些显卡可能会引发卡顿?_
   - 使用 Azul 驱动的显卡 (如 HD 4400, HD 4600) 在没有独立显卡并在使用完整接口的 ig-platform-id 时可能因为不工作的 HDCP 而无法播放高清视频，但这不是 Shiki 导致的，您需要 [IntelGraphicsFixup.kext](https://sourceforge.net/p/intelgraphicsfixup) 来解决这个问题。

- _我的机器会被禁止吗?_  
如果您可以播放一个电影的预告片，但购买后的电影无法播放，即使已经对此电脑进行了授权，那么您的 NIC MAC 可能已被禁止。有时可以通过注销账户，并稍等片刻来重新授权来解决，但如果这无效的话，您可能需要修改以太网 MAC 地址。
查阅[系统配置常见问题](https://github.com/vit9696/Shiki/blob/master/Manual/FAQ.zh_CN.md#系统配置常见问题)以获取 libHookMac 的详细用法，如果它可以工作的话，尝试修改您的以太网卡 MAC 地址。(或使用其他方式修改亦可)

- _Shiki 开源吗?_  
从 2.0.0 版本起开源。

- _如何下载一个受 DRM 保护的视频来测试 Shiki 是否正常工作?_  
首先请在 终端 输入: `defaults write com.apple.coremedia cfbyteflume_trace 1`  
然后在 控制台 中 过滤 栏中输入 iTunes ，您会看到一个影片预告片的地址，像下面这样:
`... <<< CFByteFlume >>> FigCFHTTPCheckCacheValidator: Comparing dictUrl = http://.....m4v, url = http://......m4v`  
粘贴到浏览器中，即可下载此影片。
或者，这里有一个例子，您可以直接下载[此影片](http://rgho.st/download/private/7df2dy85Y/8e91160b88b19984abda5dbd9247d579/cb78ad46934391d67ab638718f6dd466267a217c/1.m4v)。

- _为什么 DRM 播放一段时间后开始出现一些错乱?_  
从测试中来看这似乎是 Apple DRM 解码器的问题，并且在某些白苹果上也存在。
基本上这样的问题不会出现，除了在某些 1080p 高比特率的视频上，如遇到这样的问题，请尝试重新启动电脑，重设 DRM 配置(查阅[系统配置常见问题](https://github.com/vit9696/Shiki/blob/master/Manual/FAQ.zh_CN.md#系统配置常见问题))，检查电源管理是否正常工作。

- _可以用 Shiki 播放 HTML5 Netfilx 视频吗?_  
不，Netflix 限制了 1080p 的某些视频，只有极少数的机型才可以播放。

- _可以用 Shiki 修改其他系统运行库吗?_  
可以，这是完全可能的。

#### 系统配置常见问题:
- _如何确定硬件解码可用?_  
运行 [VDADecoderChecker for 10.11](https://applelife.ru/threads/shiki-patcher-polzovatelskogo-urovnja.1349123/page-2#post-595056)/[VDADecoderChecker for 10.12](https://applelife.ru/threads/shiki-patcher-polzovatelskogo-urovnja.1349123/page-26#post-647746) ([或者自行编译](https://github.com/cylonbrain/VDADecoderCheck)) 然后查阅它的输出:  
`GVA info: Successfully connected to the Intel plugin, offline Gen75`  
`Hardware acceleration is fully supported`

- _我如何确定 IMEI/IGPU 存在于 IOReg 中?_  
终端 中执行 `ioreg | grep IMEI` ，确保存在像这样的输出:  
`    | |   +-o IMEI@16  <class IOPCIDevice, id 0x100000209, registered, matched, active, busy 0 (6 ms), retain 11>`

- _如何确认当前的 ig-platform-id?_  
终端 中执行 `ioreg -l | grep platform-id` ，确保存在像这样的输出:  
`    | |   | |   "AAPL,ig-platform-id" = <04001204>`  
`04 00 12 04` 即为当前的 ig-platform-id 。

- _如何启用 AppleGVA 排错模式?_  
终端 中执行:  
`defaults write com.apple.AppleGVA gvaDebug -boolean yes`  
`defaults write com.apple.AppleGVA enableSyslog -boolean yes`

- _如何启用 FP 排错模式?_  
终端 中执行:  
`defaults write com.apple.coremedia fp_trace 2`  

- _如何设置硬件解码器偏好设置 (一些 AMD/ATI 显卡和较老的 NVIDIA 可能需要)?_  
终端 中执行下面的命令***之一***:  
`defaults write com.apple.AppleGVA forceNV -boolean yes`  — 启用 NVIDIA 解码器  
`defaults write com.apple.AppleGVA forceATI -boolean yes` — 启用 ATI 解码器  
`defaults write com.apple.AppleGVA forceIntel -boolean yes` — 启用 Intel 解码器  
`defaults write com.apple.AppleGVA forceSWDecoder -boolean yes` — 启用 software 解码器  
`defaults write com.apple.coremedia hardwareVideoDecoder disable` — 禁用 硬件 解码器  
`defaults write com.apple.coremedia hardwareVideoDecoder force` — 启用 硬件 解码器  
注: 多数情况下这些是不需要的，错误的设定可能会导致 CL、VDA 解码不正常。

- _如何重设 DRM 配置 (用于修复闪退、崩溃等问题)?_  
终端 中执行下面的命令:  
`defaults delete com.apple.coremedia`  
`defaults delete com.apple.AppleGVA`  
`sudo rm -rf /Users/Shared/SC\ Info`  
`sudo defaults delete com.apple.coremedia`  
`sudo defaults delete com.apple.AppleGVA`  
在这之后确保以下命令不会输出任何内容 (表明 AppleGVA 已正确签名):  
`codesign --no-strict --verify /System/Library/PrivateFrameworks/AppleGVA.framework`  
如果以上命令输出了内容，尝试恢复原版的 AppleGVA.framework ，并正确设置权限。
重新启动两次。

- _如何通过 [libHookMac.dylib](https://github.com/vit9696/Shiki/raw/master/HookMac/libHookMac.dylib) 修改 NIC MAC 地址?_  
   - 禁用 SIP (System Integrity Protection);  
   - 终端 中执行以下命令 (请自行替换 libHookMac.dylib 的完整路径以及新的 MAC 地址):   
    `DYLD_INSERT_LIBRARIES=/full/path/to/libHookMac.dylib MAC=00:11:22:33:44:55 /Applications/iTunes.app/Contents/MacOS/iTunes`;
   - 您将会看到对应的输出，如果成功的话。

- _如何确认 共享缓存 可用?_  
终端 中执行以下两段命令，并比较它们的输出:  
`DYLD_PREBIND_DEBUG=1 DYLD_SHARED_CACHE_DONT_VALIDATE=1 "/Applications/QuickTime Player.app/Contents/MacOS/QuickTime Player"`  
`DYLD_PREBIND_DEBUG=1 "/Applications/QuickTime Player.app/Contents/MacOS/QuickTime Player"`  
如果两者差异很大，并且第二段命令输出了很多路径，则表明 共享缓存 已损坏。
若 共享缓存 已损坏，请禁用 Shiki (通过 `-Shikioff` 启动)，并在 终端 中运行: `sudo update_dyld_shared_cache -force`

- _哪些机型支持硬件加速?_  
请查阅 `/System/Library/PrivateFrameworks/AppleGVA.framework/Info.plist`, 如果您看到了您的机型, 则表明这个机型支持硬件解码加速。 您应该选择一个与您机器配备最接近的型号。 比如， iMac13,1 配备一个核芯显卡和一个独立显卡，然而 iMac13,3 只有一个核芯显卡。如果您使用的机型使用独立显卡，但没有核芯显卡，VDA 将不会工作，并且您可能会从 VDADecoderChecker 看到错误信息。欲修正此错误，您可以选择一个正确的机型，或是修改 Info.plist 中的 `forceOfflineRenderer` 项，它需要被设为 `NO` 。

- _如何启用 Intel 在线解码器如果 AppleGVA 强制将其设置为离线模式?_  
请加入 `shikigva=1` 启动系统。

- _如何为某些 NVIDIA 显卡注入 IOVARendererID/IOVARendererSubID 属性?_
NVIDIA 显卡驱动没有正确加入这些对 Maxwell 架构显卡来说必要的用于 VDA 解码的键值，您可以通过一个 Info 空壳来加入它们。以下是正确的值：
IOVARendererID    → `<08 00 04 01>`  
IOVARendererSubID → `<03 00 00 00>`  
VP3 需要一个不同的 IOVARendererID → `<04 00 04 01>`.  
感谢 igork 的发现。
或者，亦可使用 [NvidiaGraphicsFixup](https://sourceforge.net/p/nvidiagraphicsfixup) (需使用 1.2.0 或更高版本) 来自动设定这些值。  
Intel Skylake (第六代) 或更新平台的核芯显卡需使用对 AppleGVA 的一组补丁以配合 NVIDIA 独立显卡工作，您可以通过加入 `shikigva=4` 参数启用此补丁。

- _如果 AMD 显卡无法使用 VDA 解码器_  
使用 Intel 核芯显卡作为主显卡，并注入一个所有 connectors 都可用的 ig-platform-id (如 HD 4000 使用 `<03 00 66 01>`；HD 4600 使用 `<03 00 22 0D>`)，这似乎改善了某些 AMD 显卡的情况。(如 HD 7750)

- _如果启用 ATI 解码器后，AMD 显卡已成功启用硬件加速解码，但 DRM 解码仍不工作_  
某些 AMD 显卡如 HD 7750，支持硬件加速解码，但无法解码 DRM 视频。原因不明，请正常使用 Shiki 。

- _[BoardHash](https://github.com/vit9696/Shiki/raw/master/BoardHash/BoardHash) 是做什么的?_  
BoardHash 可以用来生成 board-id 的哈希值，类似于 CoreAUC.framework 中的 _PsZXJ2EK7ifxrtgc 函数。
比如， Mac-F221BEC8 (MacPro5,1) 对应 5f571162ce99350785007863627a096bfa11c81b.  
目前已知 MacPro5,1 可以无视解码器状态而直接允许 HD 4000 播放高清视频。

- _如何为 HD 4400, HD 4600 以及 HD 530 禁用 PAVP/HDCP ?_  
考虑使用 [IntelGraphicsFixup.kext](https://sourceforge.net/p/intelgraphicsfixup) 以禁用 PAVP/HDCP 来避免冻屏。

#### 配置列表

- _如果您拥有以下配置，Shiki 是不需要的:_  
   - 可用的 DRM VDA AMD 显卡 (如 HD 7870, HD 6670 和 HD 7970);
   - 可用的 DRM VDA NVIDIA 显卡 (可能 2xx 系列以及一些其他的型号).  

  这些配置不确定具体情况，不过据一些朋友反馈可能不需要 Shiki :
   - 已启用核芯显卡并且为主显卡，并且 ig-platform-id 包含所有 connectors (如 `<03 00 66 01>`)，或者完全移除/禁用，IOReg 中没有相关痕迹 (如通过 [D2EN register](https://applelife.ru/threads/chernye-trejlery-itunes.42290/page-14#post-584519));
   - 硬件视频解码器偏好设置被设定为 ATI/NVIDIA (取决于具体安装的显卡);
   - VDADecoderChecker 确认 VDA 解码器在 VP3 (NVIDIA) 或者 AMD 解码器可用;
   - 机型为一个支持硬件加速的型号

- _适用于无冻屏的 Intel 显卡或者任何独立显卡的情况:_  
   - 已启用核芯显卡， 并且 ig-platform-id 不包含所有 connectors (如 `<04 00 12 04>`, `<07 00 62 01>`), 在只有核芯显卡的情况下使用全部 connectors 的 FrameBuffer;
   - 正确配置的独立显卡，并且 IOReg 中存在 IOVARendererID 属性;
   - VDADecoderChecker 确认 Intel 离线 (在线) VDA 解码器正常工作;
   - IOReg 中存在 IMEI 设备;
   - 无覆盖的偏好设置使用;
   - 机型为一个支持硬件加速的型号

- _适用于 HD4400, HD4600 无独立显卡的情况:_  
   - 已启用核芯显卡，并且 ig-platform-id 包含所有 connectors (如 `<03 00 22 0d>`, `<00 00 16 0a>`)。将 IGPU 重命名为 GFX0 或任意其他名称可能会降低冻屏出现的可能;
   - 显卡驱动/FrameBuffer 已禁用 PAVP/HDCP;
   - 硬件视频解码器已通过 defaults 指令禁用 (否则视频在唤醒后将无法播放);
   - IOReg 中存在 IMEI 设备;
   - 机型为一个支持硬件加速的型号

- _适用于不受原生支持的 CPU (如 Haswell Celeron E):_  
  硬件视频加速解码不会在这些 CPU 上工作，以及您需要 FakeCPUID 来启动。
  完全禁用核芯显卡，或者将其重命名为其他的名字 (如 IGFX)，安装 Shiki 。
  目前不确定需要哪些偏好设置，不过已知通过修改 AppleGVA 中的 plist 或使用 MacPro5,1 机型有时或许可以播放高清视频。
  

_感谢: 07151129, Andrey1970, Сашко666, chrome, family1232009, garcon, iDark Soul, igork, lvs1974, m-dudarev, Mieze, Quadie, savvas, tatur_sn, 以及一些其他帮助开发 Shiki 的朋友。_

译者: [PMheart](https://github.com/PMheart)
