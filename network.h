#ifndef BULLSHITCORE_NETWORK
#define BULLSHITCORE_NETWORK

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
int32_t bullshitcore_network_varint_decode(VarInt varint, size_t * restrict bytes);
VarLong bullshitcore_network_varlong_encode(int64_t value);
int64_t bullshitcore_network_varlong_decode(VarLong varlong, size_t * restrict bytes);

#endif
