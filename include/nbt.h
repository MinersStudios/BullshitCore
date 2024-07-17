#ifndef BULLSHITCORE_NBT
#define BULLSHITCORE_NBT

#include <stdint.h>

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
	int32_t length;
	int8_t *contents;
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
	int32_t length;
	int32_t *contents;
} TAG_Int_Array;
typedef struct
{
	int32_t length;
	int64_t *contents;
} TAG_Long_Array;
typedef struct NBT NBT;
struct NBT
{
	NBT *parent;
	NBT *children;
	uint8_t children_count;
	uint8_t tag_type;
	TAG_String name;
	uint8_t *payload;
};

#endif
