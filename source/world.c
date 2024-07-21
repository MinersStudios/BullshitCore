#include <dirent.h>
#include <errno.h>
#include "global_macros.h"
#include "world.h"

int
bullshitcore_world_load_world(void)
{
	DIR *world = opendir("./world");
	if (unlikely(!world)) return errno;
	return 0;
}

int
bullshitcore_world_unload_world(void)
{
	return 0;
}
