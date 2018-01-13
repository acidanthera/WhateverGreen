#!/bin/bash

BUILDDIR=$(dirname "$0")
pushd "$BUILDDIR" >/dev/null
BUILDDIR=$(pwd)
popd >/dev/null

rm -f "$BUILDDIR/rd_route.32.o" "$BUILDDIR/rd_route.64.o" \
      "$BUILDDIR/WhateverName.32.o" "$BUILDDIR/WhateverName.64.o" \
      "$BUILDDIR/libWhateverName.32.dylib" "$BUILDDIR/libWhateverName.64.dylib" \
      "$BUILDDIR/libWhateverName.dylib" "$BUILDDIR/WhateverName.app/Contents/MacOS/libWhateverName.dylib"

xcrun --sdk macosx cc -x c -c -std=c11 -arch x86_64 -mmacosx-version-min=10.10 -DNDEBUG=1 -flto -fvisibility=hidden $1 -O3 "$BUILDDIR/rd_route.c" -o "$BUILDDIR/rd_route.64.o" || exit 1
xcrun --sdk macosx cc -x objective-c++ -c -std=c++14 -arch x86_64 -mmacosx-version-min=10.10 -DNDEBUG=1 -flto -fvisibility=hidden $1 -O3 "$BUILDDIR/WhateverName.mm" -o "$BUILDDIR/WhateverName.64.o" || exit 1
xcrun --sdk macosx cc -arch x86_64 -mmacosx-version-min=10.10 -flto -fobjc-link-runtime -framework IOKit -lc++ $1 -O3 -dynamiclib "$BUILDDIR/rd_route.64.o" "$BUILDDIR/WhateverName.64.o" -o "$BUILDDIR/libWhateverName.64.dylib" || exit 1
strip -x "$BUILDDIR/libWhateverName.64.dylib" || exit 1

xcrun --sdk macosx cc -x c -c -std=c11 -arch i386 -mmacosx-version-min=10.10 -DNDEBUG=1 -flto -fvisibility=hidden $1 -O3 "$BUILDDIR/rd_route.c" -o "$BUILDDIR/rd_route.32.o" || exit 1
xcrun --sdk macosx cc -x objective-c++ -c -std=c++14 -arch i386 -mmacosx-version-min=10.10 -DNDEBUG=1 -flto -fvisibility=hidden $1 -O3 "$BUILDDIR/WhateverName.mm" -o "$BUILDDIR/WhateverName.32.o" || exit 1
xcrun --sdk macosx cc -arch i386 -mmacosx-version-min=10.10 -flto -fobjc-link-runtime -framework IOKit -lc++ $1 -O3 -dynamiclib "$BUILDDIR/rd_route.32.o" "$BUILDDIR/WhateverName.32.o" -o "$BUILDDIR/libWhateverName.32.dylib" || exit 1
strip -x "$BUILDDIR/libWhateverName.32.dylib" || exit 1

lipo -create "$BUILDDIR/libWhateverName.64.dylib" "$BUILDDIR/libWhateverName.32.dylib" -output "$BUILDDIR/libWhateverName.dylib" || exit 1

rm -f "$BUILDDIR/rd_route.32.o" "$BUILDDIR/rd_route.64.o" \
      "$BUILDDIR/WhateverName.32.o" "$BUILDDIR/WhateverName.64.o" \
      "$BUILDDIR/libWhateverName.32.dylib" "$BUILDDIR/libWhateverName.64.dylib"

mv "$BUILDDIR/libWhateverName.dylib" "$BUILDDIR/WhateverName.app/Contents/MacOS/libWhateverName.dylib"

exit 0
