#include <iostream>
#include <easy_tcp.h>
#include <robot_lib/robot_simulator.h>
#include <params_cpp.h>
#include <agent_tracking/tracking_client.h>
#include <robot_lib/tracking_simulator.h>
#include <cell_world.h>
#include <experiment.h>
#include <controller.h>
#include <map>
#include <robot_lib/robot_agent.h>
#include <cellworld_planner.h>
#include <cellworld_planner/prey.h>
#include <cellworld_planner/static_data.h>
#include <cellworld_planner/lppo.h>

using namespace std;
using namespace json_cpp;
using namespace cell_world;
using namespace easy_tcp;
using namespace params_cpp;
using namespace robot;
using namespace experiment;
using namespace controller;

struct Robot_experiment_client : Experiment_client {
    void on_experiment_started(const Start_experiment_response &experiment) {
        experiment_occlusions[experiment.experiment_name] = experiment.world.occlusions;
    }
    void on_episode_started(const std::string &experiment_name) {
        auto occlusions = Resources::from("cell_group").
                key("hexagonal").
                key(experiment_occlusions[experiment_name]).
                key("occlusions").get_resource<Cell_group_builder>();
        Robot_simulator::set_occlusions(occlusions);
    };
    std::map<string, string> experiment_occlusions;
};


planner::Static_data new_valid_simulation_data(const string &configuration, const string &occlusions){
    auto wc = Resources::from("world_configuration").key(configuration).get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key(configuration).key("canonical").get_resource<World_implementation>();
    auto world = World(wc,wi);
    planner::Static_data data(world);
    data.world_info.world_configuration = configuration;
    data.world_info.world_implementation = "canonical";
    data.world_info.occlusions = occlusions;
    auto occlusion_builder = Resources::from("cell_group").key(configuration).key(occlusions).key("occlusions").get_resource<Cell_group_builder>();
    data.world.set_occlusions(occlusion_builder);
    data.cells = data.world.create_cell_group();
    data.free_cells = data.cells.free_cells();
    data.graph = data.world.create_graph();
    data.paths = data.world.create_paths(Resources::from("paths").key(configuration).key(occlusions).key("astar").get_resource<Path_builder>());
    data.visibility = data.world.create_graph(Resources::from("graph").key(configuration).key(occlusions).key("cell_visibility").get_resource<Graph_builder>());
    data.predator_start_locations = data.world.create_cell_group(Resources::from("cell_group").key(configuration).key(occlusions).key("spawn_locations").get_resource<Cell_group_builder>());
    data.occluded_cells = data.cells.occluded_cells();
    data.map = Map(data.cells);
    data.inverted_visibility = data.visibility.invert();
    data.world_statistics = data.world.get_statistics(3);
    planner::Lppos lppos(data.world, data.world_statistics);
    data.lppos = data.world.create_cell_group(lppos.get_spatial_lppos(data.world.size() / 10));
    data.predator_moves = data.world.connection_pattern;
    data.possible_destinations = data.free_cells;
    auto min_distance = data.paths.get_steps(data.start_cell(),data.goal_cell()) / 2;
    for (int i=0;i<random_spawn_locations;i++) {
        while(true) {
            auto &cell = data.free_cells.random_cell();
            if (data.paths.get_steps(data.start_cell(),cell) >= min_distance) {
                data.predator_start_locations.add(cell);
                break;
            }
        }
    }
    data.options = planner::Options::get_options(data.paths,data.lppos);
    return data;
}

