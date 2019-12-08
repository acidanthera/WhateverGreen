#### DRM Compatibility Chart for 10.15

| Configuration   | Mode           | iTunes Trailers | iTunes Movies | TV+    | Prime Trailers | Prime/Netflix | IQSV |
|:----------------|:---------------|:----------------|:--------------|:-------|:---------------|---------------|------|
| AMD+IGPU, IM/MM | `shikigva=16`  | SW1             | SW2           | HW4    | HW             | NO            | OK   |
| AMD+IGPU, IM/MM | `shikigva=80`  | SW1             | SW2           | HW4    | HW3            | HW3           | OK   |
| AMD, IMP/MP     |                | SW1             | SW2           | HW4    | HW3            | HW3           | NO   |
| AMD, IMP/MP     | `shikigva=128` | HW1             | HW2           | HW4    | HW3            | HW3           | NO   |
| NV+IGPU, IM/MM  |                | SW1             | SW2           | NO     | HW             | NO            | OK   |
| NV, IMP/MP      | `shikigva=256` | SW1             | SW2           | SW3    | HW             | NO            | NO   |
| IGPU, IM/MM     |                | SW1             | NO            | NO     | HW             | NO            | OK   |

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
