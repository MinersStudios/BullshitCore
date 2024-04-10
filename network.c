#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "global_macros.h"
#include "network.h"

#define ADDRESS INADDR_ANY
#define PORT 25565
#define PERROR_AND_GOTO_CLOSEFD(s, ctx) { perror(s); goto ctx ## closefd; }
#define MINECRAFT_VERSION "1.20.4"
#define PROTOCOL_VERSION 765

// TODO: Add support for IPv6
int main(void)
{
	int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (unlikely(server_endpoint == -1)) PERROR_AND_EXIT("socket")
	if (unlikely(setsockopt(server_endpoint, IPPROTO_TCP, TCP_NODELAY, &(const int){ 1 }, sizeof(int)) == -1))
		PERROR_AND_GOTO_CLOSEFD("setsockopt", server)
	{
		const struct sockaddr_in server_address = { AF_INET, htons(PORT), (struct in_addr){ htonl(ADDRESS) } };
		struct sockaddr server_address_data;
		memcpy(&server_address_data, &server_address, sizeof server_address);
		if (unlikely(bind(server_endpoint, &server_address_data, sizeof server_address_data) == -1))
			PERROR_AND_GOTO_CLOSEFD("bind", server)
	}
	if (unlikely(listen(server_endpoint, 1) == -1))
		PERROR_AND_GOTO_CLOSEFD("listen", server)
	{
		int client_endpoint = accept(server_endpoint, NULL, NULL);
		if (unlikely(client_endpoint == -1))
			PERROR_AND_GOTO_CLOSEFD("accept", server)
		if (unlikely(close(client_endpoint) == -1))
			PERROR_AND_GOTO_CLOSEFD("close", server)
	}
	if (unlikely(close(server_endpoint) == -1)) PERROR_AND_EXIT("close")
	return EXIT_SUCCESS;
serverclosefd:
	if (unlikely(close(server_endpoint) == -1)) perror("close");
	return EXIT_FAILURE;
}
