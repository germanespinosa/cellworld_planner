#include <params_cpp.h>
#include <cellworld_planner/world_creation.h>
#include <cellworld_planner/lppo.h>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;


int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto configuration = p.get(Key("-wc", "--world_configuration"), "hexagonal");
    auto occlusion_count = stoi(p.get(Key("-o", "--occlusion_count"), "0"));
    auto cluster_max_size = stoi(p.get(Key("-c", "--cluster_max_size"), "0"));
    auto world_file_name = p.get(Key("-w","--world_file_name"),"");
    auto world_stats_file_name = p.get(Key("-ws","--world_stats_file_name"),"");
    auto world_paths_file_name = p.get(Key("-wp","--world_paths_file_name"),"");
    auto world_visibility_file_name = p.get(Key("-wv","--world_visibility_file_name"),"");
    if (configuration.empty()){
        cerr << "missing world configuration parameter (-wc --world_configuration)" << endl;
        exit(1);
    }
    if (world_file_name.empty()){
        cerr << "missing world file name parameter (-w --world_file_name)" << endl;
        exit(1);
    }
    if (world_stats_file_name.empty()){
        cerr << "missing world stats file name parameter (-ws --world_stats_file_name)" << endl;
        exit(1);
    }
    if (world_paths_file_name.empty()){
        cerr << "missing world paths file name parameter (-wp --world_paths_file_name)" << endl;
        exit(1);
    }
    if (world_visibility_file_name.empty()){
        cerr << "missing world paths file name parameter (-wv --world_visibility_file_name)" << endl;
        exit(1);
    }
    auto world = World::get_from_parameters_name(configuration, "canonical");
    auto occlusions = World_creation::get_valid_occlusions(world, occlusion_count, cluster_max_size);
    world.set_occlusions(occlusions);
    auto stats = world.get_statistics(1);
    auto cells = world.create_cell_group();
    auto graph = world.create_graph();
    auto paths = Paths::get_astar(graph);
    auto visibility = Coordinates_visibility::create_graph(cells,world.get_configuration().cell_shape, world.get_implementation().cell_transformation);
    occlusions.save(world_file_name);
    stats.save(world_stats_file_name);
    paths.save(world_paths_file_name);
    visibility.save(world_visibility_file_name);
}