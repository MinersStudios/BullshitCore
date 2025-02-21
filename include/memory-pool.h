#ifndef BULLSHITCORE_MEMORY_POOL
#define BULLSHITCORE_MEMORY_POOL

#include <stddef.h>

void *bullshitcore_memory_pool_retrieve(size_t size);
void bullshitcore_memory_pool_leave(void * restrict pointer, size_t size);

#endif
