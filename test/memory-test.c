#include <assert.h>
#include <errno.h>
#include "memory-pool.h"

int
main(void)
{
#ifdef bullshitcore_memory_pool_retrieve
	assert(bullshitcore_memory_pool_retrieve(4096));
#endif
#ifdef bullshitcore_memory_pool_leave
	void * const pointer = bullshitcore_memory_pool_retrieve(4096);
	bullshitcore_memory_pool_leave(pointer, 4096);
	assert(!errno);
#endif
}
