#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "log.h"

#define MESSAGE "Hello, p0loskun!"

static void
variadic_wrapper(va_list * restrict variadic_arguments, ...)
{
	va_start(*variadic_arguments, variadic_arguments);
}

int
main(void)
{
	int pipe_ends[2];
	assert(!pipe(pipe_ends));
	FILE * const logged_message = fdopen(*pipe_ends, "r");
	assert(logged_message);
#ifdef BULLSHITCORE_LOG_LOG
	assert(dup2(pipe_ends[1], STDOUT_FILENO) != -1);
	bullshitcore_log_log(MESSAGE);
	assert(!errno);
	assert(!fflush(stdout));
#endif
#ifdef BULLSHITCORE_LOG_LOG_FORMATTED
	assert(dup2(pipe_ends[1], STDOUT_FILENO) != -1);
	bullshitcore_log_log_formatted("%s\n", MESSAGE);
	assert(!errno);
	assert(!fflush(stdout));
#endif
#ifdef BULLSHITCORE_LOG_VARIADIC_LOG_FORMATTED
	va_list variadic_arguments;
	variadic_wrapper(&variadic_arguments, MESSAGE);
	assert(dup2(pipe_ends[1], STDOUT_FILENO) != -1);
	bullshitcore_log_variadic_log_formatted("%s\n", variadic_arguments);
	assert(!errno);
	assert(!fflush(stdout));
#endif
#ifdef BULLSHITCORE_LOG_ERROR
	assert(dup2(pipe_ends[1], STDERR_FILENO) != -1);
	bullshitcore_log_error(MESSAGE);
	assert(!errno);
	assert(!fflush(stderr));
#endif
#ifdef BULLSHITCORE_LOG_ERROR_FORMATTED
	assert(dup2(pipe_ends[1], STDERR_FILENO) != -1);
	bullshitcore_log_error_formatted("%s\n", MESSAGE);
	assert(!errno);
	assert(!fflush(stderr));
#endif
#ifdef BULLSHITCORE_LOG_VARIADIC_ERROR_FORMATTED
	va_list variadic_arguments;
	variadic_wrapper(&variadic_arguments, MESSAGE);
	assert(dup2(pipe_ends[1], STDERR_FILENO) != -1);
	bullshitcore_log_variadic_error_formatted("%s\n", variadic_arguments);
	assert(!errno);
	assert(!fflush(stderr));
#endif
	for (size_t i = 0; i < sizeof MESSAGE - 1; ++i)
		assert((uint8_t)getc(logged_message) == (uint8_t)MESSAGE[i]);
	assert((uint8_t)getc(logged_message) == (uint8_t)'\n');
}
