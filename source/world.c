#define _XOPEN_SOURCE 500
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include "config.h"
#include "global-macros.h"
#include "memory-pool.h"
#include "world.h"

#define REGION_FILE_NAME_MAX_SIZE 33

uint8_t *
bullshitcore_world_chunk_load(long x, long z)
{
// 	uint8_t region_file_name[REGION_FILE_NAME_MAX_SIZE];
// 	if (unlikely(sprintf((char *)region_file_name, "world/region/r.%ld.%ld.mca", x / 32, z / 32) < 0))
// 		return NULL;
// 	FILE * const region = fopen((char *)region_file_name, "r");
// 	if (unlikely(!region)) return NULL;
// 	uint32_t chunk_location;
// 	if (unlikely(fseeko(region, ((x % 32 + 32) % 32 + (z % 32 + 32) % 32 * 32) * 4, SEEK_SET) == -1))
// 		goto close_file;
// 	if (fread(&chunk_location, 1, 4, region) < 4) goto close_file;
// 	if (!chunk_location) return NULL;
// 	if (unlikely(fseeko(region, (chunk_location >> 8) * 4096, SEEK_SET) == -1))
// 		goto close_file;
// 	uint32_t chunk_size;
// 	if (fread(&chunk_size, 1, 4, region) < 4) goto close_file;
// 	--chunk_size;
// 	if (unlikely(fseeko(region, 1, SEEK_CUR) == -1)) goto close_file;
// 	{
// 		const off_t region_offset = ftello(region);
// 		if (unlikely(region_offset == -1)) goto close_file;
// 		uint8_t * const command = bullshitcore_memory_pool_retrieve(sizeof "dd if= skip= count=B bs= | zcat -" - 1 + strlen(region_file_name) + LOG10(region_offset) + 2 + LOG10(chunk_size) + 1 + sizeof CHUNK_READ_BLOCK_SIZE - 1);
// 		if (unlikely(sprintf(command, "dd if=%s skip=%lld count=%luB bs=" CHUNK_READ_BLOCK_SIZE " | zcat -", region_file_name, region_offset, chunk_size) < 0))
// 			goto close_file;
// 		FILE * const uncompressed_chunk = popen(command, "r");
// 		if (unlikely(!uncompressed_chunk)) goto close_file;
// 		uint8_t * const chunk = bullshitcore_memory_pool_retrieve(chunk_size);
// 		if (unlikely(!chunk)) goto close_file;
// 		while (fread())
// 		if (unlikely(fclose(region) == EOF)) return NULL;
// 		return chunk;
// free_memory:
// 		bullshitcore_memory_pool_leave(chunk, chunk_size);
// 	}
// close_file:
// 	fclose(region);
	return NULL;
}
