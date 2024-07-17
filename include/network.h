#ifndef BULLSHITCORE_NETWORK
#define BULLSHITCORE_NETWORK

#include <stdint.h>
#include "nbt.h"

typedef _Bool Boolean;
#define true 1
#define false 0
typedef int8_t VarInt;
typedef int8_t VarLong;
typedef struct
{
	const VarInt *length;
	const uint8_t *contents;
} String;
typedef struct
{
	size_t length;
	const uint32_t *contents;
} UnicodeString;
typedef NBT TextComponent;
typedef String JSONTextComponent;
#define JSONTEXTCOMPONENT_MAXSIZE 262144
typedef String Identifier;
#define IDENTIFIER_MAXSIZE 32767
typedef struct
{
	uint8_t index;
	VarInt *type;
	void *value;
} EntityMetadata[];
typedef struct
{
	Boolean present;
	VarInt *item;
	uint8_t item_count;
	NBT NBT;
} Slot;
typedef uint64_t Position;
typedef uint8_t Angle;
typedef uint64_t UUID[2];
typedef struct
{
	VarInt *length;
	uint64_t data[];
} BitSet;
typedef uint8_t FixedBitSet[];
typedef struct
{
	VarInt *length;
	VarInt *packet_identifier;
} PacketHeader;
typedef struct
{
	VarInt *packet_length;
	VarInt *data_length;
	VarInt *packet_identifier;
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
enum Packet_Handshake
{
	Packet_Handshake
};
enum Packet_Status_Client
{
	Packet_Status_Client_Request,
	Packet_Status_Client_Ping_Request
};
enum Packet_Status_Server
{
	Packet_Status_Server_Response,
	Packet_Status_Server_Ping_Response
};
enum Packet_Login_Client
{
	Packet_Login_Client_Login_Start,
	Packet_Login_Client_Encryption_Response,
	Packet_Login_Client_Login_Plugin_Response,
	Packet_Login_Client_Login_Acknowledged,
	Packet_Login_Client_Cookie_Response
};
enum Packet_Login_Server
{
	Packet_Login_Server_Disconnect,
	Packet_Login_Server_Encryption_Request,
	Packet_Login_Server_Login_Success,
	Packet_Login_Server_Set_Compression,
	Packet_Login_Server_Login_Plugin_Request,
	Packet_Login_Server_Cookie_Request
};
enum Packet_Configuration_Client
{
	Packet_Configuration_Client_Client_Information,
	Packet_Configuration_Client_Cookie_Response,
	Packet_Configuration_Client_Plugin_Message,
	Packet_Configuration_Client_Finish_Configuration_Acknowledge,
	Packet_Configuration_Client_Keep_Alive,
	Packet_Configuration_Client_Pong, // pog
	Packet_Configuration_Client_Resource_Pack_Response,
	Packet_Configuration_Client_Known_Packs
};
enum Packet_Configuration_Server
{
	Packet_Configuration_Server_Cookie_Request,
	Packet_Configuration_Server_Plugin_Message,
	Packet_Configuration_Server_Disconnect,
	Packet_Configuration_Server_Finish_Configuration,
	Packet_Configuration_Server_Keep_Alive,
	Packet_Configuration_Server_Ping,
	Packet_Configuration_Server_Reset_Chat,
	Packet_Configuration_Server_Registry_Data,
	Packet_Configuration_Server_Remove_Resource_Pack,
	Packet_Configuration_Server_Add_Resource_Pack,
	Packet_Configuration_Server_Store_Cookie,
	Packet_Configuration_Server_Transfer,
	Packet_Configuration_Server_Feature_Flags,
	Packet_Configuration_Server_Update_Tags,
	Packet_Configuration_Server_Known_Packs,
	Packet_Configuration_Server_Custom_Report_Details,
	Packet_Configuration_Server_Server_Links
};
enum Packet_Play_Server
{
	Packet_Play_Server_Login = 0x2B
};

VarInt *bullshitcore_network_varint_encode(uint32_t value);
int32_t bullshitcore_network_varint_decode(const VarInt * restrict varint, uint8_t * restrict bytes);
VarLong *bullshitcore_network_varlong_encode(uint64_t value);
int64_t bullshitcore_network_varlong_decode(const VarLong * restrict varlong, uint8_t * restrict bytes);
String bullshitcore_network_string_java_utf8_encode(const UnicodeString codepoints);

#endif
