#!/bin/bash
# This code is based off HP ProBook 4x30s Fix EDID by pokenguyen

GenEDID() {
    /usr/libexec/PlistBuddy -c "Print :$1" /tmp/display.plist &>/dev/null || return 0

    rm -f /tmp/EDID.bin

    EDID=$(/usr/libexec/PlistBuddy -x -c "Print :$1" /tmp/display.plist |
        tr -d '\n\t' | grep -o 'IODisplayEDID</key><data>[^<]*' |
        sed 's/.*IODisplayEDID<\/key><data>//' | base64 -D | xxd -p | tr -d '\n\t\s')
    version=${EDID:38:2}
    basicparams=${EDID:40:2}
    checksum=${EDID:254:2}

    newchecksum=$(printf '%x' $((0x$checksum + 0x$version + 0x$basicparams - 0x04 - 0x90)) | tail -c 2)
    newedid=${EDID:0:38}0490${EDID:42:212}${newchecksum}

    echo $newedid | xxd -r -p >>/tmp/EDID.bin

    if [ $? -eq 0 ]; then
        RegName=$(/usr/libexec/PlistBuddy -c "Print :$1:IORegistryEntryName" /tmp/display.plist)
        DisplayFlags=$(/usr/libexec/PlistBuddy -c "Print :$1:IODisplayConnectFlags" /tmp/display.plist)
        VenID=$(/usr/libexec/PlistBuddy -c "Print :$1:DisplayVendorID" /tmp/display.plist)
        VenIDhex=$(printf '%x\n' $VenID)
        ProdID=$(/usr/libexec/PlistBuddy -c "Print :$1:DisplayProductID" /tmp/display.plist)
        ProdIDhex=$(printf '%x\n' $ProdID)
        GenPlist=~/"Desktop/${VenIDhex}_${ProdIDhex}.plist"
        GenBin=~/"Desktop/${VenIDhex}_${ProdIDhex}.bin"

        rm -f "$GenPlist" "$GenBin"

        # This check does not really detect external displays
        if [ "$RegName" == "AppleBacklightDisplay" -o "$DisplayFlags" == "$(echo AAgAAA== | base64 -D)" ] ||
            [ "$RegName" == "AppleDisplay" -o "$DisplayFlags" == "$(echo AAgAAA== | base64 -D)" ]; then

            /usr/libexec/PlistBuddy -c "Add :DisplayProductName string 'Display'" "$GenPlist"
            /usr/libexec/PlistBuddy -c "Add :test array" "$GenPlist"
            /usr/libexec/PlistBuddy -c "Merge /tmp/display.plist :test" "$GenPlist"
            /usr/libexec/PlistBuddy -c "Copy :test:$1:DisplayProductID :DisplayProductID" "$GenPlist"
            /usr/libexec/PlistBuddy -c "Copy :test:$1:DisplayVendorID :DisplayVendorID" "$GenPlist"

            /usr/libexec/PlistBuddy -c "Remove :test" "$GenPlist"
            /usr/libexec/PlistBuddy -c "Import :IODisplayEDID /tmp/EDID.bin" "$GenPlist"

            mv /tmp/EDID.bin "$GenBin"

            echo "Display $1"
            echo "  vendor id  ${VenIDhex}"
            echo "  product id ${ProdIDhex}"
            echo "EDID:"
            xxd -i "$GenBin" | grep -vE 'unsigned|}'

            echo "If you cannot inject this EDID via SSDT (AAPL00,override-no-connect), save $GenPlist as:"
            echo "/System/Library/Displays/Contents/Resources/Overrides/DisplayVendorID-${VenIDhex}/DisplayProductID-${ProdIDhex}.plist"
        else
            echo "External display detected (${VenIDhex}_${ProdIDhex}}!"
        fi
    else
        echo "No display detected!"
    fi
}

rm -f /tmp/display.plist

case $(ioreg -n AppleBacklightDisplay -rxw0) in
"")
    ioreg -n AppleDisplay -arxw0 >/tmp/display.plist
    ;;
*)
    ioreg -n AppleBacklightDisplay -arxw0 >/tmp/display.plist
    ;;
esac

GenEDID 0 && GenEDID 1 && GenEDID 2
rm -f /tmp/display.plist /tmp/EDID.bin
