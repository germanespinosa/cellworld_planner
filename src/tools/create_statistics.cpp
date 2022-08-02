#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/statistics.h>


using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;

int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto simulation_file = p.get(Key("-s", "--simulation_file"), "");
    auto stats_file = p.get(Key("-o", "--output_statistics_file"), "");
    if (simulation_file.empty()) {
        cout << "Missing simulation file parameter." << endl;
        exit(1);
    }
    if (stats_file.empty()) {
        auto last_point = simulation_file.find_last_of('.');
        stats_file = simulation_file.substr(0,last_point) + "_stats" + simulation_file.substr(last_point);
    }
    int workers = 8;
    Simulation simulation;
    try {
        if (!simulation.load(simulation_file)) {
            cout << "Simulation file "  << simulation_file << " not found." << endl;
            exit(1);
        }
    } catch(...){
        cout << "Not a simulation file." << endl;
        exit(1);
    }
    World w = World::get_from_world_info(simulation.world_info);
    auto data = Static_data(w);
    data.cells = w.create_cell_group();
    data.paths = w.create_paths(Resources::from("paths").key(simulation.world_info.world_configuration).key(simulation.world_info.occlusions).key("astar").get_resource<Path_builder>());
    Simulation_statistics sim_stats(simulation, data);
    sim_stats.save(stats_file);
    cout << "done!" << endl;
}