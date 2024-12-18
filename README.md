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

Note: [GCC](https://gcc.gnu.org) is used by default, but you may use any
compiler you want by editing [build.sh](build.sh) directly.

### Dependencies

- C standard library
- C POSIX library
- [wolfSSL](https://www.wolfssl.com)

### Windows support

Not officially supported, but you may use [MSYS2](https://www.msys2.org)
environment to build an executable yourself at your own risk.

## Usage

1. Open a newly generated executable binary file called
`BullshitCore-{Minecraft release}-{BullshitCore release}`.
2. Open a port set in settings or, if none is set, the default one, 25565, for
TCP connections.
3. Check if the server is running and it is possible to connect by using a
Minecraft client with the same version as in the release of BullshitCore you're using or
through other means.

Note: Even if the Minecraft release in the name says otherwise, you should try if it works with a different version, as not 100% of functionality is implemented and some changes may not have any effect on compatibility, but Minecraft requires servers to send the vanilla datapack, which is more likely to be outdated or too new, but you can try patching BullshitCore for this or leave an [issue](https://github.com/MinersStudios/BullshitCore/issues).

## Further reading

https://github.com/MinersStudios/BullshitCore/wiki
