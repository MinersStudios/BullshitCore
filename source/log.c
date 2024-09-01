#include <stdio.h>
#include "log.h"

// TODO: Perform logging in a separate thread

void
bullshitcore_log_log(const char * restrict s)
{
	puts(s);
}

void
bullshitcore_log_log_formatted(const char * restrict format, ...)
{
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
}

void
bullshitcore_log_variadic_log_formatted(const char * restrict format, va_list ap)
{
	vprintf(format, ap);
}

void
bullshitcore_log_error(const char * restrict s)
{
	fputs(s, stderr);
}

void bullshitcore_log_error_formatted(const char * restrict format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

void
bullshitcore_log_variadic_error_formatted(const char * restrict format, va_list ap)
{
	vfprintf(stderr, format, ap);
}
