#ifndef BULLSHITCORE_NETWORK
#define BULLSHITCORE_NETWORK

#include <stdint.h>

typedef _Bool Boolean;
#define true 1
#define false 0
typedef uint8_t *VarInt;
typedef uint8_t *VarLong;
typedef struct
{
	VarInt length;
	const char *contents;
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
typedef int8_t TAG_Byte;
typedef int16_t TAG_Short;
typedef int32_t TAG_Int;
typedef int64_t TAG_Long;
typedef float TAG_Float;
typedef double TAG_Double;
typedef struct
{
	uint32_t length;
	int8_t contents[];
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
	uint32_t contents[];
} TAG_Int_Array;
typedef struct
{
	uint32_t length;
	uint64_t contents[];
} TAG_Long_Array;
typedef struct
{
	uint8_t tag_type;
	TAG_String name;
	void *payload;
} NBT;
typedef NBT TextComponent;
typedef String JSONTextComponent;
#define JSONTEXTCOMPONENT_MAXSIZE 262144
typedef String Identifier;
#define IDENTIFIER_MAXSIZE 32767
typedef struct
{
	uint8_t index;
	VarInt type;
	void *value;
} EntityMetadata[];
typedef struct
{
	Boolean present;
	VarInt item;
	uint8_t item_count;
	NBT NBT;
} Slot;
typedef uint64_t Position;
typedef uint8_t Angle;
typedef uint64_t UUID[2];
typedef struct
{
	VarInt length;
	uint64_t data[];
} BitSet;
typedef uint8_t FixedBitSet[];
typedef struct
{
	VarInt length;
	VarInt packet_identifier;
} PacketHeader;
typedef struct
{
	VarInt packet_length;
	VarInt data_length;
	VarInt packet_identifier;
} CompressedPacketHeader;
enum State
{
	State_Handshaking,
	State_Status,
	State_Login,
	State_Transfer,
	State_Configuration,
	State_Play
};
enum HandshakePacket
{
	HANDSHAKE_PACKET
};
enum StatusPacket
{
	STATUSREQUEST_PACKET,
	PINGREQUEST_PACKET
};

VarInt bullshitcore_network_varint_encode(int32_t value);
int32_t bullshitcore_network_varint_decode(VarInt restrict varint, size_t * restrict bytes);
VarLong bullshitcore_network_varlong_encode(int64_t value);
int64_t bullshitcore_network_varlong_decode(VarLong restrict varlong, size_t * restrict bytes);

#endif
