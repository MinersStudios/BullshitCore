#!/bin/sh

(cd "$(dirname "$0")/source" || exit
. ../version || { printf "Version formatted wrong\n"; exit; }
output="../BullshitCore-$MINECRAFT_VERSION-$VERSION"
gcc -std=c99 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wwrite-strings \
-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs \
-Wmissing-include-dirs -Wjump-misses-init -Wlogical-op -I../include -I. \
-lwolfssl -O2 -flto -pthread -DNDEBUG network.c log.c world.c nbt.c ../main.c \
-o "$output" "$@" && strip "$output")
