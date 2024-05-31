#include <stdlib.h>
#include "global_macros.h"
#include "network.h"

VarInt
bullshitcore_network_varint_encode(int32_t value)
{
	return bullshitcore_network_varlong_encode(value);
}

int32_t
bullshitcore_network_varint_decode(VarInt restrict varint, size_t * restrict bytes)
{
	int32_t value = 0;
	size_t i = 0;
	for (;i < 5; ++i)
	{
		const int8_t varint_byte = varint[i];
		value |= (varint_byte & 0x7F) << 7 * i;
		if (!(varint_byte & 0x80)) break;
	}
	if (bytes) *bytes = i + 1;
	return value;
}

VarLong
bullshitcore_network_varlong_encode(int64_t value)
{
	uint_fast8_t bytes = 1;
	while (value >> 7 * bytes) ++bytes;
	VarLong varlong = calloc(bytes, 1);
	if (unlikely(!varlong)) return NULL;
	for (size_t i = 0; i < bytes; ++i)
		varlong[i] = value >> 7 * i & 0x7F | (i != bytes - 1u) << 7;
	return varlong;
}

int64_t
bullshitcore_network_varlong_decode(VarLong restrict varlong, size_t * restrict bytes)
{
	int64_t value = 0;
	size_t i = 0;
	for (;i < 10; ++i)
	{
		const int8_t varlong_byte = varlong[i];
		value |= (varlong_byte & 0x7F) << 7 * i;
		if (!(varlong_byte & 0x80)) break;
	}
	if (bytes) *bytes = i + 1;
	return value;
}
