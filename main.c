#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
// DO NOT MOVE NEXT 3 (THREE) INCLUDE DIRECTIVES AFTER ANY OTHER WOLFSSL HEADER INCLUDE DIRECTIVE
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
// ADD NEW WOLFSSL HEADERS AFTER THIS LINE
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/hash.h>
#include "config.h"
#include "global_macros.h"
#include "log.h"
#include "memory.h"
#include "network.h"
#include "registries.h"
#include "version"
#include "world.h"

#define PERROR_AND_GOTO_DESTROY(s, object) { perror(s); goto destroy_ ## object; }
#define THREAD_STACK_SIZE 8388608L
#define SEND(...) \
{ \
	const uintptr_t args[] = { __VA_ARGS__ }; \
	size_t data_length, interthread_buffer_offset = 0; \
	ret = pthread_mutex_lock(interthread_buffer_mutex); \
	if (unlikely(ret)) \
	{ \
		errno = ret; \
		goto clear_stack_receiver; \
	} \
	for (size_t i = 0; i < NUMOF(args); i += 2) \
	{ \
		data_length = (size_t)args[i + 1]; \
		memcpy(interthread_buffer + interthread_buffer_offset, (void *)args[i], data_length); \
		interthread_buffer_offset += data_length; \
	} \
	*interthread_buffer_length = interthread_buffer_offset; \
	ret = pthread_cond_signal(interthread_buffer_condition); \
	if (unlikely(ret)) \
	{ \
		errno = ret; \
		goto clear_stack_receiver; \
	} \
	ret = pthread_mutex_unlock(interthread_buffer_mutex); \
	if (unlikely(ret)) \
	{ \
		errno = ret; \
		goto clear_stack_receiver; \
	} \
}
#define PACKET_MAXSIZE 2097151L
#define ACTUAL_SIMULATION_DISTANCE (RENDER_DISTANCE <= SIMULATION_DISTANCE ? RENDER_DISTANCE : SIMULATION_DISTANCE)

struct ThreadArguments
{
	int client_endpoint;
	uint8_t *interthread_buffer;
	size_t *interthread_buffer_length;
	pthread_mutex_t *interthread_buffer_mutex;
	pthread_cond_t *interthread_buffer_condition;
	sem_t *client_thread_arguments_semaphore;
	enum Connection_State *connection_state;
	pthread_t sender_thread;
};

