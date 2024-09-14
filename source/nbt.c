#define _XOPEN_SOURCE 500
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "global_macros.h"
#include "memory.h"
#include "nbt.h"

#define CASE_GENERIC(identifier, T) \
	case identifier: \
	{ \
		root_element = (TAG_Compound *)(((T *)((TAG_List *)root_element)->contents) + atoi(token) - 1); \
		break; \
	}

enum ParserContext
{
	ParserContext_Tag_Type,
	ParserContext_Byte_Name_Length_1,
	ParserContext_Byte_Name_Length_2,
	ParserContext_Byte_Name_Character,
	ParserContext_Byte_Payload,
	ParserContext_Short_Name_Length_1,
	ParserContext_Short_Name_Length_2,
	ParserContext_Short_Name_Character,
	ParserContext_Short_Payload_1,
	ParserContext_Short_Payload_2,
	ParserContext_Int_Name_Length_1,
	ParserContext_Int_Name_Length_2,
	ParserContext_Int_Name_Character,
	ParserContext_Int_Payload_1,
	ParserContext_Int_Payload_2,
	ParserContext_Int_Payload_3,
	ParserContext_Int_Payload_4,
	ParserContext_Long_Name_Length_1,
	ParserContext_Long_Name_Length_2,
	ParserContext_Long_Name_Character,
	ParserContext_Long_Payload_1,
	ParserContext_Long_Payload_2,
	ParserContext_Long_Payload_3,
	ParserContext_Long_Payload_4,
	ParserContext_Long_Payload_5,
	ParserContext_Long_Payload_6,
	ParserContext_Long_Payload_7,
	ParserContext_Long_Payload_8,
	ParserContext_Float_Name_Length_1,
	ParserContext_Float_Name_Length_2,
	ParserContext_Float_Name_Character,
	ParserContext_Float_Payload_1,
	ParserContext_Float_Payload_2,
	ParserContext_Float_Payload_3,
	ParserContext_Float_Payload_4,
	ParserContext_Double_Name_Length_1,
	ParserContext_Double_Name_Length_2,
	ParserContext_Double_Name_Character,
	ParserContext_Double_Payload_1,
	ParserContext_Double_Payload_2,
	ParserContext_Double_Payload_3,
	ParserContext_Double_Payload_4,
	ParserContext_Double_Payload_5,
	ParserContext_Double_Payload_6,
	ParserContext_Double_Payload_7,
	ParserContext_Double_Payload_8,
	ParserContext_Byte_Array_Name_Length_1,
	ParserContext_Byte_Array_Name_Length_2,
	ParserContext_Byte_Array_Name_Character,
	ParserContext_Byte_Array_Length_1,
	ParserContext_Byte_Array_Length_2,
	ParserContext_Byte_Array_Payload,
	ParserContext_String_Name_Length_1,
	ParserContext_String_Name_Length_2,
	ParserContext_String_Name_Character,
	ParserContext_String_Length_1,
	ParserContext_String_Length_2,
	ParserContext_String_Payload,
	ParserContext_List_Type,
	ParserContext_List_Length_1,
	ParserContext_List_Length_2,
	ParserContext_List_Length_3,
	ParserContext_List_Length_4,
	ParserContext_List_Payload,
	ParserContext_Compound_Name_Length_1,
	ParserContext_Compound_Name_Length_2,
	ParserContext_Compound_Name_Character,
	ParserContext_Compound_Payload,
	ParserContext_Int_Array_Name_Length_1,
	ParserContext_Int_Array_Name_Length_2,
	ParserContext_Int_Array_Name_Character,
	ParserContext_Int_Array_Payload_1,
	ParserContext_Int_Array_Payload_2,
	ParserContext_Int_Array_Payload_3,
	ParserContext_Int_Array_Payload_4,
	ParserContext_Long_Array_Name_Length_1,
	ParserContext_Long_Array_Name_Length_2,
	ParserContext_Long_Array_Name_Character,
	ParserContext_Long_Array_Payload_1,
	ParserContext_Long_Array_Payload_2,
	ParserContext_Long_Array_Payload_3,
	ParserContext_Long_Array_Payload_4,
	ParserContext_Long_Array_Payload_5,
	ParserContext_Long_Array_Payload_6,
	ParserContext_Long_Array_Payload_7,
	ParserContext_Long_Array_Payload_8
};

