#include <assert.h>
#include <errno.h>
#include "memory.h"

int
main(void)
{
#ifdef BULLSHITCORE_MEMORY_RETRIEVE
	assert(bullshitcore_memory_retrieve(4096));
#endif
#ifdef BULLSHITCORE_MEMORY_LEAVE
	void * const pointer = bullshitcore_memory_retrieve(4096);
	bullshitcore_memory_leave(pointer, 4096);
	assert(!errno);
#endif
}
