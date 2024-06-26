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

#define MINECRAFT_VERSION "1.21"
#define PROTOCOL_VERSION 767
#define PROTOCOL_VERSION_STRING EXPAND_AND_STRINGIFY(PROTOCOL_VERSION)
#define PERROR_AND_GOTO_DESTROY(s, object) { perror(s); goto destroy_ ## object; }
#define THREAD_STACK_SIZE 8388608
#define PACKET_MAXSIZE 2097151

// config
#define ADDRESS "0.0.0.0"
#define PORT 25565
#define MAX_CONNECTIONS 15
#define FAVICON

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
		uint8_t buffer[PACKET_MAXSIZE];
		size_t buffer_offset, packet_next_boundary;
		int ret;
		while (1)
		{
			const ssize_t bytes_read = recv(client_endpoint, buffer, PACKET_MAXSIZE, 0);
			if (!bytes_read) return NULL;
			else if (unlikely(bytes_read == -1)) goto clear_stack_receiver;
			bullshitcore_network_varint_decode(buffer, &packet_next_boundary);
			buffer_offset = packet_next_boundary;
			const int32_t packet_identifier = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
			buffer_offset += packet_next_boundary;
			switch (current_state)
			{
				case State_Handshaking:
				{
					switch (packet_identifier)
					{
						case Packet_Handshake:
						{
							const int32_t client_protocol_version = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							if (client_protocol_version != PROTOCOL_VERSION)
								bullshitcore_log_log("Warning! Client and server protocol version mismatch.");
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
						case Packet_Status_Request:
						{
							ret = pthread_mutex_lock(interthread_buffer_mutex);
							if (unlikely(ret))
							{
								errno = ret;
								goto clear_stack_receiver;
							}
							size_t interthread_buffer_offset = 0;
							// TODO: Sanitise input (implement it after testing)
							const char * const text = "{\"version\":{\"name\":\"" MINECRAFT_VERSION "\",\"protocol\":" PROTOCOL_VERSION_STRING "},\"description\":{\"text\":\"BullshitCore is up and running!\",\"favicon\":\"" FAVICON "\"}}";
							const size_t text_length = strlen(text);
							const JSONTextComponent packet_payload =
							{
								bullshitcore_network_varint_encode(text_length),
								text
							};
							if (text_length > JSONTEXTCOMPONENT_MAXSIZE) // TODO: Shall probably do something about it
								break;
							size_t packet_payload_length_length;
							bullshitcore_network_varint_decode(packet_payload.length, &packet_payload_length_length);
							const size_t packet_length = 1 + packet_payload_length_length + text_length;
							const VarInt packet_length_varint = bullshitcore_network_varint_encode(packet_length);
							size_t packet_length_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_length);
							*interthread_buffer_length = packet_length_length + packet_length;
							memcpy(interthread_buffer, packet_length_varint, packet_length_length);
							interthread_buffer_offset += packet_length_length;
							interthread_buffer[interthread_buffer_offset] = Packet_Status_Request;
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
						case Packet_Status_Ping_Request:
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
					break;
				}
				case State_Transfer:
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
#ifndef NDEBUG
			bullshitcore_log_log("Packet jump table looped.");
#endif
		}
	}
get_closed_receiver:
	return NULL;
clear_stack_receiver:; // Deallocate all stack variables to preserve space for error recovery.
	// TODO: Also pass failed function name
	const int my_errno = errno;
#ifndef NDEBUG
	bullshitcore_log_logf("Receiver thread crashed! %s\n", strerror(my_errno));
#endif
	int * const p_my_errno = malloc(sizeof my_errno);
	if (unlikely(!p_my_errno)) return (void *)1; // In case if errno was lost due to a catastrophic failure.
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
