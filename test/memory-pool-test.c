#include <assert.h>
#include <errno.h>
#include "memory-pool.h"

int
main(void)
{
#ifdef BULLSHITCORE_MEMORY_POOL_RETRIEVE
	assert(bullshitcore_memory_pool_retrieve(4096));
#endif
#ifdef BULLSHITCORE_MEMORY_POOL_LEAVE
	void * const region = bullshitcore_memory_pool_retrieve(4096);
	bullshitcore_memory_pool_leave(region, 4096);
	assert(!errno);
#endif
}
