#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <string.h>
#include "global_macros.h"
#include "network.h"

VarInt *
bullshitcore_network_varint_encode(int32_t value)
{
	return bullshitcore_network_varlong_encode(value);
}

int32_t
bullshitcore_network_varint_decode(const VarInt * const restrict varint, uint8_t * restrict bytes)
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
	while (value >> 7 * bytes) ++bytes;
	VarLong *varlong = calloc(bytes, 1);
	if (unlikely(!varlong)) return NULL;
	for (size_t i = 0; i < bytes; ++i)
		varlong[i] = value >> 7 * i & 0x7F | (i != bytes - 1u) << 7;
	return varlong;
}

int64_t
bullshitcore_network_varlong_decode(const VarLong * const restrict varlong, uint8_t * restrict bytes)
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
bullshitcore_network_string_java_utf8_encode(const UnicodeString codepoints)
{
	uint8_t *contents = malloc(98301);
	if (unlikely(!contents)) return (String){ 0 };
	uint32_t codepoint;
	size_t characters = 0;
	for (size_t i = 0; i < codepoints.length; ++i)
	{
		codepoint = codepoints.contents[i];
		if (!codepoint)
		{
			contents[characters] = 0xC0;
			contents[++characters] = 0x80;
			++characters;
		}
		else if (codepoint <= 0x7F)
		{
			contents[characters] = codepoint;
			++characters;
		}
		else if (codepoint <= 0x7FF)
		{
			contents[characters] = 0xC0 | codepoint >> 6 & 0x1F;
			contents[++characters] = 0x80 | codepoint & 0x3F;
			++characters;
		}
		else if (codepoint <= 0xFFFF)
		{
			if (codepoint >= 0xD800 && codepoint <= 0xDFFF) continue;
			contents[characters] = 0xE0 | codepoint >> 12 & 0xF;
			contents[++characters] = 0x80 | codepoint >> 6 & 0x3F;
			contents[++characters] = 0x80 | codepoint & 0x3F;
			++characters;
		}
		else if (codepoint <= 0x10FFFF)
		{
			contents[characters] = 0xD800 + (codepoint - 0x10000 >> 10);
			contents[++characters] = 0xDC00 + (codepoint - 0x10000 & 0x3FF);
			++characters;
		}
	}
	return (String){ bullshitcore_network_varint_encode(characters + 1), contents };
}
