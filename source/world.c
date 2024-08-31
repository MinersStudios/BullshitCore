#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include "global_macros.h"
#include "world.h"

#define WORLD_SIZE 60000000L
#define REGION_FILE_NAME_MAX_SIZE (sizeof "world/region/r...mca" + 2 * (2 + LOG10(WORLD_SIZE / 2)))

uint8_t *
bullshitcore_world_get_chunk(long x, long z)
{
	uint8_t region_file_name[REGION_FILE_NAME_MAX_SIZE];
	if (unlikely(sprintf((char *)region_file_name, "world/region/r.%ld.%ld.mca", x / 32, z / 32) < 0))
		return NULL;
	FILE * const region = fopen((char *)region_file_name, "r");
	if (unlikely(!region)) return NULL;
	uint32_t chunk_location;
	if (unlikely(fseeko(region, ((x % 32 + 32) % 32 + (z % 32 + 32) % 32 * 32) * 4, SEEK_SET) == -1))
		goto close_file;
	if (fread(&chunk_location, 1, 4, region) < 4) goto close_file;
	if (!chunk_location) return NULL;
	if (unlikely(fseeko(region, chunk_location >> 8, SEEK_SET) == -1))
		goto close_file;
	{
		uint8_t *chunk = malloc(chunk_location & 0xFF);
		if (unlikely(!chunk)) goto close_file;
		if (fread(chunk, 1, chunk_location & 0xFF, region) < chunk_location & 0xFF)
			goto free_memory;
		if (unlikely(fclose(region) == EOF)) return NULL;
		return chunk;
free_memory:
		free(chunk);
	}
close_file:
	fclose(region);
	return NULL;
}
