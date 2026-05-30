#include "state.h"

struct state state = {
    .world.logic_blocks_capacity = 1
};

const char *mode_name(enum mode mode) {
    switch (mode) {
        case MODE_BLOCK_PLACE: return "BLOCK_PLACE";
        case MODE_WIRE_PLACE: return "WIRE_PLACE";
        case MODE_WIRE_PARALLEL_PLACE: return "WIRE_PARALLEL_PLACE";
        case MODE_WIRE_PARALLEL_DESTROY: return "WIRE_PARALLEL_DESTROY";
        case MODE_WIRE_DESTROY: return "WIRE_DESTROY";
        case MODE_BLOCK_POKE: return "BLOCK_POKE";
        case MODE_BLOCK_HOVER: return "BLOCK_HOVER";
        case MODE_BUILDING_PLACE: return "BUILDING_PLACE";
        case MODE_BUILDING_POKE: return "BUILDING_POKE";
        default: return "UNKNOWN";
    }
}
