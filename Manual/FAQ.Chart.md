# DRM Compatibility Chart for 10.15

| Configuration   | Mode           | iTunes Trailers | iTunes Movies | TV+  | Prime Trailers | Prime/Netflix | IQSV |
| :-------------- | :------------- | :-------------- | :------------ | :--- | :------------- | ------------- | ---- |
| AMD+IGPU, IM/MM | `shikigva=16`  | SW1             | SW2           | HW4  | HW             | NO            | OK   |
| AMD+IGPU, IM/MM | `shikigva=80`  | SW1             | SW2           | HW4  | HW3            | HW3           | OK   |
| AMD, IMP/MP     |                | SW1             | SW2           | HW4  | HW3            | HW3           | NO   |
| AMD, IMP/MP     | `shikigva=128` | HW1             | HW2           | HW4  | HW3            | HW3           | NO   |
| NV+IGPU, IM/MM  |                | SW1             | SW2           | NO   | HW             | NO            | OK   |
| NV, IMP/MP      | `shikigva=256` | SW1             | SW2           | SW3  | HW             | NO            | NO   |
| IGPU, IM/MM     |                | SW1             | NO            | NO   | HW             | NO            | OK   |

- SW - software unencrypted decoder, works everywhere
- HW - hardware unencrypted decoder, works with any compatible GPU
- SW1 - software FairPlay 1.0 decoder (CoreFP)
- SW2 - software FairPlay 1.0 decoder (CoreFP), requires HDCP
- SW3 - software FairPlay 4.0 decoder (CoreLSKD), requires HDCP and no IGPU
- HW1 - hardware FairPlay 1.0 decoder (CoreFP), requires select AMD GPUs
- HW2 - hardware FairPlay 1.0 decoder (CoreFP), requires HDCP and select AMD GPUs
- HW3 - hardware FairPlay 2.0/3.0 decoder (CoreLSKDMSE), requires HDCP and select AMD GPUs
- HW4 - hardware FairPlay 4.0 decoder (CoreLSKD), requires HDCP and select AMD GPUs

- IM/MM - iMac/Macmini models with IGPU, IGPU must have connector-less framebuffer-id when AMD/NV is used.
- IMP/MP - iMacPro/MacPro models without IGPU.
- Other configurations are used at your own risk, use `-shikioff` to disable modifications

# DRM Compatibility on macOS 11+ (Big Sur and later)

Shiki is deprecated so the shikigva device property/boot-arg is no longer functional.
Instead a partial solution unfairgva is provided with the following boot-arg options in a bitmask fashion:

unfairgva = 1 -> enables DRM on old CPUID
unfairgva = 2 -> Relax HDCP requirements
unfairgva = 1+2 = 3 -> enables the options 1 and 2 above
unfairgva = 4 -> Inject the iMacPro1,1 board ID and therefore forces AMD video decoder/encoder.
unfairgva = 4+1 = 5 -> enables the options 4 and 1
unfairgva = 4+2 = 6 -> enables the options 4 and 2
unfairgva = 4+2+1 = 7 -> enables all

These options can also be injected through the unfairgva device property

Things to keep in mind:

- All kinds of software DRM decoders were removed from macOS 11
- All kinds of legacy hardware DRM decoders (e.g. NVIDIA VP3) were removed from macOS 11
- Only IGPU-free Mac models allow for full DRM content access given a compatible AMD GPU video decoder
- Forcing the use of the AMD Video DRM decoder on a iGPU+dGPU setup can be done in 3 ways:
  1. SMBIOS iMacPro1,1 or MacPro7,1
  2. use "unfairgva = 4" (5, 6 or 7)
  3. Use the first override in the list below in MacOS terminal
  
  The latter option is preferred as it permit switching back and forth between iGPU and dGPU video processor without rebooting the computer.
  Usability of the Sidecar is compromised when forcing AMD DRM as Sidecar relies on HEVC encoding and AMD video processor is not as efficient as intel's.
  Some AMD GPUs also are reported to show a drop of 35-40% in metal and openGL performance when AMD video processing is forced. Other side effects can be expercted

- AMD GPU video decoder preference can be chosen through preferences overrides for some types of DRM content (like Apple TV and iTunes movie streaming).  

List of overrides:

- `defaults write com.apple.AppleGVA gvaForceAMDKE -boolean yes` forces AMD DRM decoder for streaming services (like Apple TV and iTunes movie streaming)
- `defaults write com.apple.AppleGVA gvaForceAMDAVCDecode -boolean yes` forces AMD AVC accelerated decoder
- `defaults write com.apple.AppleGVA gvaForceAMDAVCEncode -boolean yes` forces AMD AVC accelerated encoder
- `defaults write com.apple.AppleGVA gvaForceAMDHEVCDecode -boolean yes` forces AMD HEVC accelerated decoder
- `defaults write com.apple.AppleGVA disableGVAEncryption -string YES` forces AMD HEVC accelerated decoder
- `defaults write com.apple.coremedia hardwareVideoDecoder -string force` forces hardware accelerated video decoder (for any resolution)
- `defaults write com.apple.coremedia hardwareVideoDecoder -string disable` disables hardware accelerated video decoder (in QuickTime / Apple TV)