NBT *
bullshitcore_nbt_read(FILE * restrict file)
{
	NBT *nbt = NULL;
	enum ParserContext parser_context = ParserContext_Tag_Type;
	int byte;
	uint16_t counter;
	uint64_t compound_count = 0;
	while ((byte = getc(file)) != EOF)
	{
		switch (parser_context)
		{
			case ParserContext_Tag_Type:
			{
				switch (byte)
				{
					case TAGType_End:
					{
						if (!compound_count)
						{
							bullshitcore_memory_leave(nbt, sizeof *nbt);
							return NULL;
						}
						if (!--compound_count) return nbt;
						break;
					}
					case TAGType_Byte:
					{
						break;
					}
					case TAGType_Short:
					{
						break;
					}
					case TAGType_Int:
					{
						break;
					}
					case TAGType_Long:
					{
						break;
					}
					case TAGType_Float:
					{
						break;
					}
					case TAGType_Double:
					{
						break;
					}
					case TAGType_Byte_Array:
					{
						nbt = bullshitcore_memory_retrieve(sizeof *nbt);
						if (unlikely(!nbt)) return NULL;
						nbt->type_identifier = TAGType_Byte_Array;
						parser_context = ParserContext_Byte_Array_Name_Length_1;
						break;
					}
					case TAGType_String:
					{
						nbt = bullshitcore_memory_retrieve(sizeof *nbt);
						if (unlikely(!nbt)) return NULL;
						nbt->type_identifier = TAGType_String;
						parser_context = ParserContext_String_Name_Length_1;
						break;
					}
					case TAGType_List:
					{
						break;
					}
					case TAGType_Compound:
					{
						parser_context = ParserContext_Compound_Name_Length_1;
						++compound_count;
						break;
					}
					case TAGType_Int_Array:
					{
						break;
					}
					case TAGType_Long_Array:
					{
						break;
					}
				}
				break;
			}
			case ParserContext_Byte_Array_Name_Length_1:
			{
				nbt->tag_name.length = byte << 8;
				parser_context = ParserContext_Byte_Array_Name_Length_2;
				break;
			}
			case ParserContext_Byte_Array_Name_Length_2:
			{
				nbt->tag_name.length |= byte & 0xFF;
				nbt->tag_name.contents = bullshitcore_memory_retrieve(sizeof *nbt->tag_name.contents * nbt->tag_name.length);
				if (unlikely(!nbt->tag_name.contents)) return NULL;
				counter = 0;
				parser_context = ParserContext_Byte_Array_Name_Character;
				break;
			}
			case ParserContext_Byte_Array_Name_Character:
			{
				if (counter < nbt->tag_name.length)
				{
					nbt->tag_name.contents[counter] = byte;
					++counter;
					break;
				}
				nbt->contents = bullshitcore_memory_retrieve(sizeof(TAG_Byte_Array));
				if (unlikely(!nbt->contents)) return NULL;
			}
			case ParserContext_Byte_Array_Length_1:
			{
				((TAG_Byte_Array *)nbt->contents)->length = byte << 8;
				parser_context = ParserContext_Byte_Array_Length_2;
				break;
			}
			case ParserContext_Byte_Array_Length_2:
			{
				((TAG_Byte_Array *)nbt->contents)->length |= byte & 0xFF;
				((TAG_Byte_Array *)nbt->contents)->contents = bullshitcore_memory_retrieve(sizeof(TAG_Byte) * ((TAG_Byte_Array *)nbt->contents)->length);
				if (unlikely(!((TAG_Byte_Array *)nbt->contents)->contents)) return NULL;
				counter = 0;
				parser_context = ParserContext_Byte_Array_Payload;
				break;
			}
			case ParserContext_Byte_Array_Payload:
			{
				if (counter < ((TAG_Byte_Array *)nbt->contents)->length)
				{
					((TAG_Byte_Array *)nbt->contents)->contents[counter] = byte;
					++counter;
				}
				else parser_context = ParserContext_Tag_Type;
				continue;
			}
			case ParserContext_String_Name_Length_1:
			{
				nbt->tag_name.length = byte << 8;
				parser_context = ParserContext_String_Name_Length_2;
				break;
			}
			case ParserContext_String_Name_Length_2:
			{
				nbt->tag_name.length |= byte & 0xFF;
				nbt->tag_name.contents = bullshitcore_memory_retrieve(sizeof *nbt->tag_name.contents * nbt->tag_name.length);
				if (unlikely(!nbt->tag_name.contents)) return NULL;
				counter = 0;
				parser_context = ParserContext_String_Name_Character;
				break;
			}
			case ParserContext_String_Name_Character:
			{
				if (counter < nbt->tag_name.length)
				{
					nbt->tag_name.contents[counter] = byte;
					++counter;
					break;
				}
				nbt->contents = bullshitcore_memory_retrieve(sizeof(TAG_String));
				if (unlikely(!nbt->contents)) return NULL;
			}
			case ParserContext_String_Length_1:
			{
				((TAG_String *)nbt->contents)->length = byte << 8;
				parser_context = ParserContext_String_Length_2;
				break;
			}
			case ParserContext_String_Length_2:
			{
				((TAG_String *)nbt->contents)->length |= byte & 0xFF;
				((TAG_String *)nbt->contents)->contents = bullshitcore_memory_retrieve(sizeof(uint8_t) * ((TAG_String *)nbt->contents)->length);
				if (unlikely(!((TAG_String *)nbt->contents)->contents)) return NULL;
				counter = 0;
				parser_context = ParserContext_String_Payload;
				break;
			}
			case ParserContext_String_Payload:
			{
				if (counter < ((TAG_String *)nbt->contents)->length)
				{
					((TAG_String *)nbt->contents)->contents[counter] = byte;
					++counter;
				}
				else parser_context = ParserContext_Tag_Type;
				continue;
			}
			case ParserContext_Compound_Name_Length_1:
			{
				counter = byte << 8;
				parser_context = ParserContext_Compound_Name_Length_2;
				break;
			}
			case ParserContext_Compound_Name_Length_2:
			{
				counter |= byte & 0xFF;
				parser_context = ParserContext_Compound_Name_Character;
				break;
			}
			case ParserContext_Compound_Name_Character:
			{
				if (counter > 1) --counter;
				else parser_context = ParserContext_Tag_Type;
				break;
			}
		}
	}
	if (unlikely(ferror(file)))
	{
		bullshitcore_memory_leave(nbt, sizeof *nbt);
		return NULL;
	}
	return nbt;
}

