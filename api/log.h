#ifndef BULLSHITCORE_LOG
#define BULLSHITCORE_LOG

#include <stdarg.h>

void bullshitcore_log_log(const char * restrict s);
void bullshitcore_log_logf(const char * restrict format, ...);
void bullshitcore_log_vlogf(const char * restrict format, va_list ap);

#endif
