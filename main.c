#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wolfssl/wolfcrypt/hash.h>
#include "global_macros.h"
#include "log.h"
#include "network.h"
#include "world.h"

#define MINECRAFT_VERSION "1.21"
#define PROTOCOL_VERSION 767
#define PERROR_AND_GOTO_DESTROY(s, object) { perror(s); goto destroy_ ## object; }
#define THREAD_STACK_SIZE 8388608
#define PACKET_MAXSIZE 2097151

// config
#define ADDRESS "0.0.0.0"
#define FAVICON
#define MAX_PLAYERS 15
#define PORT 25565
#define RENDER_DISTANCE 2

struct ThreadArguments
{
	int client_endpoint;
	uint8_t *interthread_buffer;
	size_t *interthread_buffer_length;
	pthread_mutex_t *interthread_buffer_mutex;
	pthread_cond_t *interthread_buffer_condition;
	sem_t *client_thread_arguments_semaphore;
};

static void *
packet_receiver(void *thread_arguments)
{
	{
		const int client_endpoint = ((struct ThreadArguments*)thread_arguments)->client_endpoint;
		uint8_t * const interthread_buffer = ((struct ThreadArguments*)thread_arguments)->interthread_buffer;
		size_t * const interthread_buffer_length = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_length;
		pthread_mutex_t * const interthread_buffer_mutex = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_mutex;
		pthread_cond_t * const interthread_buffer_condition = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_condition;
		if (unlikely(sem_post(((struct ThreadArguments*)thread_arguments)->client_thread_arguments_semaphore) == -1))
			goto clear_stack_receiver;
		enum State current_state = State_Handshaking;
		Boolean compression_enabled = false;
		int8_t buffer[PACKET_MAXSIZE];
		uint8_t packet_next_boundary;
		size_t buffer_offset;
		int ret;
		while (1)
		{
			const ssize_t bytes_read = recv(client_endpoint, buffer, PACKET_MAXSIZE, 0);
			if (!bytes_read) return NULL;
			else if (unlikely(bytes_read == -1)) goto clear_stack_receiver;
			bullshitcore_network_varint_decode(buffer, &packet_next_boundary);
			buffer_offset = packet_next_boundary;
			const uint32_t packet_identifier = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
			buffer_offset += packet_next_boundary;
			switch (current_state)
			{
				case State_Handshaking:
				{
					switch (packet_identifier)
					{
						case Packet_Handshake:
						{
							const uint32_t client_protocol_version = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							if (client_protocol_version != PROTOCOL_VERSION)
								bullshitcore_log_log("Warning! Client and server protocol version mismatch.");
							const uint32_t server_address_string_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							const uint32_t target_state = bullshitcore_network_varint_decode(buffer + buffer_offset + server_address_string_length + 2, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							if (target_state >= State_Status && target_state <= State_Transfer)
								current_state = target_state;
							break;
						}
					}
					break;
				}
				case State_Status:
				{
					switch (packet_identifier)
					{
						case Packet_Status_Client_Request:
						{
							// TODO: Sanitise input (implement it after testing)
							const uint8_t * const text = (const uint8_t *)"{\"version\":{\"name\":\"" MINECRAFT_VERSION "\",\"protocol\":" EXPAND_AND_STRINGIFY(PROTOCOL_VERSION) "},\"players\":{\"max\":" EXPAND_AND_STRINGIFY(MAX_PLAYERS) ",\"online\":0},\"description\":{\"text\":\"BullshitCore is up and running!\",\"favicon\":\"" FAVICON "\"}}";
							const size_t text_length = strlen((const char *)text);
							const JSONTextComponent packet_payload =
							{
								bullshitcore_network_varint_encode(text_length),
								text
							};
							assert(text_length <= JSONTEXTCOMPONENT_MAXSIZE);
							uint8_t packet_payload_length_length;
							bullshitcore_network_varint_decode(packet_payload.length, &packet_payload_length_length);
							const size_t packet_length = 1 + packet_payload_length_length + text_length;
							const VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_length);
							uint8_t packet_length_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_length);
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							*interthread_buffer_length = packet_length_length + packet_length;
							memcpy(interthread_buffer, packet_length_varint, packet_length_length);
							size_t interthread_buffer_offset = packet_length_length;
							interthread_buffer[interthread_buffer_offset] = Packet_Status_Server_Response;
							++interthread_buffer_offset;
							memcpy(interthread_buffer + interthread_buffer_offset, packet_payload.length, packet_payload_length_length);
							interthread_buffer_offset += packet_length_length;
							memcpy(interthread_buffer + interthread_buffer_offset, packet_payload.contents, text_length);
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							break;
						}
						case Packet_Status_Client_Ping_Request:
						{
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							*interthread_buffer_length = 6;
							memcpy(interthread_buffer, buffer, 6);
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							goto get_closed_receiver;
							break;
						}
					}
					break;
				}
				case State_Login:
				{
					switch (packet_identifier)
					{
						case Packet_Login_Client_Login_Start:
						{
							uint8_t username_length_length;
							const uint32_t username_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &username_length_length);
							buffer_offset += username_length_length + username_length;
							const uint32_t packet_length = username_length_length + username_length + 19;
							const VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_length);
							uint8_t packet_length_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_length);
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							*interthread_buffer_length = username_length_length + username_length + packet_length_length + 19;
							memcpy(interthread_buffer, packet_length_varint, packet_length_length);
							size_t interthread_buffer_offset = packet_length_length;
							interthread_buffer[interthread_buffer_offset] = Packet_Login_Server_Login_Success;
							++interthread_buffer_offset;
							memcpy(interthread_buffer + interthread_buffer_offset, buffer + buffer_offset, 16);
							interthread_buffer_offset += 16;
							memcpy(interthread_buffer + interthread_buffer_offset, buffer + buffer_offset - username_length_length - username_length, username_length_length + username_length);
							interthread_buffer_offset += username_length_length + username_length;
							interthread_buffer[interthread_buffer_offset] = 0;
							++interthread_buffer_offset;
							interthread_buffer[interthread_buffer_offset] = true;
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
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
							current_state = State_Configuration;
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							interthread_buffer[1] = Packet_Configuration_Server_Known_Packs;
							size_t interthread_buffer_offset = 2;
							const VarInt * const known_packs_count_varint = bullshitcore_network_varint_encode(1);
							uint8_t known_packs_count_varint_length;
							bullshitcore_network_varint_decode(known_packs_count_varint, &known_packs_count_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, known_packs_count_varint, known_packs_count_varint_length);
							interthread_buffer_offset += known_packs_count_varint_length;
							const String namespace = { bullshitcore_network_varint_encode(strlen("minecraft")), (const uint8_t *)"minecraft" };
							const String identifier = { bullshitcore_network_varint_encode(strlen("core")), (const uint8_t *)"core" };
							const String version = { bullshitcore_network_varint_encode(strlen("1.21")), (const uint8_t *)"1.21" };
							uint8_t namespace_length_varint_length;
							bullshitcore_network_varint_decode(namespace.length, &namespace_length_varint_length);
							uint8_t identifier_length_varint_length;
							bullshitcore_network_varint_decode(identifier.length, &identifier_length_varint_length);
							uint8_t version_length_varint_length;
							bullshitcore_network_varint_decode(version.length, &version_length_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, namespace.length, namespace_length_varint_length);
							interthread_buffer_offset += namespace_length_varint_length;
							memcpy(interthread_buffer + interthread_buffer_offset, namespace.contents, strlen((const char *)namespace.contents));
							interthread_buffer_offset += strlen((const char *)namespace.contents);
							memcpy(interthread_buffer + interthread_buffer_offset, identifier.length, identifier_length_varint_length);
							interthread_buffer_offset += identifier_length_varint_length;
							memcpy(interthread_buffer + interthread_buffer_offset, identifier.contents, strlen((const char *)identifier.contents));
							interthread_buffer_offset += strlen((const char *)identifier.contents);
							memcpy(interthread_buffer + interthread_buffer_offset, version.length, version_length_varint_length);
							interthread_buffer_offset += version_length_varint_length;
							memcpy(interthread_buffer + interthread_buffer_offset, version.contents, strlen((const char *)version.contents));
							interthread_buffer_offset += strlen((const char *)version.contents);
							interthread_buffer[0] = interthread_buffer_offset - 1;
							*interthread_buffer_length = interthread_buffer_offset;
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							break;
						}
						case Packet_Login_Client_Cookie_Response:
						{
							break;
						}
					}
					break;
				}
				case State_Transfer:
				{
					break;
				}
				case State_Configuration:
				{
					switch (packet_identifier)
					{
						case Packet_Configuration_Client_Client_Information:
						{
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
							current_state = State_Play;
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							interthread_buffer[1] = Packet_Play_Server_Login;
							size_t interthread_buffer_offset = 2;
							memcpy(interthread_buffer + interthread_buffer_offset, &(const int32_t){ 0 }, sizeof(uint32_t));
							interthread_buffer_offset += sizeof(uint32_t);
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ false }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							const uint8_t *overworld = (const uint8_t *)"minecraft:overworld";
							const uint8_t *nether = (const uint8_t *)"minecraft:the_nether";
							const uint8_t *end = (const uint8_t *)"minecraft:the_end";
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
							const VarInt * const dimension_count_varint = bullshitcore_network_varint_encode(NUMOF(dimensions));
							uint8_t dimension_count_varint_length;
							bullshitcore_network_varint_decode(dimension_count_varint, &dimension_count_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, dimension_count_varint, dimension_count_varint_length);
							interthread_buffer_offset += dimension_count_varint_length;
							uint8_t dimension_length_length;
							uint8_t dimension_length;
							for (size_t dimension = 0; dimension < NUMOF(dimensions); ++dimension)
							{
								bullshitcore_network_varint_decode(dimensions[dimension].length, &dimension_length_length);
								memcpy(interthread_buffer + interthread_buffer_offset, *(const VarInt **)(dimensions + dimension), dimension_length_length);
								interthread_buffer_offset += dimension_length_length;
								dimension_length = strlen((const char *)dimensions[dimension].contents);
								memcpy(interthread_buffer + interthread_buffer_offset, *(const uint8_t **)((const uint8_t *)(dimensions + dimension) + offsetof(Identifier, contents)), dimension_length);
								interthread_buffer_offset += dimension_length;
							}
							const VarInt * const max_players_varint = bullshitcore_network_varint_encode(MAX_PLAYERS);
							uint8_t max_players_varint_length;
							bullshitcore_network_varint_decode(max_players_varint, &max_players_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, max_players_varint, max_players_varint_length);
							interthread_buffer_offset += max_players_varint_length;
							const VarInt * const render_distance_varint = bullshitcore_network_varint_encode(RENDER_DISTANCE);
							uint8_t render_distance_varint_length;
							bullshitcore_network_varint_decode(render_distance_varint, &render_distance_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, render_distance_varint, render_distance_varint_length);
							interthread_buffer_offset += render_distance_varint_length;
							memcpy(interthread_buffer + interthread_buffer_offset, render_distance_varint, render_distance_varint_length);
							interthread_buffer_offset += render_distance_varint_length;
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ false }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ true }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ false }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							const VarInt * const dimension_type_varint = bullshitcore_network_varint_encode(0);
							uint8_t dimension_type_varint_length;
							bullshitcore_network_varint_decode(dimension_type_varint, &dimension_type_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, dimension_type_varint, dimension_type_varint_length);
							interthread_buffer_offset += dimension_type_varint_length;
							bullshitcore_network_varint_decode(dimensions[0].length, &dimension_length_length);
							memcpy(interthread_buffer + interthread_buffer_offset, *(const VarInt **)dimensions, dimension_length_length);
							interthread_buffer_offset += dimension_length_length;
							dimension_length = strlen((const char *)dimensions[0].contents);
							memcpy(interthread_buffer + interthread_buffer_offset, *(const uint8_t **)((const uint8_t *)dimensions + offsetof(Identifier, contents)), dimension_length);
							interthread_buffer_offset += dimension_length;
							int64_t seed_hash = -1916599016670012116;
							if (unlikely(wc_Sha256Hash((const byte *)&seed_hash, sizeof seed_hash, (byte *)&seed_hash)))
							{} // should not fail
							memcpy(interthread_buffer + interthread_buffer_offset, &seed_hash, sizeof seed_hash);
							interthread_buffer_offset += sizeof seed_hash;
							memcpy(interthread_buffer + interthread_buffer_offset, &(const uint8_t){ 3 }, sizeof(uint8_t));
							interthread_buffer_offset += sizeof(uint8_t);
							memcpy(interthread_buffer + interthread_buffer_offset, &(const int8_t){ -1 }, sizeof(int8_t));
							interthread_buffer_offset += sizeof(int8_t);
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ false }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ false }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ false }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							const VarInt * const portal_cooldown_varint = bullshitcore_network_varint_encode(0);
							uint8_t portal_cooldown_varint_length;
							bullshitcore_network_varint_decode(portal_cooldown_varint, &portal_cooldown_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, portal_cooldown_varint, portal_cooldown_varint_length);
							interthread_buffer_offset += portal_cooldown_varint_length;
							memcpy(interthread_buffer + interthread_buffer_offset, &(const Boolean){ false }, sizeof(Boolean));
							interthread_buffer_offset += sizeof(Boolean);
							interthread_buffer[0] = interthread_buffer_offset - 1;
							*interthread_buffer_length = interthread_buffer_offset;
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							*interthread_buffer_length = 7;
							interthread_buffer[0] = 6;
							interthread_buffer[1] = Packet_Play_Server_Game_Event;
							interthread_buffer[2] = (uint8_t)13;
							memcpy(interthread_buffer + 3, &(float){ 0 }, sizeof(float) >= 4 ? 4 : sizeof(float));
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							interthread_buffer[1] = Packet_Play_Server_Synchronise_Player_Position;
							interthread_buffer_offset = 2;
							memcpy(interthread_buffer + interthread_buffer_offset, &(double){ 0 }, sizeof(double) >= 8 ? 8 : sizeof(double));
							interthread_buffer_offset += sizeof(double) >= 8 ? 8 : sizeof(double);
							memcpy(interthread_buffer + interthread_buffer_offset, &(double){ 128 }, sizeof(double) >= 8 ? 8 : sizeof(double));
							interthread_buffer_offset += sizeof(double) >= 8 ? 8 : sizeof(double);
							memcpy(interthread_buffer + interthread_buffer_offset, &(double){ 0 }, sizeof(double) >= 8 ? 8 : sizeof(double));
							interthread_buffer_offset += sizeof(double) >= 8 ? 8 : sizeof(double);
							memcpy(interthread_buffer + interthread_buffer_offset, &(float){ 0 }, sizeof(float) >= 8 ? 8 : sizeof(float));
							interthread_buffer_offset += sizeof(float) >= 8 ? 8 : sizeof(float);
							memcpy(interthread_buffer + interthread_buffer_offset, &(float){ 0 }, sizeof(float) >= 8 ? 8 : sizeof(float));
							interthread_buffer_offset += sizeof(float) >= 8 ? 8 : sizeof(float);
							interthread_buffer[interthread_buffer_offset] = (int8_t)0;
							++interthread_buffer_offset;
							const VarInt * const teleportation_identifier_varint = bullshitcore_network_varint_encode(0);
							uint8_t teleportation_identifier_varint_length;
							bullshitcore_network_varint_decode(teleportation_identifier_varint, &teleportation_identifier_varint_length);
							memcpy(interthread_buffer + interthread_buffer_offset, teleportation_identifier_varint, teleportation_identifier_varint_length);
							interthread_buffer_offset += teleportation_identifier_varint_length;
							interthread_buffer[0] = interthread_buffer_offset - 1;
							*interthread_buffer_length = interthread_buffer_offset;
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
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
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							*interthread_buffer_length = 3;
							interthread_buffer[0] = 2;
							interthread_buffer[1] = Packet_Configuration_Server_Registry_Data;
							interthread_buffer[2] = 0;
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							*interthread_buffer_length = 2;
							interthread_buffer[0] = 1;
							interthread_buffer[1] = Packet_Configuration_Server_Finish_Configuration;
							ret = pthread_cond_signal(interthread_buffer_condition);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							ret = pthread_mutex_unlock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							break;
						}
					}
					break;
				}
				case State_Play:
				{
					break;
				}
			}
