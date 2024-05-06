#!/bin/sh

script_dir="$(dirname "$0")"
gcc -std=c99 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wwrite-strings \
-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs \
-Wmissing-include-dirs -Wjump-misses-init -Wlogical-op -O2 -pthread \
"$script_dir"/network.c "$script_dir"/log.c "$script_dir"/main.c -o main