void
bullshitcore_nbt_free(NBT * restrict nbt)
{
	bullshitcore_memory_leave(nbt, sizeof *nbt);
}

void *
bullshitcore_nbt_search(const NBT * restrict nbt, const uint8_t * restrict query)
{
	TAG_Compound *root_element = nbt;
	char *query_copy = strdup((const char *)query);
	if (unlikely(!query_copy)) return NULL;
	for (char *token = strtok(query_copy, ">"); token != NULL; token = strtok(NULL, ">"))
	{
		if (isdigit(*token))
		{
			switch (((TAG_List *)root_element)->type_identifier)
			{
				CASE_GENERIC(TAGType_Byte, TAG_Byte)
				CASE_GENERIC(TAGType_Short, TAG_Short)
				CASE_GENERIC(TAGType_Int, TAG_Int)
				CASE_GENERIC(TAGType_Long, TAG_Long)
				CASE_GENERIC(TAGType_Float, TAG_Float)
				CASE_GENERIC(TAGType_Double, TAG_Double)
				CASE_GENERIC(TAGType_Byte_Array, TAG_Byte_Array)
				CASE_GENERIC(TAGType_String, TAG_String)
				CASE_GENERIC(TAGType_List, TAG_List)
				case TAGType_Compound:
				{
					root_element = *(((TAG_Compound **)((TAG_List *)root_element)->contents) + atoi(token) - 1);
					break;
				}
				CASE_GENERIC(TAGType_Int_Array, TAG_Int_Array)
				CASE_GENERIC(TAGType_Long_Array, TAG_Long_Array)
			}
			continue;
		}
		else if (!strcmp((const char *)root_element->tag_name.contents, token))
		{
			root_element = root_element->contents;
			continue;
		}
		else ++root_element;
	}
	bullshitcore_memory_leave(query_copy, strlen((const char *)query) + 1);
	return root_element;
}