static void *
packet_receiver(void *thread_arguments)
{
	{
		const int client_endpoint = ((struct ThreadArguments *)thread_arguments)->client_endpoint;
		uint8_t * const interthread_buffer = ((struct ThreadArguments *)thread_arguments)->interthread_buffer;
		size_t * const interthread_buffer_length = ((struct ThreadArguments *)thread_arguments)->interthread_buffer_length;
		pthread_mutex_t * const interthread_buffer_mutex = ((struct ThreadArguments *)thread_arguments)->interthread_buffer_mutex;
		pthread_cond_t * const interthread_buffer_condition = ((struct ThreadArguments *)thread_arguments)->interthread_buffer_condition;
		enum Connection_State * const connection_state = ((struct ThreadArguments *)thread_arguments)->connection_state;
		const pthread_t sender_thread = ((struct ThreadArguments *)thread_arguments)->sender_thread;
		if (unlikely(sem_post(((struct ThreadArguments *)thread_arguments)->client_thread_arguments_semaphore) == -1))
			goto clear_stack_receiver;
		Boolean compression_enabled = false;
		PlayerInformation player_information = { 0 };
		ssize_t bytes_read;
		int8_t buffer[PACKET_MAXSIZE];
		uint8_t packet_next_boundary;
		size_t buffer_offset;
		uint32_t packet_identifier;
		int ret;
		while (1)
		{
			bytes_read = recv(client_endpoint, buffer, PACKET_MAXSIZE, 0);
			if (!bytes_read) goto close_connection;
			else if (unlikely(bytes_read == -1)) goto clear_stack_receiver;
			bullshitcore_network_varint_decode(buffer, &packet_next_boundary);
			buffer_offset = packet_next_boundary;
			packet_identifier = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
			buffer_offset += packet_next_boundary;
			switch (*connection_state)
			{
				case Connection_State_Handshaking:
				{
					if (packet_identifier == Packet_Handshaking_Client_Handshake)
					{
						const uint32_t client_protocol_version = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
						buffer_offset += packet_next_boundary;
						if (client_protocol_version != PROTOCOL_VERSION)
							bullshitcore_log_log("Warning! Client and server protocol version mismatch.");
						const uint32_t server_address_string_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
						buffer_offset += packet_next_boundary;
						const uint32_t target_state = bullshitcore_network_varint_decode(buffer + buffer_offset + server_address_string_length + 2, &packet_next_boundary);
						buffer_offset += packet_next_boundary;
						if (target_state == Connection_State_Status || target_state == Connection_State_Login)
							*connection_state = target_state;
					}
					break;
				}
				case Connection_State_Status:
				{
					switch (packet_identifier)
					{
						case Packet_Status_Client_Status_Request:
						{
							const uint8_t * const text = (const uint8_t *)"{\"version\":{\"name\":\"" MINECRAFT_VERSION "\",\"protocol\":" EXPAND_AND_STRINGIFY(PROTOCOL_VERSION) "},\"players\":{\"max\":" EXPAND_AND_STRINGIFY(MAX_PLAYERS) ",\"online\":0},\"description\":{\"text\":\"" DESCRIPTION "\"},\"favicon\":\"" FAVICON "\"}";
							const size_t text_length = strlen((const char *)text);
							const JSONTextComponent packet_payload =
							{
								bullshitcore_network_varint_encode(text_length),
								text
							};
							if (text_length > JSONTEXTCOMPONENT_MAXSIZE) break;
							uint8_t packet_payload_length_length;
							bullshitcore_network_varint_decode(packet_payload.length, &packet_payload_length_length);
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Status_Server_Status_Response);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
								+ packet_payload_length_length
								+ text_length);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)packet_payload.length, packet_payload_length_length,
								(uintptr_t)packet_payload.contents, text_length)
							bullshitcore_memory_leave(packet_payload.length, packet_payload_length_length);
							bullshitcore_memory_leave(packet_identifier_varint, packet_identifier_varint_length);
							bullshitcore_memory_leave(packet_length_varint, packet_length_varint_length);
							break;
						}
						case Packet_Status_Client_Ping_Request:
						{
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Status_Server_Ping_Response);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length + 8);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)buffer, 8)
							bullshitcore_memory_leave(packet_identifier_varint, packet_identifier_varint_length);
							bullshitcore_memory_leave(packet_length_varint, packet_length_varint_length);
							goto close_connection;
							break;
						}
					}
					break;
				}
				case Connection_State_Login:
				{
					switch (packet_identifier)
					{
						case Packet_Login_Client_Login_Start:
						{
							uint8_t username_length_length;
							const uint32_t username_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &username_length_length);
							buffer_offset += username_length_length + username_length;
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Login_Server_Login_Success);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const properties_count_varint = bullshitcore_network_varint_encode(1);
							uint8_t properties_count_varint_length;
							bullshitcore_network_varint_decode(properties_count_varint, &properties_count_varint_length);
							const String name = { bullshitcore_network_varint_encode(strlen("textures")), (const uint8_t *)"textures" };
							uint8_t name_length_varint_length;
							bullshitcore_network_varint_decode(name.length, &name_length_varint_length);
							const String value = { bullshitcore_network_varint_encode(strlen("")), (const uint8_t *)"" };
							uint8_t value_length_varint_length;
							bullshitcore_network_varint_decode(value.length, &value_length_varint_length);
							const String signature = { bullshitcore_network_varint_encode(strlen("")), (const uint8_t *)"" };
							uint8_t signature_length_varint_length;
							bullshitcore_network_varint_decode(signature.length, &signature_length_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
								+ 16
								+ username_length_length + username_length
								+ properties_count_varint_length
								+ name_length_varint_length
								+ strlen((const char *)name.contents)
								+ value_length_varint_length
								+ strlen((const char *)value.contents)
								+ sizeof(Boolean)
								+ signature_length_varint_length
								+ strlen((const char *)signature.contents)
								+ sizeof(Boolean));
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)(buffer + buffer_offset), 16,
								(uintptr_t)(buffer + buffer_offset - username_length_length - username_length), username_length_length + username_length,
								(uintptr_t)properties_count_varint, properties_count_varint_length,
								(uintptr_t)name.length, name_length_varint_length,
								(uintptr_t)name.contents, strlen((const char *)name.contents),
								(uintptr_t)value.length, value_length_varint_length,
								(uintptr_t)value.contents, strlen((const char *)value.contents),
								(uintptr_t)&(const Boolean){ true }, sizeof(Boolean),
								(uintptr_t)signature.length, signature_length_varint_length,
								(uintptr_t)signature.contents, strlen((const char *)signature.contents),
								(uintptr_t)&(const Boolean){ true }, sizeof(Boolean))
							bullshitcore_memory_leave(packet_identifier_varint, packet_identifier_varint_length);
							bullshitcore_memory_leave(properties_count_varint, properties_count_varint_length);
							bullshitcore_memory_leave(name.length, name_length_varint_length);
							bullshitcore_memory_leave(value.length, value_length_varint_length);
							bullshitcore_memory_leave(signature.length, signature_length_varint_length);
							bullshitcore_memory_leave(packet_length_varint, packet_length_varint_length);
							break;
						}
						case Packet_Login_Client_Encryption_Response:
						{
							break;
						}
						case Packet_Login_Client_Login_Plugin_Response:
						{
							break;
						}
						case Packet_Login_Client_Login_Acknowledged:
						{
							*connection_state = Connection_State_Configuration;
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Configuration_Server_Known_Packs);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const known_packs_count_varint = bullshitcore_network_varint_encode(1);
							uint8_t known_packs_count_varint_length;
							bullshitcore_network_varint_decode(known_packs_count_varint, &known_packs_count_varint_length);
							const String namespace = { bullshitcore_network_varint_encode(strlen("minecraft")), (const uint8_t *)"minecraft" };
							const String identifier = { bullshitcore_network_varint_encode(strlen("core")), (const uint8_t *)"core" };
							const String version = { bullshitcore_network_varint_encode(strlen(MINECRAFT_VERSION)), (const uint8_t *)MINECRAFT_VERSION };
							uint8_t namespace_length_varint_length;
							bullshitcore_network_varint_decode(namespace.length, &namespace_length_varint_length);
							uint8_t identifier_length_varint_length;
							bullshitcore_network_varint_decode(identifier.length, &identifier_length_varint_length);
							uint8_t version_length_varint_length;
							bullshitcore_network_varint_decode(version.length, &version_length_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
								+ known_packs_count_varint_length
								+ namespace_length_varint_length
								+ strlen((const char *)namespace.contents)
								+ identifier_length_varint_length
								+ strlen((const char *)identifier.contents)
								+ version_length_varint_length
								+ strlen((const char *)version.contents));
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)known_packs_count_varint, known_packs_count_varint_length,
								(uintptr_t)namespace.length, namespace_length_varint_length,
								(uintptr_t)namespace.contents, strlen((const char *)namespace.contents),
								(uintptr_t)identifier.length, identifier_length_varint_length,
								(uintptr_t)identifier.contents, strlen((const char *)identifier.contents),
								(uintptr_t)version.length, version_length_varint_length,
								(uintptr_t)version.contents, strlen((const char *)version.contents))
							bullshitcore_memory_leave(packet_identifier_varint, packet_identifier_varint_length);
							bullshitcore_memory_leave(known_packs_count_varint, known_packs_count_varint_length);
							bullshitcore_memory_leave(namespace.length, namespace_length_varint_length);
							bullshitcore_memory_leave(identifier.length, identifier_length_varint_length);
							bullshitcore_memory_leave(version.length, version_length_varint_length);
							bullshitcore_memory_leave(packet_length_varint, packet_length_varint_length);
							break;
						}
						case Packet_Login_Client_Cookie_Response:
						{
							break;
						}
					}
					break;
				}
				case Connection_State_Transfer:
				{
					break;
				}
				case Connection_State_Configuration:
				{
					switch (packet_identifier)
					{
						case Packet_Configuration_Client_Client_Information:
						{
							const uint32_t locale_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							memcpy(player_information.locale, buffer + buffer_offset, locale_length > NUMOF(player_information.locale) ? NUMOF(player_information.locale) : locale_length);
							buffer_offset += locale_length;
							player_information.render_distance = buffer[buffer_offset];
							++buffer_offset;
							player_information.chat_mode = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							player_information.colored_chat = buffer[buffer_offset];
							++buffer_offset;
							player_information.displayed_skin_parts = buffer[buffer_offset];
							++buffer_offset;
							player_information.main_hand = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							player_information.text_filtering = buffer[buffer_offset];
							++buffer_offset;
							player_information.server_listing = buffer[buffer_offset];
							break;
						}
						case Packet_Configuration_Client_Cookie_Response:
						{
							break;
						}
						case Packet_Configuration_Client_Plugin_Message:
						{
							break;
						}
						case Packet_Configuration_Client_Finish_Configuration_Acknowledge:
						{
							*connection_state = Connection_State_Play;
							VarInt *packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Play_Server_Login);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							const uint8_t * const overworld = (const uint8_t *)"minecraft:overworld";
							const uint8_t * const nether = (const uint8_t *)"minecraft:the_nether";
							const uint8_t * const end = (const uint8_t *)"minecraft:the_end";
							const Identifier dimensions[] =
							{
								{
									bullshitcore_network_varint_encode(strlen((const char *)overworld)),
									overworld
								},
								{
									bullshitcore_network_varint_encode(strlen((const char *)nether)),
									nether
								},
								{
									bullshitcore_network_varint_encode(strlen((const char *)end)),
									end
								}
							};
							VarInt * const dimension_count_varint = bullshitcore_network_varint_encode(NUMOF(dimensions));
							uint8_t dimension_count_varint_length;
							bullshitcore_network_varint_decode(dimension_count_varint, &dimension_count_varint_length);
							uint8_t dimension_1_length_length;
							bullshitcore_network_varint_decode(dimensions[0].length, &dimension_1_length_length);
							uint8_t dimension_2_length_length;
							bullshitcore_network_varint_decode(dimensions[1].length, &dimension_2_length_length);
							uint8_t dimension_3_length_length;
							bullshitcore_network_varint_decode(dimensions[2].length, &dimension_3_length_length);
							VarInt * const max_players_varint = bullshitcore_network_varint_encode(MAX_PLAYERS);
							uint8_t max_players_varint_length;
							bullshitcore_network_varint_decode(max_players_varint, &max_players_varint_length);
							VarInt * const render_distance_varint = bullshitcore_network_varint_encode(RENDER_DISTANCE);
							uint8_t render_distance_varint_length;
							bullshitcore_network_varint_decode(render_distance_varint, &render_distance_varint_length);
							VarInt * const simulation_distance_varint = bullshitcore_network_varint_encode(ACTUAL_SIMULATION_DISTANCE);
							uint8_t simulation_distance_varint_length;
							bullshitcore_network_varint_decode(simulation_distance_varint, &simulation_distance_varint_length);
							VarInt * const dimension_type_varint = bullshitcore_network_varint_encode(0);
							uint8_t dimension_type_varint_length;
							bullshitcore_network_varint_decode(dimension_type_varint, &dimension_type_varint_length);
							int64_t seed_hash = 0;
							ret = wc_Sha256Hash((const byte *)&seed_hash, sizeof seed_hash, (byte *)&seed_hash);
							if (unlikely(ret))
							{
								fprintf(stderr, "wc_Sha256Hash: %s\n", wc_GetErrorString(ret));
								goto clear_stack_receiver;
							}
							VarInt * const portal_cooldown_varint = bullshitcore_network_varint_encode(0);
							uint8_t portal_cooldown_varint_length;
							bullshitcore_network_varint_decode(portal_cooldown_varint, &portal_cooldown_varint_length);
							VarInt *packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
								+ sizeof(int32_t)
								+ sizeof(Boolean)
								+ dimension_count_varint_length
								+ dimension_1_length_length
								+ bullshitcore_network_varint_decode(dimensions[0].length, NULL)
								+ dimension_2_length_length
								+ bullshitcore_network_varint_decode(dimensions[1].length, NULL)
								+ dimension_3_length_length
								+ bullshitcore_network_varint_decode(dimensions[2].length, NULL)
								+ max_players_varint_length
								+ render_distance_varint_length
								+ simulation_distance_varint_length
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ dimension_type_varint_length
								+ dimension_1_length_length
								+ bullshitcore_network_varint_decode(dimensions[0].length, NULL)
								+ sizeof seed_hash
								+ sizeof(uint8_t)
								+ sizeof(int8_t)
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ portal_cooldown_varint_length
								+ sizeof(Boolean));
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							VarInt * const packet_2_identifier_varint = bullshitcore_network_varint_encode(Packet_Play_Server_Game_Event);
							uint8_t packet_2_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_2_identifier_varint, &packet_2_identifier_varint_length);
							VarInt * const packet_2_length_varint = bullshitcore_network_varint_encode(packet_2_identifier_varint_length
								+ sizeof(uint8_t)
								+ (sizeof(float) >= 4 ? 4 : sizeof(float)));
							uint8_t packet_2_length_varint_length;
							bullshitcore_network_varint_decode(packet_2_length_varint, &packet_2_length_varint_length);
							VarInt * const packet_3_identifier_varint = bullshitcore_network_varint_encode(Packet_Play_Server_Plugin_Message);
							uint8_t packet_3_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_3_identifier_varint, &packet_3_identifier_varint_length);
							const Identifier channel_identifier = { bullshitcore_network_varint_encode(strlen("minecraft:brand")), (const uint8_t *)"minecraft:brand" };
							uint8_t channel_identifier_length_varint_length;
							bullshitcore_network_varint_decode(channel_identifier.length, &channel_identifier_length_varint_length);
							const String server_brand = { bullshitcore_network_varint_encode(strlen(SERVER_BRAND)), (const uint8_t *)SERVER_BRAND };
							uint8_t server_brand_length_varint_length;
							bullshitcore_network_varint_decode(server_brand.length, &server_brand_length_varint_length);
							VarInt * const packet_3_length_varint = bullshitcore_network_varint_encode(packet_3_identifier_varint_length
								+ channel_identifier_length_varint_length
								+ bullshitcore_network_varint_decode(channel_identifier.length, NULL)
								+ server_brand_length_varint_length
								+ bullshitcore_network_varint_decode(server_brand.length, NULL));
							uint8_t packet_3_length_varint_length;
							bullshitcore_network_varint_decode(packet_3_length_varint, &packet_3_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)&(const int32_t){ 0 }, sizeof(int32_t),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)dimension_count_varint, dimension_count_varint_length,
								(uintptr_t)dimensions[0].length, dimension_1_length_length,
								(uintptr_t)dimensions[0].contents, bullshitcore_network_varint_decode(dimensions[0].length, NULL),
								(uintptr_t)dimensions[1].length, dimension_2_length_length,
								(uintptr_t)dimensions[1].contents, bullshitcore_network_varint_decode(dimensions[1].length, NULL),
								(uintptr_t)dimensions[2].length, dimension_3_length_length,
								(uintptr_t)dimensions[2].contents, bullshitcore_network_varint_decode(dimensions[2].length, NULL),
								(uintptr_t)max_players_varint, max_players_varint_length,
								(uintptr_t)render_distance_varint, render_distance_varint_length,
								(uintptr_t)simulation_distance_varint, simulation_distance_varint_length,
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ true }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)dimension_type_varint, dimension_type_varint_length,
								(uintptr_t)dimensions[0].length, dimension_1_length_length,
								(uintptr_t)dimensions[0].contents, bullshitcore_network_varint_decode(dimensions[0].length, NULL),
								(uintptr_t)&seed_hash, sizeof seed_hash,
								(uintptr_t)&(const uint8_t){ 3 }, sizeof(uint8_t),
								(uintptr_t)&(const int8_t){ -1 }, sizeof(int8_t),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)portal_cooldown_varint, portal_cooldown_varint_length,
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)packet_2_length_varint, packet_2_length_varint_length,
								(uintptr_t)packet_2_identifier_varint, packet_2_identifier_varint_length,
								(uintptr_t)&(const uint8_t){ 13 }, sizeof(uint8_t),
								(uintptr_t)&(const float){ 0 }, sizeof(float) >= 4 ? 4 : sizeof(float),
								(uintptr_t)packet_3_length_varint, packet_3_length_varint_length,
								(uintptr_t)packet_3_identifier_varint, packet_3_identifier_varint_length,
								(uintptr_t)channel_identifier.length, channel_identifier_length_varint_length,
								(uintptr_t)channel_identifier.contents, bullshitcore_network_varint_decode(channel_identifier.length, NULL),
								(uintptr_t)server_brand.length, server_brand_length_varint_length,
								(uintptr_t)server_brand.contents, bullshitcore_network_varint_decode(server_brand.length, NULL))
							bullshitcore_memory_leave(packet_identifier_varint, packet_identifier_varint_length);
							bullshitcore_memory_leave(dimension_count_varint, dimension_count_varint_length);
							bullshitcore_memory_leave(dimensions[0].length, dimension_1_length_length);
							bullshitcore_memory_leave(dimensions[1].length, dimension_2_length_length);
							bullshitcore_memory_leave(dimensions[2].length, dimension_3_length_length);
							bullshitcore_memory_leave(max_players_varint, max_players_varint_length);
							bullshitcore_memory_leave(render_distance_varint, render_distance_varint_length);
							bullshitcore_memory_leave(simulation_distance_varint, simulation_distance_varint_length);
							bullshitcore_memory_leave(dimension_type_varint, dimension_type_varint_length);
							bullshitcore_memory_leave(portal_cooldown_varint, portal_cooldown_varint_length);
							bullshitcore_memory_leave(packet_length_varint, packet_length_varint_length);
							bullshitcore_memory_leave(packet_2_identifier_varint, packet_2_identifier_varint_length);
							bullshitcore_memory_leave(packet_2_length_varint, packet_2_length_varint_length);
							bullshitcore_memory_leave(packet_3_identifier_varint, packet_3_identifier_varint_length);
							bullshitcore_memory_leave(channel_identifier.length, channel_identifier_length_varint_length);
							bullshitcore_memory_leave(server_brand.length, server_brand_length_varint_length);
							bullshitcore_memory_leave(packet_3_length_varint, packet_3_length_varint_length);
							break;
						}
						case Packet_Configuration_Client_Keep_Alive:
						{
							break;
						}
						case Packet_Configuration_Client_Pong:
						{
							break;
						}
						case Packet_Configuration_Client_Resource_Pack_Response:
						{
							break;
						}
						case Packet_Configuration_Client_Known_Packs:
						{
							const uint32_t client_known_packs = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Configuration_Server_Finish_Configuration);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)REGISTRY_1, sizeof REGISTRY_1,
								(uintptr_t)REGISTRY_2, sizeof REGISTRY_2,
								(uintptr_t)REGISTRY_3, sizeof REGISTRY_3,
								(uintptr_t)REGISTRY_4, sizeof REGISTRY_4,
								(uintptr_t)REGISTRY_5, sizeof REGISTRY_5,
								(uintptr_t)REGISTRY_6, sizeof REGISTRY_6,
								(uintptr_t)REGISTRY_7, sizeof REGISTRY_7,
								(uintptr_t)REGISTRY_8, sizeof REGISTRY_8,
								(uintptr_t)REGISTRY_9, sizeof REGISTRY_9,
								(uintptr_t)REGISTRY_10, sizeof REGISTRY_10,
								(uintptr_t)REGISTRY_11, sizeof REGISTRY_11,
								(uintptr_t)REGISTRY_12, sizeof REGISTRY_12,
								(uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length)
							bullshitcore_memory_leave(packet_identifier_varint, packet_identifier_varint_length);
							bullshitcore_memory_leave(packet_length_varint, packet_length_varint_length);
							break;
						}
					}
					break;
				}
				case Connection_State_Play:
				{
					switch (packet_identifier)
					{
						case Packet_Play_Client_Confirm_Teleportation:
						{
							break;
						}
						case Packet_Play_Client_Query_Block_Entity_Tag:
						{
							break;
						}
						case Packet_Play_Client_Change_Difficulty:
						{
							break;
						}
						case Packet_Play_Client_Acknowledge_Message:
						{
							break;
						}
						case Packet_Play_Client_Chat_Command:
						{
							break;
						}
						case Packet_Play_Client_Signed_Chat_Command:
						{
							break;
						}
						case Packet_Play_Client_Chat_Message:
						{
							break;
						}
						case Packet_Play_Client_Player_Session:
						{
							break;
						}
						case Packet_Play_Client_Chunk_Batch_Received:
						{
							break;
						}
						case Packet_Play_Client_Client_Status:
						{
							break;
						}
						case Packet_Play_Client_Client_Information:
						{
							const uint32_t locale_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							memcpy(player_information.locale, buffer + buffer_offset, locale_length > NUMOF(player_information.locale) ? NUMOF(player_information.locale) : locale_length);
							buffer_offset += locale_length;
							player_information.render_distance = buffer[buffer_offset];
							++buffer_offset;
							player_information.chat_mode = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							player_information.colored_chat = buffer[buffer_offset];
							++buffer_offset;
							player_information.displayed_skin_parts = buffer[buffer_offset];
							++buffer_offset;
							player_information.main_hand = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							player_information.text_filtering = buffer[buffer_offset];
							++buffer_offset;
							player_information.server_listing = buffer[buffer_offset];
							break;
						}
						case Packet_Play_Client_Command_Suggestions_Request:
						{
							break;
						}
						case Packet_Play_Client_Acknowledge_Configuration:
						{
							*connection_state = Connection_State_Configuration;
							break;
						}
						case Packet_Play_Client_Click_Container_Button:
						{
							break;
						}
						case Packet_Play_Client_Click_Container:
						{
							break;
						}
						case Packet_Play_Client_Close_Container:
						{
							break;
						}
						case Packet_Play_Client_Change_Container_Slot_State:
						{
							break;
						}
						case Packet_Play_Client_Cookie_Response:
						{
							break;
						}
						case Packet_Play_Client_Plugin_Message:
						{
							break;
						}
						case Packet_Play_Client_Debug_Sample_Subscription:
						{
							break;
						}
						case Packet_Play_Client_Edit_Book:
						{
							break;
						}
						case Packet_Play_Client_Query_Entity_Tag:
						{
							break;
						}
						case Packet_Play_Client_Interact:
						{
							break;
						}
						case Packet_Play_Client_Jigsaw_Generate:
						{
							break;
						}
						case Packet_Play_Client_Keep_Alive:
						{
							break;
						}
						case Packet_Play_Client_Lock_Difficulty:
						{
							break;
						}
						case Packet_Play_Client_Set_Player_Position:
						{
							break;
						}
						case Packet_Play_Client_Set_Player_Position_And_Rotation:
						{
							break;
						}
						case Packet_Play_Client_Set_Player_Rotation:
						{
							break;
						}
						case Packet_Play_Client_Set_Player_On_Ground:
						{
							break;
						}
						case Packet_Play_Client_Move_Vehicle:
						{
							break;
						}
						case Packet_Play_Client_Paddle_Boat:
						{
							break;
						}
						case Packet_Play_Client_Pick_Item:
						{
							break;
						}
						case Packet_Play_Client_Ping_Request:
						{
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Play_Server_Ping_Response);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length + 8);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)buffer, 8)
							bullshitcore_memory_leave(packet_identifier_varint, packet_identifier_varint_length);
							bullshitcore_memory_leave(packet_length_varint, packet_length_varint_length);
							break;
						}
						case Packet_Play_Client_Place_Recipe:
						{
							break;
						}
						case Packet_Play_Client_Player_Abilities:
						{
							break;
						}
						case Packet_Play_Client_Player_Action:
						{
							break;
						}
						case Packet_Play_Client_Player_Command:
						{
							break;
						}
						case Packet_Play_Client_Player_Input:
						{
							break;
						}
						case Packet_Play_Client_Pong:
						{
							break;
						}
						case Packet_Play_Client_Change_Recipe_Book_Settings:
						{
							break;
						}
						case Packet_Play_Client_Set_Seen_Recipe:
						{
							break;
						}
						case Packet_Play_Client_Rename_Item:
						{
							break;
						}
						case Packet_Play_Client_Resource_Pack_Response:
						{
							break;
						}
						case Packet_Play_Client_Seen_Advancements:
						{
							break;
						}
						case Packet_Play_Client_Select_Trade:
						{
							break;
						}
						case Packet_Play_Client_Set_Beacon_Effect:
						{
							break;
						}
						case Packet_Play_Client_Set_Held_Item:
						{
							break;
						}
						case Packet_Play_Client_Program_Command_Block:
						{
							break;
						}
						case Packet_Play_Client_Program_Command_Block_Minecart:
						{
							break;
						}
						case Packet_Play_Client_Set_Creative_Mode_Slot:
						{
							break;
						}
						case Packet_Play_Client_Program_Jigsaw_Block:
						{
							break;
						}
						case Packet_Play_Client_Program_Structure_Block:
						{
							break;
						}
						case Packet_Play_Client_Update_Sign:
						{
							break;
						}
						case Packet_Play_Client_Swing_Arm:
						{
							break;
						}
						case Packet_Play_Client_Teleport_To_Entity:
						{
							break;
						}
						case Packet_Play_Client_Use_Item_On:
						{
							break;
						}
						case Packet_Play_Client_Use_Item:
						{
							break;
						}
					}
					break;
				}
			}
