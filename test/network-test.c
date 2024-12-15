#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include "network.h"

#define htonll(x) (((uint64_t)htonl((x) >> 32) << 32) | htonl((x) & 0xFFFFFFFF))

int
main(void)
{
#ifdef BULLSHITCORE_NETWORK_VARINT_ENCODE_ZERO
	assert(!*bullshitcore_network_varint_encode(0));
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_ENCODE_ONE
	assert(*bullshitcore_network_varint_encode(1) == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_ENCODE_NEGATIVE_ONE
	const VarInt * const value = bullshitcore_network_varint_encode(-1);
	assert(*value == -1);
	assert(value[1] == -1);
	assert(value[2] == -1);
	assert(value[3] == -1);
	assert(value[4] == 15);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_ENCODE_INT32_MIN
	const VarInt * const value = bullshitcore_network_varint_encode(htonl(INT32_MIN));
	assert(*value == -64);
	assert(value[1] == -128);
	assert(value[2] == -128);
	assert(value[3] == -128);
	assert(!value[4]);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_ENCODE_INT32_MAX
	const VarInt * const value = bullshitcore_network_varint_encode(htonl(INT32_MAX));
	assert(*value == -65);
	assert(value[1] == -1);
	assert(value[2] == -1);
	assert(value[3] == -1);
	assert(value[4] == 15);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_ZERO
	assert(!bullshitcore_network_varint_decode(&(const VarInt){ 0 }, NULL));
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_ZERO_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(!bullshitcore_network_varint_decode(&(const VarInt){ 0 }, &byte_count));
	assert(byte_count == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_ONE
	assert(bullshitcore_network_varint_decode(&(const VarInt){ 1 }, NULL) == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_ONE_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varint_decode(&(const VarInt){ 1 }, &byte_count) == 1);
	assert(byte_count == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_NEGATIVE_ONE
	assert(bullshitcore_network_varint_decode((const VarInt[]){ -1, -1, -1, -1, 127 }, NULL) == -1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_NEGATIVE_ONE_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varint_decode((const VarInt[]){ -1, -1, -1, -1, 127 }, &byte_count) == -1);
	assert(byte_count == 5);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_INT32_MIN
	assert(bullshitcore_network_varint_decode((const VarInt[]){ -64, -128, -128, -128, 0 }, NULL) == htonl(INT32_MIN));
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_INT32_MIN_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varint_decode((const VarInt[]){ -64, -128, -128, -128, 0 }, &byte_count) == htonl(INT32_MIN));
	assert(byte_count == 5);
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_INT32_MAX
	assert(bullshitcore_network_varint_decode((const VarInt[]){ -65, -1, -1, -1, 15 }, NULL) == htonl(INT32_MAX));
#endif
#ifdef BULLSHITCORE_NETWORK_VARINT_DECODE_INT32_MAX_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varint_decode((const VarInt[]){ -65, -1, -1, -1, 15 }, &byte_count) == htonl(INT32_MAX));
	assert(byte_count == 5);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_ENCODE_ZERO
	assert(!*bullshitcore_network_varint_encode(0));
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_ENCODE_ONE
	assert(*bullshitcore_network_varlong_encode(1) == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_ENCODE_NEGATIVE_ONE
	const VarLong * const value = bullshitcore_network_varlong_encode(-1);
	assert(*value == -1);
	assert(value[1] == -1);
	assert(value[2] == -1);
	assert(value[3] == -1);
	assert(value[4] == -1);
	assert(value[5] == -1);
	assert(value[6] == -1);
	assert(value[7] == -1);
	assert(value[8] == -1);
	assert(value[9] == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_ENCODE_INT64_MIN
	const VarInt * const value = bullshitcore_network_varlong_encode(htonll(INT64_MIN));
	assert(*value == -64);
	assert(value[1] == -128);
	assert(value[2] == -128);
	assert(value[3] == -128);
	assert(value[4] == -128);
	assert(value[5] == -128);
	assert(value[6] == -128);
	assert(value[7] == -128);
	assert(value[8] == -128);
	assert(!value[9]);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_ENCODE_INT64_MAX
	const VarLong * const value = bullshitcore_network_varlong_encode(htonll(INT64_MAX));
	assert(*value == -65);
	assert(value[1] == -1);
	assert(value[2] == -1);
	assert(value[3] == -1);
	assert(value[4] == -1);
	assert(value[5] == -1);
	assert(value[6] == -1);
	assert(value[7] == -1);
	assert(value[8] == -1);
	assert(value[9] == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_ZERO
	assert(!bullshitcore_network_varlong_decode(&(const VarLong){ 0 }, NULL));
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_ZERO_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(!bullshitcore_network_varlong_decode(&(const VarLong){ 0 }, &byte_count));
	assert(byte_count == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_ONE
	assert(bullshitcore_network_varlong_decode(&(const VarLong){ 1 }, NULL) == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_ONE_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varlong_decode(&(const VarLong){ 1 }, &byte_count) == 1);
	assert(byte_count == 1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_NEGATIVE_ONE
	assert(bullshitcore_network_varlong_decode((const VarLong[]){ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, NULL) == -1);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_NEGATIVE_ONE_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varlong_decode((const VarLong[]){ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, &byte_count) == -1);
	assert(byte_count == 10);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_INT64_MIN
	assert(bullshitcore_network_varlong_decode((const VarLong[]){ -64, -128, -128, -128, -128, -128, -128, -128, -128, 0 }, NULL) == htonll(INT64_MIN));
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_INT64_MIN_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varlong_decode((const VarLong[]){ -64, -128, -128, -128, -128, -128, -128, -128, -128, 0 }, &byte_count) == htonll(INT64_MIN));
	assert(byte_count == 10);
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_INT64_MAX
	assert(bullshitcore_network_varlong_decode((const VarLong[]){ -65, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, NULL) == htonll(INT64_MAX));
#endif
#ifdef BULLSHITCORE_NETWORK_VARLONG_DECODE_INT64_MAX_AND_GET_BYTE_COUNT
	uint8_t byte_count;
	assert(bullshitcore_network_varlong_decode((const VarLong[]){ -65, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, &byte_count) == htonll(INT64_MAX));
	assert(byte_count == 10);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_EMPTY
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 0, &(const uint32_t){ 0 } });
	assert(!*string.length);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_NULL
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0, 0 } });
	assert(*string.length == 2);
	assert(*string.contents == 0xC0);
	assert(string.contents[1] == 0x80);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_7F
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0x7F, 0 } });
	assert(*string.length == 1);
	assert(*string.contents == 0x7F);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_80
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0x80, 0 } });
	assert(*string.length == 2);
	assert(*string.contents == 0xD0);
	assert(string.contents[1] == 0x80);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_7FF
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0x7FF, 0 } });
	assert(*string.length == 2);
	assert(*string.contents == 0xDF);
	assert(string.contents[1] == 0xBF);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_800
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0x800, 0 } });
	assert(*string.length == 3);
	assert(*string.contents == 0xE8);
	assert(string.contents[1] == 0x80);
	assert(string.contents[2] == 0x80);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_D7FF
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0xD7FFU, 0 } });
	assert(*string.length == 3);
	assert(*string.contents == 0xED);
	assert(string.contents[1] == 0x9F);
	assert(string.contents[2] == 0xBF);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_FFFF
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0xFFFFU, 0 } });
	assert(*string.length == 3);
	assert(*string.contents == 0xEF);
	assert(string.contents[1] == 0xBF);
	assert(string.contents[2] == 0xBF);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_10000
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0x10000L, 0 } });
	assert(*string.length == 6);
	assert(*string.contents == 0xED);
	assert(string.contents[1] == 0xAF);
	assert(string.contents[2] == 0x80);
	assert(string.contents[3] == 0xED);
	assert(string.contents[4] == 0xB0);
	assert(string.contents[5] == 0x80);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_10FFFF
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0x10FFFFL, 0 } });
	assert(*string.length == 6);
	assert(*string.contents == 0xED);
	assert(string.contents[1] == 0xAF);
	assert(string.contents[2] == 0xBF);
	assert(string.contents[3] == 0xED);
	assert(string.contents[4] == 0xBF);
	assert(string.contents[5] == 0xBF);
#endif
#ifdef BULLSHITCORE_NETWORK_STRING_JAVA_UTF8_ENCODE_110000
	const String string = bullshitcore_network_string_java_utf8_encode((UnicodeString){ 1, (const uint32_t[]){ 0x110000L, 0 } });
	assert(!*string.length);
#endif
}
