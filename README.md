# BullshitCore

![Code size](https://img.shields.io/github/languages/code-size/MinersStudios/BullshitCore?style=flat-square)

MinersStudios' own Minecraft server implementation.

## Building

Get a copy of the software:

```sh
git clone --depth 1 --single-branch https://github.com/MinersStudios/BullshitCore.git
```

Change the working directory:

```sh
cd BullshitCore
```

Run the build script:

```sh
./build.sh [additional build arguments]
```

The [GCC](https://gcc.gnu.org) is used by default, but the project is
compiler-agnostic and you can use any compiler you want by editing
[build.sh](build.sh) and using appropriate compilation/linking options for the
used compiler. To configure the project, use macros defined in the
[config.h](config.h) during the compilation or by modifying the file directly.

### Dependencies

- C standard library
- C POSIX library
- [wolfSSL](https://www.wolfssl.com)

### Windows support

Not officially supported, but you may use [MSYS2](https://www.msys2.org)
environment to build an executable yourself at your own risk.
