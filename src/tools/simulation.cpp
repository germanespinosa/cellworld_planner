#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;

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
    simulation.parameters = data.simulation_parameters;
    auto &experiment = simulation.experiment;
    experiment.world_configuration_name = world_info.world_configuration;
    experiment.world_implementation_name = world_info.world_implementation;
    experiment.occlusions = world_info.occlusions;
    experiment.start_time = json_cpp::Json_date::now();
    experiment.name = "SIMULATION";
    for (int s = seed_start;s < seed_end;s++) {
        Model model(data.cells, 1000);
        Prey prey(data);
        Predator predator(data);
        model.add_agent(prey);
        model.add_agent(predator);
        auto episode_count = std::string(4 - to_string(experiment.episodes.size()).size(), '0') + to_string(experiment.episodes.size());
        auto &episode = experiment.episodes.emplace_back();
        cout << "running episode " << episode_count << endl;
        model.run();
        unsigned int frame = 0;
        auto &episode_history = simulation.prey_data.emplace_back();
        for (auto &dp:prey.mcts.history){
            frame++;
            auto &step = episode.trajectories.emplace_back();
            if (dp.state.current_turn == PREY){
                step.agent_name = "prey";
                step.location = dp.state.agents_state[PREY].cell.location;
                step.frame = frame;
                step.time_stamp = frame;
                if (dp.state.status==cell_world::Model_public_state::Running) {
                    auto &prey_state = episode_history.emplace_back();
                    prey_state = dp.prey_state;
                    prey_state.frame = frame;
                }
            } else {
                step.agent_name = "predator";
                step.location = dp.state.agents_state[PREDATOR].cell.location;
                step.frame = frame;
                step.time_stamp = frame;
            }
        }
    }
    simulation.save(results_file);
}