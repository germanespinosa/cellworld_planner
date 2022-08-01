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
            cout << "Simulation file not found." << endl;
            exit(1);
        }
    } catch(...){
        cout << "Not a simulation file." << endl;
        exit(1);
    }
    auto data = Static_data(simulation.world_info);
    Simulation_statistics sim_stats(simulation, data);
    sim_stats.save(stats_file);
}