#!/bin/sh

(set -u
cd "$(dirname "$0")/source" || exit 1
. ../version || { printf "Version formatted wrong\n" >&2; exit 1; }
app="../BullshitCore-$MINECRAFT_VERSION-$VERSION" ||
{ printf "Version formatted wrong\n" >&2; exit 1; }
lib="../libbullshitcore-$MINECRAFT_VERSION-$VERSION"
set +u
case $1 in
	debug)
		shift
		gcc -c -std=c99 -fPIC -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow \
		-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
		-Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
		-Wjump-misses-init -Wlogical-op -I../include -I.. -g -pthread ./*.c \
		"$@"
		gcc -std=c99 -fPIC -shared -g -pthread ./*.o -o "$lib-debug.so" "$@" \
		-lwolfssl
		gcc -std=c99 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow \
		-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
		-Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
		-Wjump-misses-init -Wlogical-op -I../include -I. -g -pthread ./*.o \
		../main.c -o "$app-debug" "$@" -lwolfssl;;
	*)
		gcc -c -std=c99 -fPIC -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow \
		-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
		-Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
		-Wjump-misses-init -Wlogical-op -I../include -I.. -O2 -flto \
		-fno-fat-lto-objects -pthread -DNDEBUG ./*.c "$@"
		gcc -std=c99 -fPIC -shared -O2 -flto -flto-partition=one -pthread \
		./*.o -o "$lib.so" "$@" -lwolfssl
		gcc -std=c99 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow \
		-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
		-Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
		-Wjump-misses-init -Wlogical-op -I../include -I. -O2 -flto \
		-flto-partition=one -fno-fat-lto-objects -fwhole-program -pthread \
		-DNDEBUG ./*.o ../main.c -o "$app" "$@" -lwolfssl && strip "$app";;
esac)
