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
enum TAG_Type
{
	TAG_Type_End,
	TAG_Type_Byte,
	TAG_Type_Short,
	TAG_Type_Int,
	TAG_Type_Long,
	TAG_Type_Float,
	TAG_Type_Double,
	TAG_Type_Byte_Array,
	TAG_Type_String,
	TAG_Type_List,
	TAG_Type_Compound,
	TAG_Type_Int_Array,
	TAG_Type_Long_Array
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
	Byte data[];
} Packet;
typedef struct
{
	VarInt packet_length;
	VarInt data_length;
	VarInt packet_identifier;
	Byte data[];
} CompressedPacket;
enum State
{
	State_Status = 1,
	State_Login,
	State_Configuration,
	State_Play
};
typedef struct
{
	VarInt protocol_version;
	String server_address;
	UShort server_port;
	VarInt next_state;
} HandshakePacketData;

#endif
