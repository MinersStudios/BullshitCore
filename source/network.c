#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <string.h>
#include "global-macros.h"
#include "memory-pool.h"
#include "network.h"

VarInt *
bullshitcore_network_varint_encode(int32_t value)
{
	uint_fast8_t bytes = 1;
	while ((uint32_t)value >> 7 * bytes && bytes < 5) ++bytes;
	VarInt *varint = bullshitcore_memory_pool_retrieve(bytes);
	if (unlikely(!varint)) return NULL;
	memset(varint, 0, bytes);
	for (size_t i = 0; i < bytes; ++i)
		varint[i] = (uint32_t)value >> 7 * i & 0x7F | (i != bytes - 1U) << 7;
	return varint;
}

int32_t
bullshitcore_network_varint_decode(const VarInt * restrict varint, uint8_t * restrict bytes)
{
	int32_t value = 0;
	size_t i = 0;
	int8_t varint_byte;
	for (;i < 5; ++i)
	{
		varint_byte = varint[i];
		value |= (varint_byte & 0x7F) << 7 * i;
		if (!(varint_byte & 0x80)) break;
	}
	if (bytes) *bytes = i + 1;
	return value;
}

VarLong *
bullshitcore_network_varlong_encode(int64_t value)
{
	uint_fast8_t bytes = 1;
	while ((uint64_t)value >> 7 * bytes && bytes < 10) ++bytes;
	VarLong *varlong = bullshitcore_memory_pool_retrieve(bytes);
	if (unlikely(!varlong)) return NULL;
	memset(varlong, 0, bytes);
	for (size_t i = 0; i < bytes; ++i)
		varlong[i] = (uint64_t)value >> 7 * i & 0x7F | (i != bytes - 1U) << 7;
	return varlong;
}

int64_t
bullshitcore_network_varlong_decode(const VarLong * restrict varlong, uint8_t * restrict bytes)
{
	int64_t value = 0;
	size_t i = 0;
	int8_t varlong_byte;
	for (;i < 10; ++i)
	{
		varlong_byte = varlong[i];
		value |= (varlong_byte & 0x7F) << 7 * i;
		if (!(varlong_byte & 0x80)) break;
	}
	if (bytes) *bytes = i + 1;
	return value;
}

String
bullshitcore_network_string_java_utf8_encode(UnicodeString codepoints)
{
	uint8_t *contents = bullshitcore_memory_pool_retrieve(STRING_MAXSIZE);
	if (unlikely(!contents)) return (String){ 0 };
	uint32_t codepoint;
	size_t characters = 0;
	for (size_t i = 0; i < codepoints.length; ++i)
	{
		codepoint = codepoints.contents[i];
		if (!codepoint) goto encode_null;
		else if (codepoint <= 0x7F)
		{
			contents[characters] = codepoint;
			++characters;
		}
		else if (codepoint <= 0x7FF)
		{
encode_null:
			contents[characters] = 0xC0 | codepoint >> 6 & 0x1F;
			contents[++characters] = 0x80 | codepoint & 0x3F;
			++characters;
		}
		else if (codepoint <= 0xFFFFU)
		{
			if (codepoint >= 0xD800U && codepoint <= 0xDFFFU) continue;
encode_surrogate_pair:
			contents[characters] = 0xE0 | codepoint >> 12 & 0xF;
			contents[++characters] = 0x80 | codepoint >> 6 & 0x3F;
			contents[++characters] = 0x80 | codepoint & 0x3F;
			++characters;
			if (codepoint >= 0xD800U && codepoint <= 0xDBFFU)
				goto low_surrogate;
		}
		else if (codepoint <= 0x10FFFFL)
		{
			codepoint = 0xD800U + (codepoint - 0x10000L << 10 & 0xFFC00L);
			goto encode_surrogate_pair;
low_surrogate:
			codepoint = 0xDC00U + (codepoint - 0x10000L & 0x3FF);
			goto encode_surrogate_pair;
		}
	}
	return (String){ bullshitcore_network_varint_encode(characters), contents };
}
