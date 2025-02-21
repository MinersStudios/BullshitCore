#ifdef BULLSHITCORE_ENABLE_MEMORY_POOLING
# include <errno.h>
# include <pthread.h>
# include "global-macros.h"
#endif
#include <stdlib.h>
#include "memory-pool.h"

#ifdef BULLSHITCORE_ENABLE_MEMORY_POOLING
# ifndef MEMORY_POOL_SIZE
#  define MEMORY_POOL_SIZE 256
# endif

static struct MemoryRegion
{
	void *data;
	size_t region_size;
} memory_pool[MEMORY_POOL_SIZE];
static pthread_mutex_t memory_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void *
bullshitcore_memory_pool_retrieve(size_t size)
{
#ifdef BULLSHITCORE_ENABLE_MEMORY_POOLING
	struct MemoryRegion region;
	int ret = pthread_mutex_lock(&memory_pool_mutex);
	if (unlikely(ret))
	{
		errno = ret;
		return NULL;
	}
	for (size_t i = 0; i < NUMOF(memory_pool); ++i)
	{
		region = memory_pool[i];
		if (region.region_size >= size)
		{
			memory_pool[i].data = NULL;
			memory_pool[i].region_size = 0;
			ret = pthread_mutex_unlock(&memory_pool_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				return NULL;
			}
			return region.data;
		}
	}
	ret = pthread_mutex_unlock(&memory_pool_mutex);
	if (unlikely(ret))
	{
		errno = ret;
		return NULL;
	}
#endif
	return malloc(size);
}

void
bullshitcore_memory_pool_leave(void * restrict region, size_t size)
{
#ifdef BULLSHITCORE_ENABLE_MEMORY_POOLING
	int ret = pthread_mutex_lock(&memory_pool_mutex);
	if (unlikely(ret))
	{
		errno = ret;
		return;
	}
	for (size_t i = 0; i < NUMOF(memory_pool); ++i) if (!memory_pool[i].data)
	{
		memory_pool[i].data = region;
		memory_pool[i].region_size = size;
		ret = pthread_mutex_unlock(&memory_pool_mutex);
		if (unlikely(ret)) errno = ret;
		return;
	}
	ret = pthread_mutex_unlock(&memory_pool_mutex);
	if (unlikely(ret)) errno = ret;
#else
	free(region);
#endif
}
