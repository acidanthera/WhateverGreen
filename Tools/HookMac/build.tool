#!/bin/sh

cd "$(dirname "$0")"
clang -s -flto -fvisibility=hidden -mmacosx-version-min=10.9 -dynamiclib -Os -framework IOKit -framework CoreFoundation HookMac.c rd_route.c -o libHookMac.dylib