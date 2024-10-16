// An IPv4/6 address to listen on (only dotted-decimal format for IPv4 and no
// hostnames).
#define ADDRESS "0.0.0.0"
// An integer signifying the size in bytes for the chunk data buffer. Has the
// following suffixes: c = 1, w = 2, b = 512, kB = 1000, K = 1024, MB = 1000 *
// 1000, M = 1024 * 1024, xM = M, GB = 1000 * 1000 * 1000, G = 1024 * 1024 *
// 1024, and so on for T, P, E, Z, Y, R, Q. Binary suffixes can be used, too:
// KiB = K, MiB = M, and so on. For optimal performance, choose a multiple of
// the physical block size of the storage device where the world data is
// stored.
#define CHUNK_READ_BUFFER_SIZE "512"
// A description of the server to show on the servers list screen.
#define DESCRIPTION "A Minecraft Server"
// A base64 data URI for a PNG image (alpha channel is supported) with
// dimensions of 64 x 64 pixels to display as the icon of the server.
#define FAVICON ""
// A toggle for the IPv6 support (ADDRESS shall be IPv6).
//#define IPV6
// An integer in the range [-2147483648, 2147483647] signifying the maximum
// number of players allowed on the server. Due to limitations associated with
// the current configuration parsing, adding the L suffix to signify a long
// integer will cause the ping to fail, making the effective range be at least
// [-32768, 32767]. Currently only affects the number on the servers list
// screen.
#define MAX_PLAYERS 20
// An integer in the range [0, 65535] signifying the port to listen on.
#define PORT 25565
// An integer in the range [2, 32] signifying the radius in chunks (16 by 16
// blocks areas) of the world data to be sent to the client.
#define RENDER_DISTANCE 2
// A brand of the server to show in the crash logs and on the debug screen.
#define SERVER_BRAND "BullshitCore"
// An integer in the range [5, 32] signifying the radius in chunks (16 by 16
// blocks areas) for entities to be simulated.
#define SIMULATION_DISTANCE 5
