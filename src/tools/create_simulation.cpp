#include <chrono>
#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;

struct Agents_cells : json_cpp::Json_object {
    Json_object_members(
            Add_member(prey);
            Add_member(predator);
            )
    Agents_cells() = default;
    Agents_cells(int prey, int predator): prey(prey), predator(predator) {}
    int prey{};
    int predator{};
};

void create_trajectories ( const Static_data &data,
                           Simulation_episode& sim_episode,
                           const Episode &experiment_episode,
                           float prey_speed) {
    float time_step = 0;
    Agents_cells agent_cells(-1, -1);
    Agents_cells first_agent_cells(-1, -1);
    bool first = true;
    json_cpp::Json_vector<Agents_cells> episode_cells;
    for (auto &step : experiment_episode.trajectories) {
        int cell_id = data.cells.find(step.location);
        if (step.agent_name == "prey") {
            agent_cells.prey = cell_id;
        } else {
            agent_cells.predator = cell_id;
        }
        if (agent_cells.predator == -1 || agent_cells.prey == -1) continue;
        if (step.time_stamp >= time_step &&
            (first || first_agent_cells.prey != agent_cells.prey || first_agent_cells.predator != agent_cells.predator)) {
            episode_cells.push_back(agent_cells);
            time_step = step.time_stamp + prey_speed;
        }
        if (first) {
            first_agent_cells = agent_cells;
            first = false;
        }
    }
    Model model(data.cells);
    Predator no_predator(data);
    Agent_internal_state_container c(sizeof(Predator_state));
    no_predator.set_internal_state(c, true);
    Dummy prey(data);
    prey.start_cell_id = episode_cells[0].prey;
    Dummy predator(data);
    predator.start_cell_id = episode_cells[0].predator;
    model.add_agent(prey);
    model.add_agent(predator);

    Tree_search ts(data, no_predator);
    Agents_cells prev = episode_cells[0];
    model.start_episode();
    for (int step_index=0;step_index< episode_cells.size(); step_index++){
        auto &step = episode_cells[step_index];
        // move agents to match current locations
        prey.next_move = data.cells[step.prey].coordinates - data.cells[prev.prey].coordinates;
        model.update();
        ts.get_best_move_ucb1(model.state.public_state);
        predator.next_move = data.cells[step.predator].coordinates - data.cells[prev.predator].coordinates;
        model.update();
        ts.record(model.state.public_state);
        auto &sim_step = sim_episode.emplace_back();
        sim_step.prey_state.options = data.options[data.cells[step.prey]].get_builder();
        sim_step.prey_state = ts.history.back().prey_state;
        sim_step.predator_state.cell_id = step.predator;
        prev = step;
    }
    model.end_episode();
}

int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto experiment_file = p.get(Key("-e", "--experiment"), "");
    auto simulation_file = p.get(Key("-s", "--simulation_file"), "");
    auto prey_speed = stof(p.get(Key("-ps", "--prey_speed"), ".116"));
    auto configuration_file = p.get(Key("-c","--configuration_file"),"");
    if (experiment_file.empty()) {
        cout << "Missing experiment file parameter." << endl;
        exit(1);
    }
    if (configuration_file.empty()){
        cout << "Missing simulation configuration file parameter." << endl;
        exit(1);
    }

    int workers = 8;
    Thread_pool tp(workers);
    Experiment experiment;
    experiment.load(experiment_file);
    Simulation simulation;
    simulation.world_info.world_implementation = "canonical";
    simulation.world_info.world_configuration = experiment.world_configuration_name;
    simulation.world_info.occlusions = experiment.occlusions;
    World world = World::get_from_world_info(simulation.world_info);

    Static_data data(simulation.world_info);
    data.predator_moves = world.connection_pattern;
    data.predator_moves.push_back(Coordinates(0,0));
    data.simulation_parameters = json_cpp::Json_from_file<Simulation_parameters>(configuration_file);
    simulation.parameters = data.simulation_parameters;

    auto cells = world.create_cell_group();
    Prey_state prey_state;
    Predator_state predator_state;

    simulation.episodes = json_cpp::Json_vector<Simulation_episode>(experiment.episodes.size());
    for (int e = 0 ; e < experiment.episodes.size(); e++) {
        auto &episode = experiment.episodes[e];
        cout << "Processing trajectories " << e << endl;
        auto &sim_episode = simulation.episodes[e];
//        create_trajectories( data, sim_episode, episode, prey_speed);

        tp.run( create_trajectories,
            std::ref(data),
            std::ref(sim_episode),
            std::ref(episode),
            prey_speed );
    }
    tp.wait();
    simulation.save(simulation_file);
}

