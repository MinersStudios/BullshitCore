#!/bin/sh

(cd "$(dirname "$0")/source"
gcc -std=c99 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wwrite-strings \
-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs \
-Wmissing-include-dirs -Wjump-misses-init -Wlogical-op -I../include -I. -O2 \
-flto -pthread -DNDEBUG network.c log.c ../main.c -o ../BullshitCore $@)
