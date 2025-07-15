#!/bin/bash

#  generate.sh
#  WhateverGreen
#
#  Copyright © 2018 vit9696. All rights reserved.

ret=0

rm -f "${PROJECT_DIR}/WhateverGreen/kern_resources.cpp"

# xcode only signs the binaries after all build phases so we can't use the binary we built on this phase on Apple Silicon 
/usr/bin/codesign --force --sign - --timestamp\=none "${TARGET_BUILD_DIR}/ResourceConverter"

"${TARGET_BUILD_DIR}/ResourceConverter" \
	"${PROJECT_DIR}/Resources" \
	"${PROJECT_DIR}/WhateverGreen/kern_resources.cpp" \
	"${PROJECT_DIR}/WhateverGreen/kern_resources.hpp" || ret=1

if (( $ret )); then
	echo "Failed to build kern_resources.cpp"
	exit 1
fi
