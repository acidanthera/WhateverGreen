//
//  kern_shiki.hpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#ifndef kern_shiki_hpp
#define kern_shiki_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_user.hpp>

class SHIKI {
public:
	void init();
	void deinit();

	/**
	 *  Property patching routine
	 *
	 *  @param patcher  KernelPatcher instance
	 *  @param info     device info
	 */
	void processKernel(KernelPatcher &patcher, DeviceInfo *info);

private:
	// Aside generic DRM unlock patches, Shiki also provides a set of patches
	// to workaround various issues with hardware video acceleration support.
	// These are set as a shikigva boot-arg bitmask.
	// For example, to enable ForceOnlineRenderer, ExecutableWhitelist, and ReplaceBoardID
	// you sum 1 + 8 + 32 = 41 -> and pass shikigva=41.
	enum ShikiGVAPatches {
		// Remove forceOfflineRenderer from AppleGVA Info.plist.
		// This is required to allow hardware acceleration on several mac models with discrete GPUs
		// when only IGPU is available.
		// See /System/Library/PrivateFrameworks/AppleGVA.framework/Resources/Info.plist for more details.
		ForceOnlineRenderer        = 1,
		// Remove hweBGRA from AppleGVA Info.plist.
		// hweBGRA is not supported by NVIDIA GPUs, so the patch is sometimes required when using NVIDIA
		// in a mac model meant to be used with AMD or Intel.
		// See /System/Library/PrivateFrameworks/AppleGVA.framework/Resources/Info.plist for more details.
		AllowNonBGRA               = 2,
		// Prior to 10.13.4 certain GPU+CPU combinations were not meant to provide hardware acceleration and had to be patched.
		// The overall problematic configuration list is: NVIDIA+BDW, NVIDIA+SKL, NVIDIA+KBL, AMD+IVB, NVIDIA+SNB.
		// Enabled automatically if shikigva is *NOT* passed on 10.13.3 and earlier. All are fixed in 10.13.4.
		ForceCompatibleRenderer    = 4,
		// Unlike 10.12.6 without security updates and earlier, on 10.13 and latest 10.12.6 AppleGVA patches
		// do not apply to all processes, and each process needs to be patched explicitly. This is a bug
		// in Lilu, which needs to be explored and fixed. For now this bit ensures that the processes present
		// in WHITELIST section of Patches.plist will definitely get the fixes even on 10.13 and 10.12.6.
		// On 10.12.6 and 10.13 this must be used if any of the following bits are used:
		// - ForceOnlineRenderer
		// - AllowNonBGRA
		// - ForceCompatibleRenderer
		// - ReplaceBoardID
		// - FixSandyBridgeClassName
		// It is enabled automatically on 10.12 and 10.13 if shikigva is *NOT* passed and ForceCompatibleRenderer or
		// FixSandyBridgeClassName are automatically enabled.
		AddExecutableWhitelist     = 8,
		// Use hardware DRM decoder (normally AMD) by pretending to be iMacPro in apps that require it.
		// For example, in Music.app or TV.app for TV+.
		UseHwDrmDecoder            = 16,
		// Replace board-id used by AppleGVA and AppleVPA by a different board-id.
		// Sometimes it is feasible to use different GPU acceleration settings from the main mac model.
		// By default Mac-27ADBB7B4CEE8E61 (iMac14,2) will be used, but you can override this via shiki-id boot-arg.
		// See /System/Library/PrivateFrameworks/AppleGVA.framework/Resources/Info.plist for more details.
		ReplaceBoardID             = 32,
		// Attempt to support fps.2_1 (FairPlay 2.x) in Safari with hardware decoder. Works on most modern AMD GPUs.
		// Note, AMD Polaris Ellesmere is broken in 10.15 (e.g. RX 590), whereas AMD Polaris Baffin (e.g. RX 460) is fine.
		// Easiest check is to run WebKitMediaKeys.isTypeSupported("com.apple.fps.2_1", "video/mp4") in Safari Web Console.
		// Broken GPU driver will just freeze the system with .gpuRestart crash.
		UseHwDrmStreaming          = 64,
		// Disables software decoder unlock patches for FairPlay 1.0.
		// This will use AMD decoder if available, but currently requires IGPU to be either not present or disabled.
		UseLegacyHwDrmDecoder      = 128,
		// Enables software decoder unlock patches for FairPlay 4.0.
		// This will use software decoder, but currently requires IGPU to be either not present or disabled.
		UseSwDrmDecoder            = 256,
	};

	/**
	 *  Current process information
	 */
	UserPatcher::ProcInfo *procInfo {nullptr};

	/**
	 *  Current process information array size
	 */
	size_t procInfoSize {0};

	/**
	 *  Automatic GPU detection is required
	 */
	bool autodetectGFX {false};

	/**
	 *  Disable Shiki, based on mode
	 */
	bool disableShiki {false};

	/**
	 *  Custom board-id set to /shiki-id IOReg to be used by AppleGVA
	 */
	char customBoardID[21] {};

	/**
	 *  iMacPro1,1 board-id used for find-replace (thus the size).
	 */
	uint8_t iMacProBoardId[21] = {"Mac-7BA5B2D9E42DDD94"};

	/**
	 *  Self board-id used for find-replace (thus the size).
	 */
	uint8_t selfBoardId[21] = {};

	/**
	 *  Self mac model for find-replace (thus the size).
	 */
	uint8_t selfMacModel[20] {};

	/**
	 *  Remove requested patches
	 *
	 *  @param section  section to remove patches from
	 */
	void disableSection(uint32_t section);

	/**
	 *  Force compatible renderer patch
	 *
	 *  @return true on success
	 */
	bool setCompatibleRendererPatch();

	/**
	 *  Get overridable boot argument from kernel args (priority) and GPU properties
	 */
	bool getBootArgument(DeviceInfo *info, const char *name, void *bootarg, int size);

	/**
	 *  Get patch by section
	 */
	UserPatcher::BinaryModPatch *getPatchSection(uint32_t section);
};

#endif /* kern_shiki_hpp */
