#include <stdio.h>
#include "log.h"

// TODO: Perform logging in a separate thread

void
bullshitcore_log_log(const char * restrict s)
{
	puts(s);
}

void
bullshitcore_log_logf(const char * restrict format, ...)
{
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
}

void
bullshitcore_log_vlogf(const char * restrict format, va_list ap)
{
	vprintf(format, ap);
}
