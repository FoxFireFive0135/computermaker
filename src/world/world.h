#pragma once

#include "chunk.h"
#include "skybox.h"

typedef struct {
    block_t *block;
    chunk_t *chunk;
    bool valid;
} cached_logic_block_t;

struct world {
    chunk_t **chunks;
    size_t chunks_size;
    skybox_t skybox;
    cached_logic_block_t *logic_blocks;
    size_t logic_blocks_size, logic_blocks_capacity;
};

void world_worldgen(struct world *world);
chunk_t *world_add_chunk(struct world *world, chunk_t chunk);
void world_draw(struct world *world);
struct world_get_at_info {
    chunk_t *chunk;
    int x, y, z; // relative to chunk
};
struct world_get_at_info world_get_at(struct world *world, float x, float y, float z);
struct world_get_at_relative_info {
    int x, y, z;
};
struct world_get_at_relative_info world_get_at_relative(struct world_get_at_info info);
struct world_get_at_info world_place_at(struct world *world, int x, int y, int z, block_t block);
void world_cache_sus_logic_block(struct world *world, cached_logic_block_t block);
