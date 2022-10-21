#include <iostream>
#include <robot_lib/robot_simulator.h>
#include <params_cpp.h>
#include <agent_tracking/tracking_client.h>
#include <cell_world.h>
#include <experiment.h>
#include <controller.h>
#include <map>
#include <robot_lib/robot_agent.h>
#include <cellworld_planner/prey.h>
#include <cellworld_planner/static_data.h>
#include <cellworld_planner/lppo.h>
#include <cellworld_planner/tree_search.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace params_cpp;
using namespace robot;
using namespace experiment;
using namespace controller;

bool episode_running = false;

struct Robot_experiment_client : Experiment_client {
    void on_experiment_started(const Start_experiment_response &experiment) override {
        experiment_occlusions[experiment.experiment_name] = experiment.world.occlusions;
    }
    void on_episode_started(const std::string &experiment_name) override {
        episode_running = true;
    };
    void on_episode_finished() override {
        episode_running = false;
    };
    std::map<string, string> experiment_occlusions;
};

int main(int argc, char *argv[])
{
    Key spawn_coordinates_key{"-s","--spawn_coordinates"};
    Key rotation_key{"-r","--theta"};
    Key prey_key{"-p","--prey"};
    Key interval_key{"-i","--interval"};
    Key noise_key{"-n","--noise"};

    Parser p(argc, argv);

    World_info world_info;
    world_info.world_configuration = "hexagonal";
    world_info.world_implementation = "canonical";
    world_info.occlusions = "21_05";

    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("canonical").get_resource<World_implementation>();
    auto peeking_parameters = Resources::from("peeking_parameters").key("default").get_resource<Peeking_parameters>();

    auto reward = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/reward/" + p.get(Key("-r", "--reward"), "reward1");
    auto tree_search_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/tree_search_parameters/" + p.get(Key("-t", "--tree_search_parameters"), "1000");
    auto predator_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/predator_parameters/" + p.get(Key("-p", "--predator_parameters"), "fast_25_randomness");
    auto prey_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/prey_parameters/" + p.get(Key("-y", "--prey_parameters"), "default");
    auto capture_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/capture_parameters/" + p.get(Key("-c", "--capture_parameters"), "small");
    auto steps = stoi (p.get(Key("-sc", "--step_count"), "100"));

    auto data = planner::Static_data(world_info);
    data.simulation_parameters.reward.load(reward);
    data.simulation_parameters.tree_search_parameters.load(tree_search_parameters);
    data.simulation_parameters.predator_parameters.load(predator_parameters);
    data.simulation_parameters.prey_parameters.load(prey_parameters);
    data.simulation_parameters.capture_parameters.load(capture_parameters);
    data.simulation_parameters.steps = steps;

    Capture capture(data.simulation_parameters.capture_parameters, data.world);
    Peeking peeking(peeking_parameters, data.world);
    Location_visibility visibility(data.cells, wc.cell_shape, wi.cell_transformation);

    auto rotation = stof(p.get(rotation_key,"1.5707963267948966"));
    auto interval = stoi(p.get(interval_key,"30"));
    auto spawn_coordinates_str = p.get(spawn_coordinates_key, "{\"x\":4,\"y\":0}");

    Experiment_service::set_logs_folder("experiment_logs/");
    Experiment_server experiment_server;

    if (!p.contains(noise_key)){
        Robot_simulator::tracking_server.noise = 0;
        Robot_simulator::tracking_server.frame_drop = 0;
        Robot_simulator::tracking_server.bad_reads = 0;
    }
    auto &experiment_tracking_client = Robot_simulator::tracking_server.create_local_client<Experiment_tracking_client>();
    experiment_tracking_client.subscribe();
    experiment_server.set_tracking_client(experiment_tracking_client);
    experiment_server.start(Experiment_service::get_port());


    auto &tracking_client = Robot_simulator::tracking_server.create_local_client<Controller_server::Controller_tracking_client>(visibility, 90, capture, peeking, "predator", "prey");
    auto &experiment_client= experiment_server.create_local_client<Robot_experiment_client>();
    experiment_client.subscribe();
    auto predator_spawn_cell = data.predator_start_locations.random_cell();

    cout << predator_spawn_cell << endl;
    Location predator_spawn_location = predator_spawn_cell.location;
    Location prey_spawn_location = data.start_cell().location;
    Robot_simulator::start_simulation(data.world, predator_spawn_location, rotation, prey_spawn_location, rotation, interval);

    auto &tracker = Robot_simulator::tracking_server.create_local_client<agent_tracking::Tracking_client>();
    tracker.connect();
    tracker.subscribe();

    Tick_agent_moves tick_moves;
    tick_moves.load("../config/tick_robot_moves.json");


    Tick_robot_agent prey_robot(tick_moves, data.map, tracker);
    prey_robot.connect("127.0.0.1");


    //starts the experiment
    auto experiment_started = experiment_client.start_experiment(world_info,"AGENT",30,"SIM","TEST");


    // create fake simulation objects
    Model model(data.cells);
    planner::Predator no_predator(data);
    Agent_internal_state_container c(sizeof(planner::Predator_state));
    no_predator.set_internal_state(c, true);
    planner::Dummy dummy_prey(data);
    planner::Dummy dummy_predator(data);
    model.add_agent(dummy_prey);
    model.add_agent(dummy_predator);
    planner::Tree_search ts(data, no_predator);

    while (!tracker.contains_agent_state("predator"));
    while (!tracker.contains_agent_state("prey"));

    // sets start cell for dummies
    dummy_prey.start_cell_id = data.cells[data.cells.find(tracker.get_current_state("prey").location)].id;
    dummy_predator.start_cell_id = data.cells[data.cells.find(tracker.get_current_state("predator").location)].id;
    //starts the episode
    auto episode_started = experiment_client.start_episode(experiment_started.experiment_name);
    model.start_episode();
    while (!episode_running);
    int i = 0;
    Coordinates target_coordinates(-20,0);
    while(!tracker.contains_agent_state("prey"));
    Coordinates last_coordinates;
    float target_distance = 0;
    unsigned int step_counter =  0;
    while (Robot_simulator::is_running()) {
        auto predator_location = tracker.get_current_state("predator").location;
        auto prey_location = tracker.get_current_state("prey").location;
        auto &prey_cell = data.cells[data.cells.find(prey_location)];
        auto &predator_cell = data.cells[data.cells.find(predator_location)];
        if (prey_cell == data.goal_cell()) {
            ts.record(model.state.public_state);
            break;
        }
        if (prey_robot.completed_move == prey_robot.move_counter - 1){
            cout << step_counter << " " << prey_robot.current_coordinates << endl;
            if (prey_cell.coordinates != prey_robot.current_coordinates ){
                cout << "ERROR: "<< prey_cell.coordinates << endl << endl;
            }
            auto state = model.get_state().public_state;
            auto &prey_state = state.agents_state[0];
            auto &predator_state = state.agents_state[1];
            prey_state.cell = data.map[prey_robot.current_coordinates];
            predator_state.cell = predator_cell;
            state.current_turn = 0;
            auto move = ts.get_best_move_ucb1(state);
            if (move !=  No_move) prey_robot.execute_move(move);
            prey_state.cell = data.map[prey_robot.current_coordinates];
            predator_state.cell = predator_cell;
            state.current_turn = 1;
            ts.record(state);
            step_counter++;
        }
        if (step_counter == steps) {
            break;
        }
    }
    experiment_client.finish_episode();
    experiment_client.finish_experiment(experiment_started.experiment_name);
    Robot_simulator::stop_simulation();
    return 0;
}