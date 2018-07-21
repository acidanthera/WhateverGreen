#!/bin/bash

cd "$(dirname "$0")"
rm -rf gpuinfo32 gpuinfo64 gpuinfo *.dSYM

if [ "$DEBUG" != "" ]; then
  clang -Wall -Wextra -pedantic -Wl,-framework,CoreFoundation -Wl,-framework,IOKit -lobjc -m32 -O0 -mmacosx-version-min=10.4 gpuinfo.c -o gpuinfo32 || exit 1
  clang -Wall -Wextra -pedantic -Wl,-framework,CoreFoundation -Wl,-framework,IOKit -lobjc -m64 -O0 -mmacosx-version-min=10.4 gpuinfo.c -o gpuinfo64 || exit 1
else
  clang -Wall -Wextra -pedantic -Wl,-framework,CoreFoundation -Wl,-framework,IOKit -lobjc -m32 -flto -O3 -mmacosx-version-min=10.4 gpuinfo.c -o gpuinfo32 || exit 1
  clang -Wall -Wextra -pedantic -Wl,-framework,CoreFoundation -Wl,-framework,IOKit -lobjc -m64 -flto -O0 -mmacosx-version-min=10.4 gpuinfo.c -o gpuinfo64 || exit 1
fi

strip -x gpuinfo32 || exit 1
strip -x gpuinfo64 || exit 1

lipo -create gpuinfo32 gpuinfo64 -output gpuinfo || exit 1

rm -f gpuinfo32 gpuinfo64

exit 0
