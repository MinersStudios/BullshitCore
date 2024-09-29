#!/bin/sh

(set -u
cd "$(dirname "$0")/source" || exit 1
. ../version || { printf "Version formatted wrong\n" >&2; exit 1; }
output="../BullshitCore-$MINECRAFT_VERSION-$VERSION" || { printf "Version formatted wrong\n" >&2; exit 1; }
gcc -std=c99 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wwrite-strings \
-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs \
-Wmissing-include-dirs -Wjump-misses-init -Wlogical-op -I../include -I. -O2 \
-flto -flto-partition=one -fwhole-program -pthread -DNDEBUG ./*.c ../main.c \
-o "$output" "$@" -lwolfssl && strip "$output")
