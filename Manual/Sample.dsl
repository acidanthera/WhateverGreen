DefinitionBlock ("", "SSDT", 2, "APPLE ", "SSDTAMDGPU", 0x00001000)
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

                            // Make sure not to write the same onboard index for each installed GPU

                            "hda-gfx",
                            Buffer ()
                            {
                                "onboard-1"
                            },

                            // Only use this if automatic detection fails or may not be used

                            "model",
                            Buffer ()
                            {
                                "AMD Radeon HD 6450"
                            },

                            // Monitor at index 0 is used as main, the value does not matter

                            "@0,AAPL,boot-display",
                            Buffer (Zero) {}

                            // You could also put other properties like "connectors", "connector-priority",
                            // or "device-id" here in case this is required for your setup.
                            // If you add or remove connectors do not forget to specify "connector-count".
                        }, Local0)
                    DTGP (Arg0, Arg1, Arg2, Arg3, RefOf (Local0))
                    Return (Local0)
                }
            }

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

                        // You may need to put "device-id" here in case this is required for your setup.
                        // Please be aware that that AppleHDAController reads the device-id directly.
                        // This means that you may have to patch your device-id in AppleHDAController.
                        // Assume your original device-id is <c8 aa 00 00> (R9 290X).
                        // And the id you are faking to is <e0 aa 00 00> (Pro 460).
                        // So here you add device-id <e0 aa 00 00>.
                        // And AppleHDAController patch is <02 10 e0 aa> -> <02 10 c8 aa>.
                        // Thanks to Pawel69 for this.
                    })
                }
            }
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
