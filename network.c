#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "global_macros.h"
#include "network.h"

#define MINECRAFT_VERSION "1.20.5"
#define PROTOCOL_VERSION 766
#define PERROR_AND_GOTO_CLOSEFD(s, ctx) { perror(s); goto ctx ## closefd; }

// config
#define ADDRESS "0.0.0.0"
#define PORT 25565
#define MAX_CONNECTIONS 15 // cannot be larger than 32767

VarInt varint_encode(int32_t value)
{
	return varlong_encode((int64_t)value);
}

int32_t varint_decode(VarInt varint)
{
	int32_t value = 0;
	for (size_t i = 0; i < 5; ++i)
	{
		const int8_t varint_byte = varint[i];
		value |= (varint_byte & 0x7F) << i * 7;
		if (!(varint_byte & 0x80)) break;
	}
	return value;
}

VarLong varlong_encode(int64_t value)
{
	uint_fast8_t bytes = 1;
	while (value >> 7 * bytes) bytes += !!(value >> 7);
	VarLong varlong = calloc(bytes, 1);
	if (unlikely(!varlong)) return NULL;
	for (size_t i = 0; i < bytes; ++i)
		varlong[i] = value >> 7 * i & 0x7F | (i != bytes - 1u) << 7;
	return varlong;
}

int64_t varlong_decode(VarLong varlong)
{
	int64_t value = 0;
	for (size_t i = 0; i < 10; ++i)
	{
		const int8_t varlong_byte = varlong[i];
		value |= (varlong_byte & 0x7F) << 7 * i;
		if (!(varlong_byte & 0x80)) break;
	}
	return value;
}

static void *main_routine(void *p_client_endpoint)
{
	int client_endpoint = *(int *)p_client_endpoint;
	free(p_client_endpoint);
	Boolean compression_enabled = false;
	enum State current_state;
	Byte buffer[128];
	// int bytes_read = recv(client_endpoint, &buffer, sizeof buffer, 0);
	// if (unlikely(bytes_read == -1)) return (void *)1;
	return NULL;
}

// TODO: Add support for IPv6
int main(void)
{
	int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (unlikely(server_endpoint == -1)) PERROR_AND_EXIT("socket")
	if (unlikely(setsockopt(server_endpoint, IPPROTO_TCP, TCP_NODELAY, &(const int){ 1 }, sizeof(int)) == -1))
		PERROR_AND_GOTO_CLOSEFD("setsockopt", server)
	{
		struct in_addr address;
		{
			int ret = inet_pton(AF_INET, ADDRESS, &address);
			if (!ret)
			{
				fputs("An invalid server address was specified.\n", stderr);
				return EXIT_FAILURE;
			}
			else if (unlikely(ret == -1))
				PERROR_AND_GOTO_CLOSEFD("inet_pton", server)
		}
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
			int_least16_t thread_counter = 0;
			while (1)
			{
				client_endpoint = accept(server_endpoint, NULL, NULL);
				if (unlikely(client_endpoint == -1))
					PERROR_AND_GOTO_CLOSEFD("accept", server)
				if (thread_counter == MAX_CONNECTIONS)
				{
					if (unlikely(close(client_endpoint) == -1))
						PERROR_AND_GOTO_CLOSEFD("close", server)
					continue;
				}
				int * const p_client_endpoint = malloc(sizeof client_endpoint);
				if (unlikely(!p_client_endpoint))
					PERROR_AND_GOTO_CLOSEFD("malloc", client)
				*p_client_endpoint = client_endpoint;
				pthread_t thread;
				pthread_create(&thread, NULL, main_routine, p_client_endpoint);
				++thread_counter;
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
