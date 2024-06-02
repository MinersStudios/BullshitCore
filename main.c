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
#include <unistd.h>
#include "global_macros.h"
#include "log.h"
#include "network.h"

#define MINECRAFT_VERSION "1.20.4"
#define PROTOCOL_VERSION 765
#define PROTOCOL_VERSION_STRING EXPAND_AND_STRINGIFY(PROTOCOL_VERSION)
#define PERROR_AND_GOTO_DESTROY(s, object) { perror(s); goto destroy_ ## object; }
#define THREAD_STACK_SIZE 8388608
#define PACKET_MAXSIZE 2097151

// config
#define ADDRESS "0.0.0.0"
#define PORT 25565
#define MAX_CONNECTIONS 15
#define FAVICON

typedef struct
{
	int *p_client_endpoint;
	sem_t *p_client_endpoint_semaphore;
} ThreadArguments;

static void *
packet_receiver(void * restrict thread_arguments)
{
	{
		const int client_endpoint = *((ThreadArguments*)thread_arguments)->p_client_endpoint;
		if (unlikely(sem_post(((ThreadArguments*)thread_arguments)->p_client_endpoint_semaphore) == -1))
			goto clear_stack_receiver;
		enum State current_state = State_Handshaking;
		Boolean compression_enabled = false;
		uint8_t buffer[PACKET_MAXSIZE];
		size_t buffer_offset, packet_next_boundary;
		while (1)
		{
			const ssize_t bytes_read = recv(client_endpoint, buffer, PACKET_MAXSIZE, 0);
			if (!bytes_read) return NULL;
			else if (unlikely(bytes_read == -1)) goto clear_stack_receiver;
			{
				const int32_t packet_length = bullshitcore_network_varint_decode(buffer, &packet_next_boundary);
			}
			buffer_offset = packet_next_boundary;
			const int32_t packet_identifier = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
			buffer_offset += packet_next_boundary;
			switch (current_state)
			{
				case State_Handshaking:
				{
					switch (packet_identifier)
					{
						case HANDSHAKE_PACKET:
						{
							const int32_t client_protocol_version = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							const int32_t server_address_string_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							const int32_t target_state = bullshitcore_network_varint_decode(buffer + buffer_offset + server_address_string_length + 2, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
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
						case STATUSREQUEST_PACKET:
						{
							size_t mybuffer_offset = 0;
							const char * const text = "{\"version\":{\"name\":\"" MINECRAFT_VERSION "\",\"protocol\":" PROTOCOL_VERSION_STRING "},\"description\":{\"text\":\"BullshitCore is up and running!\",\"favicon\":\"" FAVICON "\"}}";
							const size_t text_length = strlen(text);
							const JSONTextComponent packet_payload =
							{
								bullshitcore_network_varint_encode(text_length),
								text
							};
							if (text_length > JSONTEXTCOMPONENT_MAXSIZE) // shall probably do something about it
								break;
							size_t packet_payload_length_length;
							bullshitcore_network_varint_decode(packet_payload.length, &packet_payload_length_length);
							const size_t packet_length = 1 + packet_payload_length_length + text_length;
							const VarInt packet_length_varint = bullshitcore_network_varint_encode(packet_length);
							size_t packet_length_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_length);
							uint8_t * const packet = malloc(packet_length_length + packet_length);
							if (unlikely(!packet)) goto clear_stack_receiver;
							memcpy(packet, packet_length_varint, packet_length_length);
							mybuffer_offset += packet_length_length;
							packet[mybuffer_offset] = STATUSREQUEST_PACKET;
							++mybuffer_offset;
							memcpy(packet + mybuffer_offset, packet_payload.length, packet_payload_length_length);
							mybuffer_offset += packet_length_length;
							memcpy(packet + mybuffer_offset, packet_payload.contents, text_length);
							if (unlikely(send(client_endpoint, packet, packet_length_length + packet_length, 0) == -1))
							{
								free(packet);
								goto clear_stack_receiver;
							}
							free(packet);
							break;
						}
						case PINGREQUEST_PACKET:
						{
							uint8_t * const packet = malloc(6);
							if (unlikely(!packet)) goto clear_stack_receiver;
							memcpy(packet, buffer, 6);
							if (unlikely(send(client_endpoint, packet, 6, 0) == -1))
							{
								free(packet);
								goto clear_stack_receiver;
							}
							free(packet);
							goto get_closed_receiver;
							break;
						}
					}
					break;
				}
				case State_Login:
				{
					break;
				}
				case State_Configuration:
				{
					break;
				}
				case State_Play:
				{
					break;
				}
			}
			bullshitcore_log_log("looped");
		}
	}
get_closed_receiver:
	return NULL;
clear_stack_receiver:;
	const int my_errno = errno;
	int * const p_my_errno = malloc(sizeof my_errno);
	if (unlikely(!p_my_errno)) return (void *)1;
	*p_my_errno = my_errno;
	return p_my_errno;
}

static void *
packet_sender(void * restrict thread_arguments)
{
	{
		const int client_endpoint = *((ThreadArguments*)thread_arguments)->p_client_endpoint;
		if (unlikely(sem_post(((ThreadArguments*)thread_arguments)->p_client_endpoint_semaphore) == -1))
			goto get_closed_sender;
	}
get_closed_sender:
	return NULL;
clear_stack_sender:;
	const int my_errno = errno;
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
	sem_t *p_client_endpoint_semaphore = malloc(sizeof *p_client_endpoint_semaphore);
	if (unlikely(!p_client_endpoint_semaphore))
		PERROR_AND_GOTO_DESTROY("malloc", thread_attributes)
	if (unlikely(sem_init(p_client_endpoint_semaphore, 0, 0) == -1))
		PERROR_AND_GOTO_DESTROY("sem_init", thread_attributes)
	const int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (unlikely(server_endpoint == -1)) PERROR_AND_EXIT("socket")
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
				int * const p_client_endpoint = malloc(sizeof client_endpoint);
				if (unlikely(!p_client_endpoint))
					PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
				*p_client_endpoint = client_endpoint;
				void *thread_arguments = malloc(sizeof p_client_endpoint + sizeof p_client_endpoint_semaphore);
				if (unlikely(!thread_arguments))
					PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
				*(ThreadArguments*)thread_arguments = (ThreadArguments){ p_client_endpoint, p_client_endpoint_semaphore };
				pthread_t packet_receiver_thread;
				ret = pthread_create(&packet_receiver_thread, &thread_attributes, packet_receiver, thread_arguments);
				if (unlikely(ret))
				{
					errno = ret;
					PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
				}
				pthread_t packet_sender_thread;
				ret = pthread_create(&packet_sender_thread, &thread_attributes, packet_sender, thread_arguments);
				if (unlikely(ret))
				{
					errno = ret;
					PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
				}
				if (unlikely(sem_wait(p_client_endpoint_semaphore) == -1))
					PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
				if (unlikely(sem_wait(p_client_endpoint_semaphore) == -1))
					PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
				free(p_client_endpoint);
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
destroy_thread_attributes:
	ret = pthread_attr_destroy(&thread_attributes);
	if (unlikely(ret))
	{
		errno = ret;
		perror("pthread_attr_destroy");
	}
	return EXIT_FAILURE;
}
