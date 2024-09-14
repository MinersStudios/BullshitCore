#ifndef BULLSHITCORE_MEMORY
#define BULLSHITCORE_MEMORY

#include <stddef.h>

void *bullshitcore_memory_retrieve(size_t size);
void bullshitcore_memory_leave(void * restrict pointer, size_t size);

#endif
