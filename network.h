#ifndef BULLSHITCORE_NETWORK
#define BULLSHITCORE_NETWORK

// This codebase assumes system's architecture to have: 8-bit byte width, two's
// complement representation, IEEE 754-2008 floats.
// TODO: Explicitly fail to compile or emulate this configuration

#include <stdint.h>

typedef _Bool Boolean;
#define true 1
#define false 0
typedef int8_t Byte;
typedef uint8_t UByte;
typedef int16_t Short;
typedef uint16_t UShort;
typedef int32_t Int;
typedef int64_t Long;
typedef float Float;
typedef double Double;
typedef int8_t *VarInt;
typedef int8_t *VarLong;
typedef struct
{
	VarInt size;
	uint8_t *contents;
} String;
enum TAGType
{
	TAGType_End,
	TAGType_Byte,
	TAGType_Short,
	TAGType_Int,
	TAGType_Long,
	TAGType_Float,
	TAGType_Double,
	TAGType_ByteArray,
	TAGType_String,
	TAGType_List,
	TAGType_Compound,
	TAGType_IntArray,
	TAGType_LongArray
};
typedef Byte TAG_Byte;
typedef Short TAG_Short;
typedef Int TAG_Int;
typedef Long TAG_Long;
typedef Float TAG_Float;
typedef Double TAG_Double;
typedef struct
{
	uint32_t length;
	Byte contents[];
} TAG_Byte_Array;
typedef struct
{
	uint16_t length;
	uint8_t *contents;
} TAG_String;
#define TAG_List(T, length) T[length]
#define TAG_Compound(T) T[]
typedef struct
{
	uint32_t length;
	Int contents[];
} TAG_Int_Array;
typedef struct
{
	uint32_t length;
	Long contents[];
} TAG_Long_Array;
typedef struct
{
	Byte tag_type;
	TAG_String name;
	void *payload;
} NBT;
typedef NBT TextComponent;
typedef String JSONTextComponent;
typedef String Identifier;
typedef struct
{
	UByte index;
	VarInt type;
	void *value;
} EntityMetadata[];
typedef struct
{
	Boolean present;
	VarInt item;
	Byte item_count;
	NBT NBT;
} Slot;
typedef int64_t Position;
#ifdef UINT8_MAX
typedef uint8_t Angle;
#else
typedef int8_t Angle;
#endif
typedef uint64_t UUID[2];
typedef struct
{
	VarInt length;
	Long data[];
} BitSet;
typedef Byte FixedBitSet[];
typedef struct
{
	VarInt length;
	VarInt packet_identifier;
} UncompressedPacketHeader;
typedef struct
{
	VarInt packet_length;
	VarInt data_length;
	VarInt packet_identifier;
} CompressedPacketHeader;
union PacketHeader
{
	UncompressedPacketHeader uncompressed;
	CompressedPacketHeader compressed;
};
enum State
{
	State_Status = 1,
	State_Login,
	State_Configuration,
	State_Play
};
typedef struct
{
	union PacketHeader header;
	VarInt protocol_version;
	String server_address;
	UShort server_port;
	VarInt next_state;
} PacketServerboundHandshake;
typedef struct
{
	union PacketHeader header;
	String JSON_response;
} PacketClientboundStatusResponse;
typedef struct
{
	union PacketHeader header;
	Long payload;
} PacketClientboundStatusPingResponse;
typedef struct
{
	union PacketHeader header;
} PacketServerboundStatusRequest;
typedef struct
{
	union PacketHeader header;
	Long payload;
} PacketServerboundStatusPingRequest;
typedef struct
{
	union PacketHeader header;
	JSONTextComponent reason;
} PacketClientboundLoginDisconnect;
typedef struct
{
	union PacketHeader header;
	String server_identifier;
	VarInt public_key_length;
	Byte *public_key;
	VarInt verify_token_length;
	Byte *verify_token;
} PacketClientboundLoginEncryptionRequest;
typedef struct
{
	union PacketHeader header;
	UUID UUID;
	String username;
	VarInt properties_count;
	String property_name;
	String property_value;
	Boolean sign;
	String signature;
} PacketClientboundLoginSuccess;
typedef struct
{
	union PacketHeader header;
	VarInt threshold;
} PacketClientboundLoginSetCompression;
typedef struct
{
	union PacketHeader header;
	VarInt message_identifier;
	Identifier channel;
	Byte *data;
} PacketClientboundLoginPluginRequest;
typedef struct
{
	union PacketHeader header;
	String player_username;
	UUID player_UUID;
} PacketServerboundLoginStart;
typedef struct
{
	union PacketHeader header;
	VarInt shared_secret_length;
	Byte *shared_secret;
	VarInt verify_token_length;
	Byte *verify_token;
} PacketServerboundLoginEncryptionResponse;
typedef struct
{
	union PacketHeader header;
	VarInt message_identifier;
	Boolean successful;
	Byte *data;
} PacketServerboundLoginPluginResponse;
typedef struct
{
	union PacketHeader header;
} PacketServerboundLoginAcknowledged;

#endif
