DefinitionBlock ("", "SSDT", 2, "ACDT", "AMDGPU", 0x00001000)
{
    External (_SB_.PCI0, DeviceObj)
    External (_SB_.PCI0.GFX0, DeviceObj)
    External (_SB_.PCI0.PEG0, DeviceObj)
    External (_SB_.PCI0.PEG0.PEGP, DeviceObj)

    Scope (\_SB.PCI0)
    {
        // Follow your motherboard structure regarding the scope hierarchy. This example contains:
        // PEG0 -> PEGP (GPU #1) and HDAU (HDMI audio for GPU #1)
        // PEG1 -> PEGP (GPU #2) not installed
        // PEG2 -> PEGP (GPU #3) not installed
        // PEG3 -> PEGP (GPU #4) not installed
        // IMEI (Intel IMEI device required for proper hardware video decoding functioning)
        // GFX0 (disabled original IGPU name)
        // IGPU (Intel GPU with a connector-less frame used for hardware video decoding)
        //
        // Depending on your method you may rename PEGP to GFX0.
        Scope (PEG0)
        {
            Scope (PEGP)
            {
                Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
                {
                    Store (Package ()
                    {
                        // Write the preferred GPU index here (1-4)
                        "AAPL,slot-name",
                        Buffer ()
                        {
                            "Slot-1"
                        },

                        // Write the main main monitor index (currently 0) here, the value does not matter
                        "@0,AAPL,boot-display",
                        Buffer (Zero) {}

                        /*

                        // Only use this if your GPU device identifier is not natively supported in macOS kexts
                        "device-id",
                        Buffer (0x04)
                        {
                             0x12, 0x34, 0x00, 0x00
                        },

                        // Only use this if automatic detection fails or you are faking your GPU device-id
                        "model",
                        Buffer ()
                        {
                            "AMD Radeon HD 6450"
                        },

                        // Only use this if you are not satisfied with automatic HDMI injection
                        // Make sure not to write the same onboard index for each installed GPU
                        "hda-gfx",
                        Buffer ()
                        {
                            "onboard-1"
                        },

                        // Only use this if you need special connectors that are incompatible with the automatic connector detection
                        "connectors",
                        Buffer ()
                        {
                            0x00, 0x04, 0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x12, 0x04, 0x04, 0x01, 
                            0x00, 0x08, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x22, 0x05, 0x01, 0x03, 
                            0x04, 0x00, 0x00, 0x00, 0x14, 0x02, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x10, 0x00, 0x05, 0x06, 
                            0x04, 0x00, 0x00, 0x00, 0x14, 0x02, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x11, 0x02, 0x06, 0x05
                        },

                        // You may only specify this with connectors property when the amount of connectors differs from autodetected
                        "connector-count",
                        Buffer ()
                        {
                             0x04, 0x00, 0x00, 0x00
                        },

                        // Only use this with automatic connector detection when you need manual priority.
                        // Each value is a sense id to get a higher priority.
                        // Automatic ordering is done by type: LVDS, DVI, HDMI, DP, VGA.
                        // You may leave this empty to order all the connectors by type.
                        // Assume autodetected connectors: 0x03 DP, 0x02 DVI_D, 0x06 HDMI, 0x05 DVI_D, 0x04 VGA
                        // With the value pri will become: 0x0005 , 0x0001    , 0x0004   , 0x0003    , 0x0002
                        "connector-priority",
                        Buffer ()
                        {
                            0x02, 0x04
                        },

                        // The properties below allow you to configure aty_config, aty_properties, cail_properties

                        // This will change CAIL_DisableDrmdmaPowerGating in cail_properties to false
                        // Note: buffer of 1 byte long with 0x01 / 0x00 values is transormed into boolean
                        "CAIL,CAIL_DisableDrmdmaPowerGating",
                        Buffer ()
                        {
                            0x01
                        },

                        // This will change CFG_FB_LIMIT in aty_config to 6
                        // Starting with 10.13.2 AAPL hardcodes framebuffer limit in certain kexts.
                        // In particular, for AMD9500Controller the value is 6.
                        // This is not correct for some GPUs (e.g. Radeon Pro, which has 3), so you are to specify
                        // the correct value right here.
                        "CFG,CFG_FB_LIMIT",
                        0x06,

                        // This will change HELLO_WORLD in aty_properties to <01 02 03 04>
                        "PP,HELLO_WORLD",
                        Buffer ()
                        {
                            0x01, 0x02, 0x03, 0x04
                        }

                        // The properties below are usually relevant to mobile ATI/AMD GPUs

                        // Most mobile GPUs fail to provide their VBIOS
                        // Dump it and inject it here.
                        // You must cut or pad it by 64 KB (GOP is not relevant).
                        "ATY,bin_image", 
                        Buffer (0x00010000)
                        {
                            // Put your VBIOS here (you could extract it with Linux or Windows)
                        },

                        // Works as AAPL%02X,override-no-connect, where %02X is display index e.g. 00
                        "AAPL00,override-no-connect",
                        Buffer (0x80)
                        {
                            // Put your EDID here (you could extract it with Linux or Windows)
                            // Note that in some cases the extracted EDID may not be compatible and 
                            // as a result you may see visual glitches.
                            // After you got at least something to show you could use edid-fix.sh script
                            // (based on pokenguyen's work) to generate a compatible EDID.
                        },

                        // Just like AAPL00, @0 represents your display index (from the connectors).
                        // From there on only @X notation could be used for displays other than 0.
                        // I.e. AAPL01,override-no-connect is OK but AAPL01,DualLink is NOT.

                        // Equal to AAPL00,DualLink, only supported for LVDS displays.
                        // Required bandwidth speed is calculated as follows:
                        // BITS_PER_PIXEL * WIDTH * HEIGHT * FREQUENCY / (1024 * 1024) = N Mbps/s
                        // It depends on hardware but you could commonly assume 800 Mbit/s per link
                        // If calculated N is above this value, you may require the property below.
                        "@0,display-dual-link",
                        Buffer ()
                        {
                             0x01, 0x00, 0x00, 0x00                         
                        },

                        // Equal to AAPL00,LinkFormat = 0 (0 - 6 bits, 1 - 8 bits)
                        // Most laptop displays do not support 24-bit (32) colour but only 18-bit.
                        // This property must be used if you see "gradient" glitches. 
                        "@0,display-link-component-bits",
                        Buffer () {
                            0x06, 0x00, 0x00, 0x00
                        },

                        // Equal to AAPL00,PixelFormat = 0 (0 - 6 bits, 1 - 8 bits)
                        // Similar to display-link-component-bits
                        "@0,display-pixel-component-bits",
                        Buffer ()
                        {
                            0x06, 0x00, 0x00, 0x00
                        },

                        // Equal to AAPL00,Dither
                        // In general you should avoid this property, since it most likely will decrease
                        // visual quality. However, on some displays you may get slightly better picture
                        // with this property set to 1.
                        "@0,display-dither-support",
                        Buffer ()
                        {
                            0x01, 0x00, 0x00, 0x00
                        }

                        */
                    }, Local0)
                    DTGP (Arg0, Arg1, Arg2, Arg3, RefOf (Local0))
                    Return (Local0)
                }
            }

            /*

            // Only use this if you are not satisfied with automatic HDMI injection
            Device (HDAU)
            {
                Name (_ADR, One)  // _ADR: Address
                Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
                {
                    If (LEqual (Arg2, Zero))
                    {
                        Return (Buffer (One)
                        {
                             0x03
                        })
                    }

                    Return (Package ()
                    {
                        // layout-id should be defined, though the value is unused

                        "layout-id",
                        Buffer (0x04)
                        {
                            0x01, 0x00, 0x00, 0x00
                        },

                        "hda-gfx",
                        Buffer ()
                        {
                            "onboard-1"
                        }
                    })
                }
            }

            */
        }

        // Below goes an example for IGPU injection useful for enabling hardware video decoding and other stuff.

        Device (IMEI)
        {
            Name (_ADR, 0x00160000)  // _ADR: Address
        }

        Scope (GFX0)
        {
            Name (_STA, Zero)  // _STA: Status
        }

        Device (IGPU)
        {
            Name (_ADR, 0x00020000)  // _ADR: Address
            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                Store (Package ()
                    {
                        "name",
                        Buffer ()
                        {
                            "display"
                        },

                        // This is a connector-less frame for Azul HD 4600

                        "AAPL,ig-platform-id",
                        Buffer (0x04)
                        {
                             0x04, 0x00, 0x12, 0x04
                        },

                        // This is an override for built-in device-id

                        "device-id",
                        Buffer (0x04)
                        {
                             0x12, 0x04, 0x00, 0x00
                        }
                    }, Local0)
                DTGP (Arg0, Arg1, Arg2, Arg3, RefOf (Local0))
                Return (Local0)
            }
        }

        Method (DTGP, 5, NotSerialized)
        {
            If (LEqual (Arg0, ToUUID ("a0b5b7c6-1318-441c-b0c9-fe695eaf949b")))
            {
                If (LEqual (Arg1, One))
                {
                    If (LEqual (Arg2, Zero))
                    {
                        Store (Buffer (One)
                            {
                                 0x03
                            }, Arg4)
                        Return (One)
                    }

                    If (LEqual (Arg2, One))
                    {
                        Return (One)
                    }
                }
            }

            Store (Buffer (One)
                {
                     0x00
                }, Arg4)
            Return (Zero)
        }
    }
}
