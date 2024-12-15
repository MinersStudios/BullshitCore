#ifndef BULLSHITCORE_LOG
#define BULLSHITCORE_LOG

#include <stdarg.h>

void bullshitcore_log_log(const char * restrict s);
void bullshitcore_log_log_formatted(const char * restrict format, ...);
void bullshitcore_log_variadic_log_formatted(const char * restrict format, va_list ap);
void bullshitcore_log_error(const char * restrict s);
void bullshitcore_log_error_formatted(const char * restrict format, ...);
void bullshitcore_log_variadic_error_formatted(const char * restrict format, va_list ap);

#endif
