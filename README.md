# BullshitCore

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

### Dependencies

- C standard library
- C POSIX library
- [Berkeley DB](https://www.oracle.com/database/technologies/related/berkeleydb.html)
- [wolfSSL](https://www.wolfssl.com)
- [zlib](https://www.zlib.net)

### Windows support

Not officially supported, but you may use [MSYS2](https://www.msys2.org)
environment to build an executable yourself at your own risk.
