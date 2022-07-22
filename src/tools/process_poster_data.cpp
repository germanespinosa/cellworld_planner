#include <thread>
#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/statistics.h>
#include <thread_pool.h>
#include <filesystem>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;
using namespace thread_pool;
using std::filesystem::directory_iterator;

void run_simulation( string file_name ) {
    int distance = 1;
    int duration = 4;
    Simulation simulation;
    simulation.load(file_name);
    auto world = World::get_from_world_info(simulation.world_info);

    auto world_stats = World_statistics::get_from_parameters_name(simulation.world_info.world_configuration, simulation.world_info.occlusions);
    string stats_file = file_name;
    stats_file.erase(stats_file.size() - 5);
    stats_file += "_stats.json";
    Simulation_statistics sim_stats;
    sim_stats.load(stats_file);
    int stopped_steps = 0;
    int steps = 0;
    for (auto &episode : simulation.episodes) {
        steps += episode.size();
        int stop_ends = -1;
        for (int step_number=0;step_number<episode.size(); step_number++){
            auto &step = episode[step_number];
            auto &prey_cell = world.cells[step.prey_state.cell_id];
            if (prey_cell.coordinates.x <= -18) continue;
            if (step_number < stop_ends) continue;
            stop_ends = -1;
            for (int step_counter = step_number; step_counter < episode.size(); step_counter++) {
                auto &stop_step = episode[step_counter];
                auto &stop_cell = world.cells[stop_step.prey_state.cell_id];
                if (stop_cell.coordinates.manhattan(prey_cell.coordinates) > 2 * distance) {
                    int counter = step_counter - step_number;
                    if ((counter) >= duration) {
                        stop_ends = step_counter;
                        stopped_steps += counter;
                    }
                    break;
                }
            }
        }
    }
    float stopped_ratio = float(stopped_steps) / float(steps);
    cout <<
    file_name << "," <<
    world_stats.spatial_entropy << "," <<
    world_stats.visual_entropy << "," <<
    world_stats.spatial_espinometry << "," <<
    world_stats.visual_espinometry << "," <<
    stopped_ratio << "," <<
    sim_stats.survival_rate << "," <<
    sim_stats.belief_state_entropy << "," <<
    sim_stats.decision_difficulty << "," <<
    sim_stats.pursue_rate  << "," <<
    sim_stats.visited_cells << endl;
}

int main(int argc, char **argv){
    Parser p(argc,argv);
    auto folder = p.get(Key("-f","--folder"),"");
    if (folder.empty()){
        cout << "Missing simulation configuration file parameter." << endl;
        exit(1);
    }
    Thread_pool tp;
    vector<string> depths{"40", "50", "60", "70", "90", "100", "200", "300", "400", "500"};

    cout <<
         "file_name" << "," <<
         "spatial_entropy" << "," <<
         "visual_entropy" << "," <<
         "spatial_espinometry" << "," <<
         "visual_espinometry" << "," <<
         "stopped_ratio" << "," <<
         "survival_rate" << "," <<
         "belief_state_entropy" << "," <<
         "decision_difficulty" << "," <<
         "pursue_rate"  << "," <<
         "visited_cells" << endl;

    for (const auto & file : directory_iterator(folder + "/")){
        string file_path = file.path();
        if (file_path.find("_stats") == string::npos){
            tp.run(run_simulation, file_path);
        }
    }
    tp.wait_all();
    exit (0);
}