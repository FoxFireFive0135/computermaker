#include <stdlib.h>
#include <string.h>

#include "world.h"
#include "../util.h"
#include "../state.h"
#include "../config.h"
#include "../gfx/renderer.h"
#include "chunk.h"
#include "wire.h"

void world_worldgen(struct world *world) {
    int ax = atoi(config_get("CHUNK_AMOUNT_X")),
        ay = atoi(config_get("CHUNK_AMOUNT_Y")),
        az = atoi(config_get("CHUNK_AMOUNT_Z"));
    for (int x = 0; x < ax; x++) {
        for (int y = -1; y < ay; y++) {
            for (int z = 0; z < az; z++) {
                world_add_chunk(world, chunk_gen(x * CHUNK_X, y * CHUNK_Y, z * CHUNK_Z));
            }
        }
    }
}

chunk_t *world_add_chunk(struct world *world, chunk_t chunk) {
    chunk_t *new_chunk = smalloc(sizeof(chunk_t));
    memcpy(new_chunk, &chunk, sizeof(chunk_t));
    world->chunks = srealloc(world->chunks, ++world->chunks_size * sizeof(chunk_t*));
    world->chunks[world->chunks_size - 1] = new_chunk;
    return new_chunk;
}

void world_draw(struct world *world) {
    renderer_prepare(&state.renderer, RENDERER_PASS_3D);
    for (size_t i = 0; i < world->chunks_size; i++) {
        chunk_draw(world->chunks[i]);
    }
    world_draw_wires();
}

struct world_get_at_info world_get_at(struct world *world, float x, float y, float z) {
    struct world_get_at_info info;
    int index = 0;
    int cx = (int)floor(x) / CHUNK_X * CHUNK_X;
    int cy = (int)floor(y) / CHUNK_Y * CHUNK_Y;
    int cz = (int)floor(z) / CHUNK_Z * CHUNK_Z;
    if (cx>(int)floor(x)) cx -= CHUNK_X;
    if (cy>(int)floor(y)) cy -= CHUNK_Y;
    if (cz>(int)floor(z)) cz -= CHUNK_Z;
    for (; index < world->chunks_size; index++) {
        if (world->chunks[index]->x == cx && world->chunks[index]->y == cy && world->chunks[index]->z == cz)
            break;
    }
    if (index == world->chunks_size) {
        world_add_chunk(world, chunk_gen(cx, cy, cz));
    }
    info.chunk = world->chunks[index];
    info.x = (int)round(x) % CHUNK_X;
    info.y = (int)round(y) % CHUNK_Y;
    info.z = (int)round(z) % CHUNK_Z;
    if (info.x < 0) info.x += CHUNK_X;
    if (info.y < 0) info.y += CHUNK_Y;
    if (info.z < 0) info.z += CHUNK_Z;
    return info;
}

struct world_get_at_relative_info world_get_at_relative(struct world_get_at_info theirinfo) {
    struct world_get_at_relative_info info;
    info.x = theirinfo.chunk->x + theirinfo.x;
    info.y = theirinfo.chunk->y + theirinfo.y;
    info.z = theirinfo.chunk->z + theirinfo.z;
    return info;
}

static void push_logic_block(struct world *world, cached_logic_block_t block) {
    world->logic_blocks_size++;
    while (world->logic_blocks_capacity <= world->logic_blocks_size) {
        world->logic_blocks_capacity <<= 1;
        world->logic_blocks = srealloc(world->logic_blocks, world->logic_blocks_capacity * sizeof(cached_logic_block_t));
    }
    world->logic_blocks[world->logic_blocks_size - 1] = block;
}

void world_cache_sus_logic_block(struct world *world, cached_logic_block_t block) {
    // evict blocks in the cache
    for (size_t i = 0; i < world->logic_blocks_size; i++) {
        if (world->logic_blocks[i].valid && !is_logic_block(*world->logic_blocks[i].block)) {
            world->logic_blocks[i].valid = false;
        }
    }
    if (!is_logic_block(*(block.block))) return;
    // try to find a free entry in the cache
    for (size_t i = 0; i < world->logic_blocks_size; i++) {
        if (!world->logic_blocks[i].valid) {
            world->logic_blocks[i] = block;
            return;
        }
    }
    push_logic_block(world, block);
}

struct world_get_at_info world_place_at(struct world *world, int x, int y, int z, block_t block) {
	struct world_get_at_info info = world_get_at(world, x, y, z);
	info.chunk->blocks[info.x][info.y][info.z] = block;
    world_cache_sus_logic_block(world, (cached_logic_block_t){
        .block = &info.chunk->blocks[info.x][info.y][info.z],
        .chunk = info.chunk,
        .valid = true
    });
	chunk_bake(info.chunk);
	return info;
}
