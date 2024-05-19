#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
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
#define PERROR_AND_GOTO_CLOSEFD(s, ctx) { perror(s); goto ctx ## closefd; }
#define THREAD_STACK_SIZE 8388608
#define PACKET_MAXSIZE 2097151

// config
#define ADDRESS "0.0.0.0"
#define PORT 25565
#define MAX_CONNECTIONS 15

VarInt
bullshitcore_network_varint_encode(int32_t value)
{
	return bullshitcore_network_varlong_encode(value);
}

int32_t
bullshitcore_network_varint_decode(VarInt varint, size_t * restrict bytes)
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
bullshitcore_network_varlong_decode(VarLong varlong, size_t * restrict bytes)
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

static void *
main_routine(void *p_client_endpoint)
{
	{
		const int client_endpoint = *(int *)p_client_endpoint;
		free(p_client_endpoint);
		enum State current_state = State_Handshaking;
		Boolean compression_enabled = false;
		int8_t buffer[PACKET_MAXSIZE];
		if (unlikely(recv(client_endpoint, buffer, PACKET_MAXSIZE, 0) == -1))
			goto clear_stack;
		size_t buffer_offset = 0;
		size_t packet_next_boundary;
		int32_t packet_length = bullshitcore_network_varint_decode(buffer, &packet_next_boundary);
		buffer_offset += packet_next_boundary;
		int32_t packet_identifier = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
		buffer_offset += packet_next_boundary;
		switch (current_state)
		{
			case State_Handshaking:
			{
				switch (packet_identifier)
				{
					case HANDSHAKE_PACKET:
					{
						int32_t client_protocol_version = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
						buffer_offset += packet_next_boundary;
						int32_t server_address_string_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
						buffer_offset += packet_next_boundary;
						int32_t target_state = bullshitcore_network_varint_decode(buffer + buffer_offset + server_address_string_length + 2, &packet_next_boundary);
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
						break;
					}
					case PINGREQUEST_PACKET:
					{
						break;
					}
				}
				break;
			}
		}
	}
	return NULL;
clear_stack:;
	int my_errno = errno;
	int *p_my_errno = malloc(sizeof my_errno);
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
		PERROR_AND_EXIT("pthread_attr_setstacksize")
	}
	const int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (unlikely(server_endpoint == -1)) PERROR_AND_EXIT("socket")
	if (unlikely(setsockopt(server_endpoint, IPPROTO_TCP, TCP_NODELAY, &(const int){ 1 }, sizeof(int)) == -1))
		PERROR_AND_GOTO_CLOSEFD("setsockopt", server)
	{
		struct in_addr address;
		ret = inet_pton(AF_INET, ADDRESS, &address);
		if (!ret)
		{
			fputs("An invalid server address was specified.\n", stderr);
			return EXIT_FAILURE;
		}
		else if (unlikely(ret == -1))
			PERROR_AND_GOTO_CLOSEFD("inet_pton", server)
		const struct sockaddr_in server_address = { AF_INET, htons(PORT), address };
		struct sockaddr server_address_data;
		memcpy(&server_address_data, &server_address, sizeof server_address_data);
		if (unlikely(bind(server_endpoint, &server_address_data, sizeof server_address_data) == -1))
			PERROR_AND_GOTO_CLOSEFD("bind", server)
	}
	if (unlikely(listen(server_endpoint, SOMAXCONN) == -1))
		PERROR_AND_GOTO_CLOSEFD("listen", server)
	{
		int client_endpoint;
		{
			while (1)
			{
				client_endpoint = accept(server_endpoint, NULL, NULL);
				if (unlikely(client_endpoint == -1))
					PERROR_AND_GOTO_CLOSEFD("accept", server)
				bullshitcore_log_log("Connection is established!");
				int * const p_client_endpoint = malloc(sizeof client_endpoint);
				if (unlikely(!p_client_endpoint))
					PERROR_AND_GOTO_CLOSEFD("malloc", client)
				*p_client_endpoint = client_endpoint;
				pthread_t thread;
				ret = pthread_create(&thread, &thread_attributes, main_routine, p_client_endpoint);
				if (unlikely(ret))
				{
					errno = ret;
					PERROR_AND_GOTO_CLOSEFD("pthread_create", client)
				}
			}
			ret = pthread_attr_destroy(&thread_attributes);
			if (unlikely(ret))
			{
				errno = ret;
				PERROR_AND_GOTO_CLOSEFD("pthread_attr_destroy", client)
			}
		}
		if (unlikely(close(client_endpoint) == -1))
			PERROR_AND_GOTO_CLOSEFD("close", server)
		if (unlikely(close(server_endpoint) == -1)) PERROR_AND_EXIT("close")
		return EXIT_SUCCESS;
clientclosefd:
		if (unlikely(close(client_endpoint) == -1)) perror("close");
	}
serverclosefd:
	if (unlikely(close(server_endpoint) == -1)) perror("close");
	return EXIT_FAILURE;
}
