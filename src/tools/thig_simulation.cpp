#include <chrono>
#include <thread>
#include <cell_world.h>
#include <cellworld_planner/thig_prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;

void run_simulation( const Static_data &data,
                     Simulation_episode &episode,
                     unsigned int seed) {
    Chance::seed(seed);
    Model model(data.cells, 1000);
    Predator predator(data);
    Thig_prey prey(data, predator);
    model.add_agent(prey);
    model.add_agent(predator);
    model.run();
    for (auto &dp:prey.history){
        auto &step = episode.emplace_back();
        step.prey_state = dp.prey_state;
        step.predator_state = dp.predator_state;
    }
}

int main(int argc, char **argv){
    Parser p(argc,argv);
    auto configuration_file = p.get(Key("-c","--configuration_file"),"");
    if (configuration_file.empty()){
        cout << "Missing simulation configuration file parameter." << endl;
        exit(1);
    }

    auto results_file = p.get(Key("-r","--results_file"),"results.json");
    auto occlusions = p.get(Key("-o","--occlusions"),"20_05");
    auto seed_start = stoi(p.get(Key("-ss","--seed_start"),"0"));
    auto seed_end = stoi(p.get(Key("-se","--seed_end"),"0"));
    auto workers = stoi(p.get(Key("-w","--workers"),"4"));
    auto world = World::get_from_parameters_name("hexagonal","canonical",occlusions);

    World_info world_info;
    world_info.world_configuration = "hexagonal";
    world_info.world_implementation = "canonical";
    world_info.occlusions = occlusions;

    Static_data data(world_info);
    data.predator_moves = world.connection_pattern;
    data.predator_moves.push_back(Coordinates(0,0));
    data.simulation_parameters = json_cpp::Json_from_file<Simulation_parameters>(configuration_file);

    Simulation simulation;
    simulation.world_info.world_configuration = "hexagonal";
    simulation.world_info.world_implementation = "canonical";
    simulation.world_info.occlusions = occlusions;
    simulation.parameters = data.simulation_parameters;
    simulation.episodes.reserve(seed_end-seed_start);

    Thread_pool tp(workers);
    for (unsigned int s = seed_start;s < seed_end;s++) {
        cout << "Processing seed " << s << endl;
        auto &episode = simulation.episodes.emplace_back();
        tp.run(run_simulation,
               std::ref(data),
               std::ref(episode),
               s);
    }
    tp.wait();
    simulation.save(results_file);
}