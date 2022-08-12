#pragma once
#include <cell_world.h>

namespace cell_world::planner {
    struct World_creation {
        static Cell_group_builder get_valid_occlusions(World &wc, unsigned int k, int max_cluster_size=-1);
    };
}