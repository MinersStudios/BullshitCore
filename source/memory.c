#include <stdlib.h>
#include "global_macros.h"
#include "network.h"
#include "memory.h"

struct PointerMap
{
	void *region;
	size_t region_size;
	Boolean owned;
} pointer_map[256] = { { NULL } };

// thread unsafe
// issues: linear search is going to take a lot of time; tiny memory regions
// are going to deplete the capacity, rendering the map useless.
// solution to 1st issue - perform faster lookup by separating free and
// currently used pointers into separate maps and place pointers in order
// depending on the size of memory region allocated.
// solution to 2nd issue - eventually resize memory regions by yourself.
void *
bullshitcore_memory_retrieve(size_t size)
{
	struct PointerMap pointer;
	for (size_t i = 0; i < NUMOF(pointer_map); ++i)
	{
		pointer = pointer_map[i];
		if (pointer.region_size >= size && !pointer.owned)
			return pointer.region;
	}
	for (size_t i = 0; i < NUMOF(pointer_map); ++i)
		if (!pointer_map[i].region)
		{
			pointer_map[i].region = malloc(size);
			if (unlikely(!pointer_map[i].region)) return NULL;
			pointer_map[i].region_size = size;
			pointer_map[i].owned = true;
			return pointer_map[i].region;
		}
	return NULL;
}

// thread unsafe
void
bullshitcore_memory_leave(void *pointer)
{
	for (size_t i = 0; i < NUMOF(pointer_map); ++i)
		if (pointer_map[i].region == pointer)
		{
			pointer_map[i].owned = false;
			return;
		}
}
