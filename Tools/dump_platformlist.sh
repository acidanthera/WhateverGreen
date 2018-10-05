#!/bin/bash
#set -x

function capture_ioreg_data
# $1 is node name to search/capture
# $2 is file name to save XML to
{
    ioreg -n $1 -arxw0 >$2
}

function extract_bin_property
# $1 is plist file name
# $2 is path within
# $3 is file name for binary output
{
    local data=$(/usr/libexec/PlistBuddy -x -c "Print :$2" $1 2>&1)
    data=$([[ "$data" =~ \<data\>(.*)\<\/data\> ]] && echo ${BASH_REMATCH[1]})
    echo "$data" | base64 --decode >$3
}

plist=/tmp/org.rehabman.platformlist.plist
capture_ioreg_data WhateverGreen $plist
if [[ ! -s /tmp/org.rehabman.platformlist.plist ]]; then
    echo "Error capturing WhateverGreen registry data (WhateverGreen not installed?)"
    exit
fi

for x in native patched; do
    extract_bin_property $plist :0:platform-table-$x $x.bin
    xxd -groupsize 4 <$x.bin >$x.txt
done

#EOF