int main(int argc, char *argv[])
{
    Key spawn_coordinates_key{"-s","--spawn_coordinates"};
    Key rotation_key{"-r","--theta"};
    Key prey_key{"-p","--prey"};
    Key interval_key{"-i","--interval"};
    Key noise_key{"-n","--noise"};

    Parser p(argc, argv);
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("canonical").get_resource<World_implementation>();
    auto capture_parameters = Resources::from("capture_parameters").key("default").get_resource<Capture_parameters>();
    auto peeking_parameters = Resources::from("peeking_parameters").key("default").get_resource<Peeking_parameters>();


    World world(wc, wi);
    Capture capture(capture_parameters, world);
    Peeking peeking(peeking_parameters, world);
    Cell_group cells = world.create_cell_group();
    Map map(cells);
    Location_visibility visibility(cells, wc.cell_shape, wi.cell_transformation);

    auto rotation = stof(p.get(rotation_key,"1.5707963267948966"));
    auto interval = stoi(p.get(interval_key,"30"));
    auto spawn_coordinates_str = p.get(spawn_coordinates_key, "{\"x\":4,\"y\":0}");
    auto verbose = p.contains(Key("-v"));

    Experiment_service::set_logs_folder("experiment_logs/");
    Experiment_server experiment_server;

    auto &controller_experiment_client = experiment_server.create_local_client<Controller_server::Controller_experiment_client>();
    controller_experiment_client.subscribe();

    auto &prey_controller_experiment_client = experiment_server.create_local_client<Prey_controller_server::Controller_experiment_client>();
    prey_controller_experiment_client.subscribe();


    if (!p.contains(noise_key)){
        Robot_simulator::tracking_server.noise = 0;
        Robot_simulator::tracking_server.frame_drop = 0;
        Robot_simulator::tracking_server.bad_reads = 0;
    }
    auto &experiment_tracking_client = Robot_simulator::tracking_server.create_local_client<Experiment_tracking_client>();
    experiment_tracking_client.subscribe();
    experiment_server.set_tracking_client(experiment_tracking_client);
    experiment_server.start(Experiment_service::get_port());


    auto &tracking_client = Robot_simulator::tracking_server.create_local_client<Controller_server::Controller_tracking_client>(visibility, float(90), capture, peeking, "predator", "prey");

    auto &experiment_client= experiment_server.create_local_client<Robot_experiment_client>();
    experiment_client.subscribe();


    auto &prey_tracking_client = Robot_simulator::tracking_server.create_local_client<Prey_controller_server::Controller_tracking_client>(visibility, float(270), capture, peeking,"prey", "predator");
    Coordinates prey_spawn_coordinates(-20,0); // TODO: change this back to -20, 0

    Coordinates spawn_coordinates;
    cout << spawn_coordinates_str << endl;
    try {
        spawn_coordinates = json_cpp::Json_create<Coordinates>(spawn_coordinates_str);
    } catch (...) {
        cout << "Wrong parameters "<< endl;
        exit(1);
    }
    Location location = map[spawn_coordinates].location;
    Location prey_location = map[prey_spawn_coordinates].location;
    Robot_simulator::start_simulation(world, location, rotation, prey_location, rotation, interval);

    Robot_simulator_server server;
    if (!server.start(Robot_agent::port())) {
        std::cout << "Server setup failed " << std::endl;
        return EXIT_FAILURE;
    }

    Agent_operational_limits limits;
    limits.load("../config/robot_simulator_operational_limits.json");
    Robot_agent robot(limits);
    robot.connect("127.0.0.1");
    Controller_service::set_logs_folder("controller_logs/");
    Controller_server controller_server("../config/pid.json", robot, tracking_client, controller_experiment_client);
    if (!controller_server.start(Controller_service::get_port())) {
        cout << "failed to start predator controller" << endl;
        exit(1);
    }


    Prey_robot_simulator_server prey_server;
    if (!prey_server.start(Tick_robot_agent::port())) {
        std::cout << "Prey server setup failed " << std::endl;
        return EXIT_FAILURE;
    }

    Tick_robot_agent prey_robot;
    prey_robot.connect("127.0.0.1");
    Prey_controller_server prey_controller_server(prey_robot, prey_tracking_client, prey_controller_experiment_client);
    if (!prey_controller_server.start(Prey_controller_service::get_port())) {
        cout << "failed to start prey controller" << endl;
        exit(1);
    }

    auto &tracker = Robot_simulator::tracking_server.create_local_client<agent_tracking::Tracking_client>();
    tracker.connect();
    tracker.subscribe();

    Timer update(.5);
    auto data = new_valid_simulation_data("hexagonal", "21_05");
    planner::Predator predator(data);
    planner::Prey planning_prey(data, predator);
    Model_public_state mps;
    mps.iterations = 500;
    mps.current_turn = 0;
    mps.status = cell_world::Model_public_state::Running;
    auto &prey_state = mps.agents_state.emplace_back();
    prey_state.agent_index = 0;
    prey_state.iteration = 0;
    auto &predator_state = mps.agents_state.emplace_back();
    predator_state.agent_index = 1;
    predator_state.iteration = 0;
    planning_prey.start_episode();
    while (Robot_simulator::is_running()) {
        if (update.time_out()) {
            update.reset();
            if (tracker.contains_agent_state("predator")) {
                if (verbose) {
                    cout << "track: " << tracker.get_current_state("predator") << endl;
                }
            }
            if (tracker.contains_agent_state("prey")) {
                if (verbose) {
                    cout << "track: " << tracker.get_current_state("prey") << endl;
                }
            }
        }
        if (prey_robot.is_ready()){
            //time to plan
            //first create observation
            prey_state.cell = data.map[prey_robot.current_coordinates];
            predator_state.cell = data.cells[data.cells.find(tracker.get_current_state("predator").location)];
            predator_state.iteration = prey_state.iteration++;
            mps.current_turn = 0;
            auto move=planning_prey.get_move(mps);
            prey_robot.execute_move(move);
            mps.current_turn = 1;
            auto status=planning_prey.update_state(mps);
        }
    }
    server.stop();
    return 0;
}