#ifndef NDEBUG
			bullshitcore_log_error("Reached an end of the packet jump table.");
#endif
		}
close_connection:
		ret = pthread_cancel(sender_thread);
		if (unlikely(ret))
		{
			errno = ret;
			goto clear_stack_receiver;
		}
		return NULL;
	}
clear_stack_receiver:;
	// TODO: Also pass failed function name.
	const int errno_copy = errno;
#ifndef NDEBUG
	bullshitcore_log_error_formatted("Receiver thread crashed! %s\n", strerror(errno_copy));
#endif
	int * const p_errno_copy = bullshitcore_memory_retrieve(sizeof errno_copy);
	if (unlikely(!p_errno_copy)) return (void *)1;
	*p_errno_copy = errno_copy;
	return p_errno_copy;
}

static void *
packet_sender(void *thread_arguments)
{
	{
		int ret = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
		if (unlikely(ret))
		{
			errno = ret;
			goto clear_stack_sender;
		}
		const int client_endpoint = ((struct ThreadArguments *)thread_arguments)->client_endpoint;
		uint8_t * const interthread_buffer = ((struct ThreadArguments *)thread_arguments)->interthread_buffer;
		size_t * const interthread_buffer_length = ((struct ThreadArguments *)thread_arguments)->interthread_buffer_length;
		pthread_mutex_t * const interthread_buffer_mutex = ((struct ThreadArguments *)thread_arguments)->interthread_buffer_mutex;
		pthread_cond_t * const interthread_buffer_condition = ((struct ThreadArguments *)thread_arguments)->interthread_buffer_condition;
		enum Connection_State * const connection_state = ((struct ThreadArguments *)thread_arguments)->connection_state;
		if (unlikely(sem_post(((struct ThreadArguments *)thread_arguments)->client_thread_arguments_semaphore) == -1))
			goto clear_stack_sender;
		struct timespec keep_alive_timeout;
		while (1)
		{
			ret = pthread_mutex_lock(interthread_buffer_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
			if (unlikely(clock_gettime(CLOCK_REALTIME, &keep_alive_timeout) == -1))
				goto clear_stack_sender;
			keep_alive_timeout.tv_sec += 15;
			ret = pthread_cond_timedwait(interthread_buffer_condition, interthread_buffer_mutex, &keep_alive_timeout);
			if (ret == ETIMEDOUT)
			{
				if (*connection_state != Connection_State_Configuration && *connection_state != Connection_State_Play)
					goto skip_send;
				int32_t packet_identifier = *connection_state == Connection_State_Configuration
					? Packet_Configuration_Server_Keep_Alive
					: Packet_Play_Server_Keep_Alive;
				VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(packet_identifier);
				uint8_t packet_identifier_varint_length;
				bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
				VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
					+ sizeof(int64_t));
				uint8_t packet_length_varint_length;
				bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
				memcpy(interthread_buffer, packet_length_varint, packet_length_varint_length);
				size_t interthread_buffer_offset = packet_length_varint_length;
				memcpy(interthread_buffer + interthread_buffer_offset, packet_identifier_varint, packet_identifier_varint_length);
				interthread_buffer_offset += packet_identifier_varint_length;
				memcpy(interthread_buffer + interthread_buffer_offset, &(const int64_t){ 0 }, sizeof(int64_t));
				*interthread_buffer_length = interthread_buffer_offset + sizeof(int64_t);
			}
			else if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
			if (unlikely(send(client_endpoint, interthread_buffer, *interthread_buffer_length, 0) == -1))
				goto clear_stack_sender;
skip_send:
			ret = pthread_mutex_unlock(interthread_buffer_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
		}
	}
	return NULL;
clear_stack_sender:;
	const int my_errno = errno;
#ifndef NDEBUG
	bullshitcore_log_error_formatted("Sender thread crashed! %s\n", strerror(my_errno));
#endif
	int * const p_my_errno = bullshitcore_memory_retrieve(sizeof my_errno);
	if (unlikely(!p_my_errno)) return (void *)1;
	*p_my_errno = my_errno;
	return p_my_errno;
}

int
main(void)
{
	pthread_attr_t thread_attributes;
	int ret = pthread_attr_init(&thread_attributes);
	if (unlikely(ret))
	{
		errno = ret;
		PERROR_AND_EXIT("pthread_attr_init")
	}
	ret = pthread_attr_setstacksize(&thread_attributes, THREAD_STACK_SIZE);
	if (unlikely(ret))
	{
		errno = ret;
		PERROR_AND_GOTO_DESTROY("pthread_attr_setstacksize", thread_attributes)
	}
	{
#ifdef IPV6
		const int server_endpoint = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#else
		const int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
		if (unlikely(server_endpoint == -1))
			PERROR_AND_GOTO_DESTROY("socket", thread_attributes)
		if (unlikely(setsockopt(server_endpoint, IPPROTO_TCP, TCP_NODELAY, &(const int){ 1 }, sizeof(int)) == -1))
			PERROR_AND_GOTO_DESTROY("setsockopt", server_endpoint)
		{
#ifdef IPV6
			struct in6_addr address;
			ret = inet_pton(AF_INET6, ADDRESS, &address);
#else
			struct in_addr address;
			ret = inet_pton(AF_INET, ADDRESS, &address);
#endif
			if (!ret)
			{
				fputs("An invalid server address was specified.\n", stderr);
				return EXIT_FAILURE;
			}
			else if (unlikely(ret == -1))
				PERROR_AND_GOTO_DESTROY("inet_pton", server_endpoint)
			struct sockaddr_storage server_address_data;
#ifdef IPV6
			memcpy(&server_address_data, &(const struct sockaddr_in6){ AF_INET6, htons(PORT), 0, address, 0 }, sizeof(struct sockaddr_in6));
#else
			memcpy(&server_address_data, &(const struct sockaddr_in){ AF_INET, htons(PORT), address }, sizeof(struct sockaddr_in));
#endif
			if (unlikely(bind(server_endpoint, (struct sockaddr *)&server_address_data, sizeof server_address_data) == -1))
				PERROR_AND_GOTO_DESTROY("bind", server_endpoint)
		}
		if (unlikely(listen(server_endpoint, SOMAXCONN) == -1))
			PERROR_AND_GOTO_DESTROY("listen", server_endpoint)
		bullshitcore_log_log("Initialisation is complete, awaiting new connections.");
		{
			int client_endpoint;
			struct sockaddr_storage client_address_data;
			struct sockaddr_in client_address_data_in;
			socklen_t client_address_data_length = sizeof client_address_data;
			{
				while (1)
				{
					client_endpoint = accept(server_endpoint, (struct sockaddr *)&client_address_data, &client_address_data_length);
					if (unlikely(client_endpoint == -1))
						PERROR_AND_GOTO_DESTROY("accept", server_endpoint)
					memcpy(&client_address_data_in, &client_address_data, sizeof client_address_data_in);
					{
						// TODO: Show IPv6 connections.
						char client_address_string[INET_ADDRSTRLEN];
						if (unlikely(!inet_ntop(AF_INET, &client_address_data_in.sin_addr, client_address_string, sizeof client_address_string)))
							PERROR_AND_GOTO_DESTROY("inet_ntop", server_endpoint)
						bullshitcore_log_log_formatted("A client (%s) has connected.\n", client_address_string);
					}
					struct ThreadArguments *thread_arguments = bullshitcore_memory_retrieve(sizeof *thread_arguments);
					if (unlikely(!thread_arguments))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					uint8_t *interthread_buffer = bullshitcore_memory_retrieve(sizeof *interthread_buffer * PACKET_MAXSIZE); // free me
					if (unlikely(!interthread_buffer))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					size_t *interthread_buffer_length = bullshitcore_memory_retrieve(sizeof *interthread_buffer_length); // free me
					if (unlikely(!interthread_buffer_length))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					pthread_mutex_t *interthread_buffer_mutex = bullshitcore_memory_retrieve(sizeof *interthread_buffer_mutex); // free me
					if (unlikely(!interthread_buffer_mutex))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					ret = pthread_mutex_init(interthread_buffer_mutex, NULL);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_mutex_init", client_endpoint)
					}
					pthread_cond_t *interthread_buffer_condition = bullshitcore_memory_retrieve(sizeof *interthread_buffer_condition); // free me
					if (unlikely(!interthread_buffer_condition))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					ret = pthread_cond_init(interthread_buffer_condition, NULL);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_cond_init", client_endpoint)
					}
					sem_t *client_thread_arguments_semaphore = bullshitcore_memory_retrieve(sizeof *client_thread_arguments_semaphore);
					if (unlikely(!client_thread_arguments_semaphore))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					if (unlikely(sem_init(client_thread_arguments_semaphore, 0, 0) == -1))
						PERROR_AND_GOTO_DESTROY("sem_init", client_endpoint)
					enum Connection_State *connection_state = bullshitcore_memory_retrieve(sizeof *connection_state); // free me
					if (unlikely(!connection_state))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					*connection_state = Connection_State_Handshaking;
					*thread_arguments = (struct ThreadArguments)
					{
						client_endpoint,
						interthread_buffer,
						interthread_buffer_length,
						interthread_buffer_mutex,
						interthread_buffer_condition,
						client_thread_arguments_semaphore,
						connection_state
					};
					pthread_t packet_sender_thread;
					ret = pthread_create(&packet_sender_thread, &thread_attributes, packet_sender, thread_arguments);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
					}
					((struct ThreadArguments *)thread_arguments)->sender_thread = packet_sender_thread;
					pthread_t packet_receiver_thread;
					ret = pthread_create(&packet_receiver_thread, &thread_attributes, packet_receiver, thread_arguments);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
					}
					if (unlikely(sem_wait(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
					if (unlikely(sem_wait(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
					if (unlikely(sem_destroy(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_destroy", client_endpoint)
					bullshitcore_memory_leave(client_thread_arguments_semaphore, sizeof *client_thread_arguments_semaphore);
					bullshitcore_memory_leave(thread_arguments, sizeof *thread_arguments);
				}
			}
			if (unlikely(close(client_endpoint) == -1))
				PERROR_AND_GOTO_DESTROY("close", server_endpoint)
			if (unlikely(close(server_endpoint) == -1)) PERROR_AND_EXIT("close")
			ret = pthread_attr_destroy(&thread_attributes);
			if (unlikely(ret))
			{
				errno = ret;
				PERROR_AND_GOTO_DESTROY("pthread_attr_destroy", client_endpoint)
			}
			return EXIT_SUCCESS;
destroy_client_endpoint:
			if (unlikely(close(client_endpoint) == -1)) perror("close");
		}
destroy_server_endpoint:
		if (unlikely(close(server_endpoint) == -1)) perror("close");
	}
destroy_thread_attributes:
	ret = pthread_attr_destroy(&thread_attributes);
	if (unlikely(ret))
	{
		errno = ret;
		perror("pthread_attr_destroy");
	}
	return EXIT_FAILURE;
}
