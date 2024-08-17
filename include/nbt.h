#ifndef BULLSHITCORE_NBT
#define BULLSHITCORE_NBT

#include <stdint.h>
#include <stdio.h>

enum TAGType
{
	TAGType_End,
	TAGType_Byte,
	TAGType_Short,
	TAGType_Int,
	TAGType_Long,
	TAGType_Float,
	TAGType_Double,
	TAGType_Byte_Array,
	TAGType_String,
	TAGType_List,
	TAGType_Compound,
	TAGType_Int_Array,
	TAGType_Long_Array
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
	TAG_Byte *contents;
} TAG_Byte_Array;
typedef struct
{
	uint16_t length;
	uint8_t *contents;
} TAG_String;
typedef struct
{
	int8_t type_identifier;
	int32_t length;
	void *contents;
} TAG_List;
typedef struct
{
	int8_t type_identifier;
	TAG_String tag_name;
	void *contents;
} TAG_Compound;
typedef struct
{
	int32_t length;
	TAG_Int *contents;
} TAG_Int_Array;
typedef struct
{
	int32_t length;
	TAG_Long *contents;
} TAG_Long_Array;
typedef TAG_Compound NBT;

NBT *bullshitcore_nbt_read(FILE * restrict file);
void bullshitcore_nbt_free(NBT * restrict nbt);
void *bullshitcore_nbt_search(const NBT * restrict nbt, const uint8_t * restrict query);

#endif
