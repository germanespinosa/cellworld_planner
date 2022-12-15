#include <chrono>
#include <performance.h>
#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/statistics.h>
#include <cellworld_planner/tree_search_location.h>
#include <thread_pool.h>

using namespace thread_pool;
using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;


struct Agents_cells : json_cpp::Json_object {
    Json_object_members(
            Add_member(prey);
            Add_member(prey_location);
            Add_member(prey_orientation);
            Add_member(predator_location);
            Add_member(predator);
            Add_member(los);
            )
    Agents_cells() = default;
    Agents_cells(int prey, int predator, unsigned int frame, Location &prey_location, float prey_orientation, Location &predator_location):
        prey(prey),
        predator(predator),
        frame(frame),
        prey_location(prey_location),
        prey_orientation(prey_orientation),
        predator_location(predator_location){}
    int prey{};
    int predator{};
    unsigned int frame{};
    Location prey_location{};
    float prey_orientation{};
    Location predator_location{};
    bool los{};
};

struct Simulation_step_data : json_cpp::Json_object {
    Simulation_step_data() = default;
    explicit Simulation_step_data (unsigned int frame) : frame(frame) {};
    Json_object_members(
            Add_member(frame);
            )
    unsigned int frame{};
};

struct Line_of_sight  : json_cpp::Json_object{
    Json_object_members(
            Add_member_with_name(visible,true,"LOS");
    )
    bool visible;
};

void create_trajectories ( const Static_data &data,
                           Simulation_episode& sim_episode,
                           const Episode &experiment_episode,
                           float prey_speed) {
    Location_visibility lv(data.cells,data.world.cell_shape,data.world.cell_transformation);
    double time_step = 0;
    Agents_cells agent_cells;
    Agents_cells first_agent_cells;
    bool first = true;
    json_cpp::Json_vector<Agents_cells> episode_cells;
    for (auto &step : experiment_episode.trajectories) {
        int cell_id = data.cells.find(step.location);
        if (step.agent_name == "prey") {
            agent_cells.prey = cell_id;
            agent_cells.prey_orientation = to_radians(step.rotation);
            agent_cells.prey_location = step.location;
            agent_cells.los = lv.is_visible(agent_cells.prey_location,agent_cells.predator_location);
        } else {
            agent_cells.predator = cell_id;
            agent_cells.predator_location = step.location;
        }
        if (agent_cells.predator == -1 || agent_cells.prey == -1) continue;
        if (step.time_stamp >= time_step &&
            (first || first_agent_cells.prey != agent_cells.prey || first_agent_cells.predator != agent_cells.predator)) {
            agent_cells.frame = step.frame;
            episode_cells.push_back(agent_cells);
            agent_cells.los = false;
            time_step = step.time_stamp + prey_speed;
        }
        if (first) {
            first_agent_cells = agent_cells;
            first = false;
        }
    }
    if (episode_cells.empty()) return;
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

    Tree_search_location ts(data, no_predator);
    Agents_cells prev = episode_cells[0];
    model.start_episode();
    for (int step_index = 0;step_index < episode_cells.size(); step_index++){
        auto &step = episode_cells[step_index];
        // move agents to match current locations
        prey.next_move = data.cells[step.prey].coordinates - data.cells[prev.prey].coordinates;
        model.update();
        ts.get_best_move_ucb1(model.state.public_state, step.prey_location, step.prey_orientation, step.predator_location, step.los);
        predator.next_move = data.cells[step.predator].coordinates - data.cells[prev.predator].coordinates;
        model.update();
        auto &sim_step = sim_episode.emplace_back();
        sim_step.prey_state = ts.history.back().prey_state;
        ts.record(model.state.public_state, step.prey_location, step.prey_orientation, step.predator_location, step.los);
        sim_step.predator_state.cell_id = step.predator;
        sim_step.data = Simulation_step_data(step.frame).to_json();
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
    Thread_pool tp(8);
    Experiment experiment;
    if (!experiment.load(experiment_file)){
        cout << "Failed to open experiment file." << endl;
        exit(1);
    }
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

    simulation.episodes = json_cpp::Json_vector<Simulation_episode>(experiment.episodes.size());
    for (int e = 0 ; e < experiment.episodes.size(); e++) {
        auto &episode = experiment.episodes[e];
        cout << "Processing trajectories " << e << endl;
        auto &sim_episode = simulation.episodes[e];
        tp.run([&data, &sim_episode, &episode, &prey_speed]() {
            create_trajectories(data, sim_episode, episode, prey_speed);
        });
    }
    tp.wait_all();
    simulation.save(simulation_file + ".json");
    Simulation_statistics stats(simulation, data);
    stats.save(simulation_file + "_stats.json");
}