#ifndef NDEBUG
			bullshitcore_log_log("Reached an end of the packet jump table.");
#endif
		}
	}
get_closed_receiver:
	return NULL;
clear_stack_receiver:;
	// TODO: Also pass failed function name
	const int my_errno = errno;
#ifndef NDEBUG
	bullshitcore_log_logf("Receiver thread crashed! %s\n", strerror(my_errno));
#endif
	int * const p_my_errno = malloc(sizeof my_errno);
	if (unlikely(!p_my_errno)) return (void *)1;
	*p_my_errno = my_errno;
	return p_my_errno;
}

static void *
packet_sender(void *thread_arguments)
{
	{
		const int client_endpoint = ((struct ThreadArguments*)thread_arguments)->client_endpoint;
		uint8_t * const interthread_buffer = ((struct ThreadArguments*)thread_arguments)->interthread_buffer;
		size_t * const interthread_buffer_length = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_length;
		pthread_mutex_t * const interthread_buffer_mutex = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_mutex;
		pthread_cond_t * const interthread_buffer_condition = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_condition;
		if (unlikely(sem_post(((struct ThreadArguments*)thread_arguments)->client_thread_arguments_semaphore) == -1))
			goto clear_stack_sender;
		while (1)
		{
			int ret = pthread_mutex_lock(interthread_buffer_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
			ret = pthread_cond_wait(interthread_buffer_condition, interthread_buffer_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
			if (unlikely(send(client_endpoint, interthread_buffer, *interthread_buffer_length, 0) == -1))
				goto clear_stack_sender;
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
	bullshitcore_log_logf("Sender thread crashed! %s\n", strerror(my_errno));
#endif
	int * const p_my_errno = malloc(sizeof my_errno);
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
		const int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (unlikely(server_endpoint == -1))
			PERROR_AND_GOTO_DESTROY("socket", thread_attributes)
		if (unlikely(setsockopt(server_endpoint, IPPROTO_TCP, TCP_NODELAY, &(const int){ 1 }, sizeof(int)) == -1))
			PERROR_AND_GOTO_DESTROY("setsockopt", server_endpoint)
		{
			struct in_addr address;
			ret = inet_pton(AF_INET, ADDRESS, &address);
			if (!ret)
			{
				fputs("An invalid server address was specified.\n", stderr);
				return EXIT_FAILURE;
			}
			else if (unlikely(ret == -1))
				PERROR_AND_GOTO_DESTROY("inet_pton", server_endpoint)
			const struct sockaddr_in server_address = { AF_INET, htons(PORT), address };
			struct sockaddr server_address_data;
			memcpy(&server_address_data, &server_address, sizeof server_address_data);
			if (unlikely(bind(server_endpoint, &server_address_data, sizeof server_address_data) == -1))
				PERROR_AND_GOTO_DESTROY("bind", server_endpoint)
		}
		if (unlikely(listen(server_endpoint, SOMAXCONN) == -1))
			PERROR_AND_GOTO_DESTROY("listen", server_endpoint)
		{
			int client_endpoint;
			{
				while (1)
				{
					client_endpoint = accept(server_endpoint, NULL, NULL);
					if (unlikely(client_endpoint == -1))
						PERROR_AND_GOTO_DESTROY("accept", server_endpoint)
					bullshitcore_log_log("Connection is established!");
					struct ThreadArguments *thread_arguments = malloc(sizeof *thread_arguments);
					if (unlikely(!thread_arguments))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					uint8_t *interthread_buffer = malloc(sizeof *interthread_buffer * PACKET_MAXSIZE); // free me
					if (unlikely(!interthread_buffer))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					size_t *interthread_buffer_length = malloc(sizeof *interthread_buffer_length); // free me
					if (unlikely(!interthread_buffer_length))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					pthread_mutex_t *interthread_buffer_mutex = malloc(sizeof *interthread_buffer_mutex); // free me
					if (unlikely(!interthread_buffer_mutex))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					ret = pthread_mutex_init(interthread_buffer_mutex, NULL);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_mutex_init", client_endpoint)
					}
					pthread_cond_t *interthread_buffer_condition = malloc(sizeof *interthread_buffer_condition); // free me
					if (unlikely(!interthread_buffer_condition))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					ret = pthread_cond_init(interthread_buffer_condition, NULL);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_cond_init", client_endpoint)
					}
					sem_t *client_thread_arguments_semaphore = malloc(sizeof *client_thread_arguments_semaphore);
					if (unlikely(!client_thread_arguments_semaphore))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					if (unlikely(sem_init(client_thread_arguments_semaphore, 0, 0) == -1))
						PERROR_AND_GOTO_DESTROY("sem_init", thread_attributes)
					*thread_arguments = (struct ThreadArguments){ client_endpoint, interthread_buffer, interthread_buffer_length, interthread_buffer_mutex, interthread_buffer_condition, client_thread_arguments_semaphore };
					{
						pthread_t packet_receiver_thread;
						ret = pthread_create(&packet_receiver_thread, &thread_attributes, packet_receiver, thread_arguments);
						if (unlikely(ret))
						{
							errno = ret;
							PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
						}
					}
					{
						pthread_t packet_sender_thread;
						ret = pthread_create(&packet_sender_thread, &thread_attributes, packet_sender, thread_arguments);
						if (unlikely(ret))
						{
							errno = ret;
							PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
						}
					}
					if (unlikely(sem_wait(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
					if (unlikely(sem_wait(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
					if (unlikely(sem_destroy(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_destroy", client_endpoint)
					free(client_thread_arguments_semaphore);
					free(thread_arguments);
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
