#include <cellworld_planner/static_data.h>

using namespace cell_world;

planner::Static_data::Static_data(const World_info &world_info):
world(World::get_from_world_info(world_info)),
cells(world.create_cell_group()),
map(cells),
graph(cells),
start_cell(map[Coordinates(-20,0)]),
goal_cell(map[Coordinates(20,0)]),
paths(world.create_paths(Resources::from("paths").key(world_info.world_configuration).key(world_info.occlusions).key("astar").get_resource<Path_builder>())),
possible_destinations(world.create_cell_group(Resources::from("cell_group").key(world_info.world_configuration).key(world_info.occlusions).key("predator_destinations").get_resource<Cell_group_builder>())),
lppos(world.create_cell_group(Resources::from("cell_group").key(world_info.world_configuration).key(world_info.occlusions).key("lppo").get_resource<Cell_group_builder>())),
visibility(world.create_graph(Resources::from("graph").key(world_info.world_configuration).key(world_info.occlusions).key("cell_visibility").get_resource<Graph_builder>())),
inverted_visibility(visibility.invert()),
options(world.create_graph(Resources::from("graph").key(world_info.world_configuration).key(world_info.occlusions).key("options").get_resource<Graph_builder>())),
predator_start_locations(world.create_cell_group(Resources::from("cell_group").key(world_info.world_configuration).key(world_info.occlusions).key("spawn_locations").get_resource<Cell_group_builder>())),
capture(Resources::from("capture_parameters").key("default").get_resource<Capture_parameters>())
{
    capture.cool_down = 0;
}
