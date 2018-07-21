#!/bin/sh

cd "$(dirname "$0")"
clang -s -flto -mmacosx-version-min=10.9 -Os BoardHash.c -o BoardHash
