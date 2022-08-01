#include <cell_world/cell_group.h>
#include <cell_world/world.h>
#include <cell_world/graph.h>
#include <cell_world/paths.h>


namespace cell_world::planner {
    struct Lppos {
        Lppos (const World &, const World_statistics &);
        Cell_group_builder get_spatial_lppos(unsigned int);
        Cell_group_builder get_visual_lppos(unsigned int);
        const World &world;
        const World_statistics &world_statistic;
    };
    struct Options {
        static Graph get_options(const Paths &paths, Cell_group &lppos);
    };
}