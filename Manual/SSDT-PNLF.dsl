// Adding PNLF device for WhateverGreen.kext and others.
// This is a simplified PNLF version originally taken from RehabMan/OS-X-Clover-Laptop-Config repository:
// https://raw.githubusercontent.com/RehabMan/OS-X-Clover-Laptop-Config/master/hotpatch/SSDT-PNLF.dsl
// Rename GFX0 to anything else if your IGPU name is different.
//
// Licensed under GNU General Public License v2.0
// https://github.com/RehabMan/OS-X-Clover-Laptop-Config/blob/master/License.md

DefinitionBlock ("", "SSDT", 2, "ACDT", "PNLF", 0x00000000)
{
    External (_SB_.PCI0.GFX0, DeviceObj)    // (from opcode)

    If (_OSI ("Darwin")) {
        Scope (\_SB.PCI0.GFX0)
        {
            // For backlight control
            Device (PNLF)
            {
             // Name(_ADR, Zero)
                Name (_HID, EisaId ("APP0002"))  // _HID: Hardware ID
                Name (_CID, "backlight")  // _CID: Compatible ID
                // _UID is set depending on PWMMax to match profiles in WhateverGreen.kext https://github.com/acidanthera/WhateverGreen/blob/1.4.7/WhateverGreen/kern_weg.cpp#L32
                // 14: Sandy/Ivy 0x710
                // 15: Haswell/Broadwell 0xad9
                // 16: Skylake/KabyLake 0x56c (and some Haswell, example 0xa2e0008)
                // 17: custom LMAX=0x7a1
                // 18: custom LMAX=0x1499
                // 19: CoffeeLake 0xffff
                // 99: Other (requires custom profile using WhateverGreen.kext via DeviceProperties applbkl-name and applbkl-data)
                Name (_UID, Zero)  // _UID: Unique ID
                Name (_STA, 0x0B)  // _STA: Status
            }

            Method (SUID, 1, NotSerialized)
            {
                ^PNLF._UID = ToInteger (Arg0)
            }
        }
    }
}

