#include <cellworld_planner/static_data.h>

using namespace cell_world;

planner::Static_data::Static_data(World &world):
world(world),
cells(world.create_cell_group()),
map(cells),
graph(cells),
start_cell(map[Coordinates(-20,0)]),
goal_cell(map[Coordinates(20,0)]),
paths(world.create_paths(Resources::from("paths").key("hexagonal").key("20_05").key("astar").get_resource<Path_builder>()))
{

}
