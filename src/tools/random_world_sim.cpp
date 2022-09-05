#include <cell_world.h>
#include <params_cpp.h>
#include <cellworld_planner/static_data.h>
#include <cellworld_planner/lppo.h>
#include <cellworld_planner/prey.h>
#include <cellworld_planner/simulation.h>
#include <gauges.h>
#include <ctime>
#include <mutex>
#include <queue>
#include <filesystem>
#include <cellworld_planner/statistics.h>
#include <thread_pool.h>
#include <cellworld_planner/thig_prey.h>
#include <performance.h>

namespace fs = std::filesystem;
using namespace thread_pool;
using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;
using namespace gauges;

Timer ts;

bool verbose = false;

struct Simulation_data : Static_data {
    Simulation_data(World world) : Static_data(world){
        valid = new atomic<bool>;
        start_cell_id = 0;
        goal_cell_id = world.size()-1;
    }
    int creation_seed = -1;
    atomic<bool> *valid{};
};

void run_planning_episode( const Static_data &data,
                     Simulation_episode &episode,
                     unsigned int seed,
                     Gauge *pb) {
    PERF_SCOPE("run_planning_episode");
    Chance::seed(seed);
    Model model(data.cells, data.simulation_parameters.steps);
    Predator predator(data);
    Prey prey(data, predator);
    model.add_agent(prey);
    model.add_agent(predator);
    model.start_episode();
    while (model.update() && model.update()) {
        if (verbose) {
            pb->set_status(" step " + to_string(prey.public_state().iteration) + " of " + to_string(data.simulation_parameters.steps));
            pb->tick();
        }
    }
    int result = 0;
    if (prey.public_state().cell==data.goal_cell()){
        result = 1;
    } else {
        if (prey.public_state().iteration == data.simulation_parameters.steps ){
            result = 2;
        } else
        {
            result = 3;
        }
    }
    if (verbose) pb->set_status(" saving results...");
    model.end_episode();
    for (auto &dp:prey.mcts.history){
        auto &step = episode.emplace_back();
        step.prey_state = dp.prey_state;
        step.predator_state = dp.predator_state;
    }
    if (verbose) {
        switch (result) {
            case 1:
                pb->set_status(" survived");
                break;
            case 2:
                pb->set_status(" timed out");
                break;
            case 3:
                pb->set_status(" died");
                break;
        }
        pb->complete();
    }
}

Simulation run_planning_simulation_st ( const Simulation_data &lppo_data, int seed_start, int seed_end, bool use_lppos = true ) {
    PERF_SCOPE("run_planning_simulation");
    Static_data data = (Static_data)lppo_data;
    if (!use_lppos){
        data.lppos = data.free_cells;
        data.options = data.graph;
    }
    Simulation simulation;
    simulation.world_info = data.world_info;
    simulation.parameters = data.simulation_parameters;
    simulation.episodes.reserve(seed_end-seed_start);
    Gauges *progress;
    Gauge *gauge;
    if (verbose) {
        progress = new Gauges(seed_end - seed_start);
        progress->auto_refresh_start(250);
        gauge = new Gauge();
        gauge->set_total_work(data.simulation_parameters.steps);
    }
    for (unsigned int s = seed_start;s < seed_end;s++) {
        Gauge *bar = nullptr;
        if (verbose) {
            auto title = "Episode " + to_string(s) + ": ";
            bar = &progress->add_gauge(*gauge);
            bar->set_title(title);
        }
        auto &episode = simulation.episodes.emplace_back();
        run_planning_episode((Static_data &) data, episode, s, bar);
    }
    if (verbose) {
        progress->auto_refresh_stop();
        free(progress);
        free(gauge);
    }
    return simulation;
}

Simulation run_planning_simulation ( const Simulation_data &lppo_data, int seed_start, int seed_end, bool use_lppos = true ) {
    PERF_SCOPE("run_planning_simulation");
    Static_data data = (Static_data)lppo_data;
    if (!use_lppos){
        data.lppos = data.free_cells;
        data.options = data.graph;
    }
    Simulation simulation;
    simulation.world_info = data.world_info;
    simulation.parameters = data.simulation_parameters;
    simulation.episodes.reserve(seed_end-seed_start);
    Thread_pool tp;
    mutex mtx;
    Gauges *progress;
    Gauge *gauge;
    if (verbose) {
        progress = new Gauges(seed_end - seed_start);
        progress->auto_refresh_start(250);
        gauge = new Gauge();
        gauge->set_total_work(data.simulation_parameters.steps);
    }
    for (unsigned int s = seed_start;s < seed_end;s++) {
        Gauge *bar = nullptr;
        if (verbose) {
            auto title = "Episode " + to_string(s) + ": ";
            bar = &progress->add_gauge(*gauge);
            bar->set_title(title);
        }
        auto &episode = simulation.episodes.emplace_back();
        tp.run([ &progress, &data, &bar](Simulation_episode &episode, unsigned int s) {
            run_planning_episode((Static_data &) data, episode, s, bar);
        }, ref(episode), s);
    }
    tp.wait_all();
    if (verbose) {
        progress->auto_refresh_stop();
        free(progress);
        free(gauge);
    }
    return simulation;
}

Cell_group_builder random_cells(int count, int cell_count){
    Cell_group_builder res;
    for (int i = 0; i < count; i++){
        int x = Chance::dice(cell_count);
        while(find(res.begin(), res.end(), x) != res.end())
            x = Chance::dice(cell_count);
        res.push_back(x);
    }
    return res;
}


void create_random_world(Simulation_data &data, int seed, int occlusion_count ){
    static mutex mtx;
    if (seed == -1){
        seed = time(0)*thread_pool::worker_id;
    }
    Chance::seed(seed);
    World worker_world = data.world;
    const Cell &start_cell = data.start_cell();
    const Cell &goal_cell = data.goal_cell();
    while (!*(data.valid)) {
        auto occlusions = random_cells(occlusion_count, worker_world.size());
        worker_world.set_occlusions(occlusions);
        auto graph = worker_world.create_graph();
        if (graph.is_connected(start_cell, goal_cell)) {
            mtx.lock();
            if (!*(data.valid)) {
                data.world.set_occlusions(occlusions);
                data.creation_seed = seed;
                *(data.valid) = true;
                for (auto &cell:data.world.cells){
                    if (!cell.occluded) {
                        if (!graph.is_connected(start_cell, cell)) {
                            cell.occluded = true;
                        }
                    }
                }
            }
            mtx.unlock();
        }
    }
}

Simulation_data new_valid_simulation_data(const string &configuration, const string &occlusions, int random_spawn_locations){
    auto wc = Resources::from("world_configuration").key(configuration).get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key(configuration).key("canonical").get_resource<World_implementation>();
    auto world = World(wc,wi);
    thread_pool::Thread_pool tp;
    Simulation_data data(world);
    data.world_info.world_configuration = configuration;
    data.world_info.world_implementation = "canonical";

    if (occlusions.empty()) {
        int occlusion_count = Chance::dice(world.size() / 2);
        if (verbose) cout << ts.to_seconds() << ": looking for a valid world with " << occlusion_count << " occlusions" << endl;
        for (int wid = 0; wid < tp.workers.size(); wid++) {
            int seed = time(0);
            tp.run(create_random_world, ref(data), seed, occlusion_count);
        }
        tp.wait_all();
        // at this point a valid world was found
        data.world_info.occlusions = "random_world_" + to_string(data.creation_seed);
        data.cells = data.world.create_cell_group();
        data.free_cells = data.cells.free_cells();
        if (verbose) cout << ts.to_seconds() << ": creating paths " << endl;
        data.graph = data.world.create_graph();
        tp.run([&data](){data.paths = Paths::get_astar(data.graph);});
        tp.run([&data](){data.visibility = Coordinates_visibility::create_graph(data.cells, data.world.cell_shape, data.world.cell_transformation);});
        if (random_spawn_locations == 0) random_spawn_locations = 10;
    } else {
        data.world_info.occlusions = occlusions;
        if (verbose) cout << ts.to_seconds() << ": loading occlusions from " << configuration << "_" << occlusions << endl;
        auto occlusion_builder = Resources::from("cell_group").key(configuration).key(occlusions).key("occlusions").get_resource<Cell_group_builder>();
        data.world.set_occlusions(occlusion_builder);
        data.cells = data.world.create_cell_group();
        data.free_cells = data.cells.free_cells();
        data.graph = data.world.create_graph();
        tp.run([&data, configuration, occlusions](){
            data.paths = data.world.create_paths(Resources::from("paths").key(configuration).key(occlusions).key("astar").get_resource<Path_builder>());
        });
        tp.run([&data, configuration, occlusions](){
            data.visibility = data.world.create_graph(Resources::from("graph").key(configuration).key(occlusions).key("cell_visibility").get_resource<Graph_builder>());
        });
        if (random_spawn_locations==0)
        tp.run([&data, configuration, occlusions](){
            data.predator_start_locations = data.world.create_cell_group(Resources::from("cell_group").key(configuration).key(occlusions).key("spawn_locations").get_resource<Cell_group_builder>());
        });
    }
    data.occluded_cells = data.cells.occluded_cells();
    data.map = Map(data.cells);
    data.inverted_visibility = data.visibility.invert();
    data.world_statistics = data.world.get_statistics(3);
    Lppos lppos(data.world, data.world_statistics);
    data.lppos = data.world.create_cell_group(lppos.get_spatial_lppos(data.world.size() / 10));
    data.predator_moves = data.world.connection_pattern;
    data.possible_destinations = data.free_cells;
    tp.wait_all();
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
    if (verbose) cout << ts.to_seconds() << ": creating options " << endl;
    data.options = Options::get_options(data.paths,data.lppos);
    if (verbose) cout << ts.to_seconds() << ": Done! " << endl;
    return data;
}

Simulation run_fixed_trajectory_simulation(Simulation &planning_simulation, Simulation_data &data, int seed_start, int seed_end){
    //collect successful trajectories
    PERF_SCOPE("run_fixed_trajectory_simulation");
    vector<Move_list> successful_trajectories;
    int survived=0, died=0, timed_out=0;
    for(auto &sim_episode:planning_simulation.episodes){
        if (sim_episode.back().prey_state.cell_id == data.goal_cell().id) {
            auto current_coordinates = data.goal_cell().coordinates;
            auto &successful_trajectory = successful_trajectories.emplace_back();
            for (auto &step:sim_episode){
                auto &step_cell = data.cells[step.prey_state.cell_id];
                if ( step_cell.coordinates != current_coordinates){
                    auto move = step_cell.coordinates - current_coordinates;
                    current_coordinates = step_cell.coordinates;
                    successful_trajectory.push_back(move);
                }
            }
        }
    }
    Gauge *progress;
    Simulation fixed_trajectory_simulation;
    fixed_trajectory_simulation.world_info = data.world_info;
    fixed_trajectory_simulation.parameters = data.simulation_parameters;
    if (verbose) {
        progress = new Gauge();
        progress->set_title("Fixed trajectory");
        progress->set_total_work(seed_end - seed_start);
    }
    Model model(data.cells, data.simulation_parameters.steps);
    Predator predator(data);
    Dummy prey(data);
    prey.start_cell_id = data.start_cell().id;
    model.add_agent(prey);
    model.add_agent(predator);
    Cell_capture capture(data.simulation_parameters.capture_parameters, data.visibility);
    for (int seed = seed_start; seed < seed_end; seed++){
        auto &episode = fixed_trajectory_simulation.episodes.emplace_back();
        if (verbose) {
            progress->tick();
        }
        Chance::seed(seed);
        auto &trajectory = pick_random(successful_trajectories);
        model.start_episode();
        for (auto move:trajectory){
            prey.next_move = move;
            if (!model.update()) break;
            auto &prey_step = episode.emplace_back();
            prey_step.prey_state.cell_id = prey.public_state().cell.id;
            prey_step.predator_state = predator.internal_state();
            if (capture.is_captured(predator.public_state().cell, prey.public_state().cell)) {
                prey_step.prey_state.capture = true;
                break;
            }
            if (prey.public_state().cell==data.goal_cell()) {
                break;
            }
            if (!model.update()) break;
            auto &predator_step = episode.emplace_back();
            predator_step.prey_state.cell_id = prey.public_state().cell.id;
            predator_step.predator_state = predator.internal_state();
            if (capture.is_captured(predator.public_state().cell, prey.public_state().cell)) {
                predator_step.prey_state.capture = true;
                break;
            }
            if (prey.public_state().cell==data.goal_cell()) {
                break;
            }
        }
        if (prey.public_state().cell==data.goal_cell()){
            survived ++;
        } else {
            if (prey.public_state().iteration == data.simulation_parameters.steps ){
                timed_out++;
            } else
            {
                died++;
            }
        }
        model.end_episode();
        if (verbose) {
            progress->set_status(" survived: " + to_string(survived) +
                                " died: " + to_string(died) +
                                " timed_out: " + to_string(timed_out));
            cout << progress;
        }
    }
    if (verbose) {
        progress->complete();
        cout << endl;
        free(progress);
    }
    return fixed_trajectory_simulation;
}

Simulation run_shortest_path_simulation(Simulation_data &data, int seed_start, int seed_end){
    //collect successful trajectories
    PERF_SCOPE("run_shortest_path_simulation");
    int survived=0, died=0, timed_out=0;
    Gauge *progress;
    if (verbose){
        progress = new Gauge();
        progress->set_title("Shortest path");
        progress->set_total_work( seed_end - seed_start);
    }
    Simulation fixed_trajectory_simulation;
    fixed_trajectory_simulation.world_info = data.world_info;
    fixed_trajectory_simulation.parameters = data.simulation_parameters;
    Model model(data.cells, data.simulation_parameters.steps);
    Predator predator(data);
    Dummy prey(data);
    prey.start_cell_id = data.start_cell().id;
    model.add_agent(prey);
    model.add_agent(predator);
    Cell_capture capture(data.simulation_parameters.capture_parameters, data.visibility);
    for (int seed = seed_start; seed < seed_end; seed++){
        auto &episode = fixed_trajectory_simulation.episodes.emplace_back();
        if (verbose) progress->tick();
        Chance::seed(seed);
        model.start_episode();
        prey.next_move = data.paths.get_move(prey.public_state().cell, data.goal_cell());
        while (model.update()){
            prey.next_move = data.paths.get_move(prey.public_state().cell, data.goal_cell());
            auto &prey_step = episode.emplace_back();
            prey_step.prey_state.cell_id = prey.public_state().cell.id;
            prey_step.predator_state = predator.internal_state();
            if (capture.is_captured(predator.public_state().cell, prey.public_state().cell)) {
                prey_step.prey_state.capture = true;
                break;
            }
            if (prey.public_state().cell==data.goal_cell()) {
                break;
            }
        }
        if (prey.public_state().cell==data.goal_cell()){
            survived ++;
        } else {
            if (prey.public_state().iteration == data.simulation_parameters.steps ){
                timed_out++;
            } else
            {
                died++;
            }
        }
        model.end_episode();
        if (verbose) {
            progress->set_status(" survived: " + to_string(survived) +
                                " died: " + to_string(died) +
                                " timed_out: " + to_string(timed_out));
            cout << progress;
        }
    }
    if (verbose) {
        progress->complete();
        cout << endl;
        free(progress);
    }
    return fixed_trajectory_simulation;
}

Simulation run_thigmotaxis_simulation (Simulation_data &data, int seed_start, int seed_end, bool reactive){
    PERF_SCOPE("run_thigmotaxis_simulation");
    int survived=0, died=0, timed_out=0;
    Simulation simulation;
    simulation.world_info = data.world_info;
    simulation.parameters = data.simulation_parameters;
    simulation.episodes.reserve(seed_end-seed_start);
    Gauge *progress;
    if (verbose) {
        progress = new Gauge();
        if (reactive)
            progress->set_title("Reactive thigmotaxis");
        else
            progress->set_title("Thigmotaxis");
        progress->set_total_work( seed_end - seed_start);
    }
    Model model(data.cells, 1000);
    Predator predator(data);
    Thig_prey prey(data, predator);
    prey.reactive = reactive;
    model.add_agent(prey);
    model.add_agent(predator);
    Cell_capture capture(data.simulation_parameters.capture_parameters, data.visibility);
    for (unsigned int seed = seed_start; seed < seed_end; seed++) {
        if (verbose) progress->tick();
        auto &episode = simulation.episodes.emplace_back();
        Chance::seed(seed);
        model.start_episode();
        while (model.update()){
            auto &prey_step = episode.emplace_back();
            prey_step.prey_state.cell_id = prey.public_state().cell.id;
            prey_step.predator_state = predator.internal_state();
            if (capture.is_captured(predator.public_state().cell, prey.public_state().cell)) {
                prey_step.prey_state.capture = true;
                break;
            }
            if (prey_step.prey_state.cell_id==data.goal_cell().id) {
                break;
            }
        }
        if (prey.public_state().cell==data.goal_cell()){
            survived ++;
        } else {
            if (prey.public_state().iteration == data.simulation_parameters.steps ){
                timed_out++;
            } else
            {
                died++;
            }
        }
        model.end_episode();
        if (verbose) {
            progress->set_status(" survived: " + to_string(survived) +
                                " died: " + to_string(died) +
                                " timed_out: " + to_string(timed_out));
            cout << progress;
        }
    }
    if (verbose) {
        progress->complete();
        cout << endl;
        free(progress);
    }
    return simulation;
}


int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto configuration = p.get(Key("-wc", "--world_configuration"), "hexagonal");
    auto occlusions = p.get(Key("-o", "--occlusions"), "");
    auto reward = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/reward/" + p.get(Key("-r", "--reward"), "reward1");
    auto episode_count = stoi(p.get(Key("-e", "--episode_count"), "100"));
    auto steps = stoi (p.get(Key("-sc", "--step_count"), "50"));
    auto tree_search_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/tree_search_parameters/" + p.get(Key("-t", "--tree_search_parameters"), "1000");
    auto predator_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/predator_parameters/" + p.get(Key("-p", "--predator_parameters"), "fast_25_randomness");
    auto prey_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/prey_parameters/" + p.get(Key("-y", "--prey_parameters"), "default");
    auto capture_parameters = get_variable("CELLWORLD_PLANNER_CONFIG","../config") + "/capture_parameters/" + p.get(Key("-c", "--capture_parameters"), "small");
    auto random_spawn_locations = stoi (p.get(Key("-s", "--random_spawn_locations"), "0"));
    auto output_folder = p.get(Key("-out", "--output_folder"), "");
    verbose = p.contains(Key("-v", "--verbose"));
    bool single_tread = p.contains(Key("-st", "--single_thread"));
    int start_seed = 0;
    int end_seed = episode_count;

    bool run_planning = p.contains(Key("-rp")) || p.contains(Key("-ra"));
    bool run_fixed_trajectory = p.contains(Key("-rf")) || p.contains(Key("-ra"));
    bool run_lppo_planning = p.contains(Key("-rl")) || p.contains(Key("-ra"));
    bool run_fixed_lppo_trajectory = p.contains(Key("-rfl")) || p.contains(Key("-ra"));
    bool run_shortest_path = p.contains(Key("-rs")) || p.contains(Key("-ra"));
    bool run_thigmotaxis = p.contains(Key("-rt")) || p.contains(Key("-ra"));
    bool run_reactive_thigmotaxis = p.contains(Key("-rr")) || p.contains(Key("-ra"));

    if ( output_folder == "" ){
        output_folder = get_variable("CELLWORLD_PLANNER_RESULTS","../simulation_results") + "/random_world/" + configuration + "." + json_cpp::Json_date::now().to_string("%Y%m%d_%H%M");
    }
    auto simulation_data = new_valid_simulation_data(configuration, occlusions, random_spawn_locations);
    simulation_data.simulation_parameters.reward.load(reward);
    simulation_data.simulation_parameters.tree_search_parameters.load(tree_search_parameters);
    simulation_data.simulation_parameters.predator_parameters.load(predator_parameters);
    simulation_data.simulation_parameters.prey_parameters.load(prey_parameters);
    simulation_data.simulation_parameters.capture_parameters.load(capture_parameters);
    simulation_data.simulation_parameters.steps = steps;

    Simulation planning_simulation;
    Simulation fixed_trajectory_simulation;
    Simulation lppo_planning_simulation;
    Simulation fixed_lppo_trajectory_simulation;
    Simulation shortest_path_simulation;
    Simulation thigmotaxis_simulation;
    Simulation reactive_thigmotaxis_simulation;

    if (run_planning) {
        if (verbose) cout << "Running Planning Simulation" << endl;
        if (single_tread)
            planning_simulation = run_planning_simulation_st(simulation_data, start_seed, end_seed, false);
        else
            planning_simulation = run_planning_simulation(simulation_data, start_seed, end_seed, false);
        if (verbose) cout << endl;
    }
    if (run_lppo_planning) {
        if (verbose) cout << "Running LPPO Planning Simulation" << endl;
        if (single_tread)
            lppo_planning_simulation = run_planning_simulation_st(simulation_data, start_seed, end_seed, true);
        else
            lppo_planning_simulation = run_planning_simulation(simulation_data, start_seed, end_seed, true);
        if (verbose) cout << endl;
    }

    if (run_shortest_path) {
        shortest_path_simulation = run_shortest_path_simulation(simulation_data, start_seed, end_seed);
    }

    if (run_thigmotaxis) {
        thigmotaxis_simulation = run_thigmotaxis_simulation(simulation_data, start_seed, end_seed, false);
    }

    if (run_reactive_thigmotaxis ) {
        reactive_thigmotaxis_simulation = run_thigmotaxis_simulation(simulation_data, start_seed, end_seed, true);
    }
    if (verbose) cout << "Saving Results... " << endl;
    fs::create_directories(output_folder);
    //save world and simulation parameters first
    simulation_data.occluded_cells.get_builder().save(output_folder + "/occlusions");
    simulation_data.lppos.get_builder().save(output_folder + "/lppos");
    simulation_data.world_statistics.save(output_folder + "/world_statistics");
    simulation_data.simulation_parameters.save(output_folder + "/simulation_parameters");
    //save simulation and statistics

    float survival_rate_planning = 0;
    float survival_rate_fixed_trajectory = 0;
    float survival_rate_lppo_planning = 0;
    float survival_rate_fixed_lppo_trajectory = 0;
    float survival_rate_shortest_path = 0;
    float survival_rate_thigmotaxis = 0;
    float survival_rate_reactive_thigmotaxis = 0;

    if (run_planning) {
        planning_simulation.save(output_folder + "/planning_simulation.json");
        Simulation_statistics planning_simulation_statistics(planning_simulation, simulation_data);
        planning_simulation_statistics.save(output_folder + "/planning_simulation_stats.json");
        survival_rate_planning = planning_simulation_statistics.success_rate;
        if (run_fixed_trajectory && planning_simulation_statistics.success_rate > 0){
            fixed_trajectory_simulation = run_fixed_trajectory_simulation(planning_simulation, simulation_data, start_seed, end_seed);
            fixed_trajectory_simulation.save(output_folder + "/fixed_trajectory_simulation.json");
            Simulation_statistics fixed_trajectory_simulation_statistics(fixed_trajectory_simulation, simulation_data);
            survival_rate_fixed_trajectory = fixed_trajectory_simulation_statistics.success_rate;
            fixed_trajectory_simulation_statistics.save(output_folder + "/fixed_trajectory_simulation_stats.json");
        }
    }
    if (run_lppo_planning) {
        lppo_planning_simulation.save(output_folder + "/lppo_planning_simulation.json");
        Simulation_statistics lppo_planning_simulation_statistics(lppo_planning_simulation, simulation_data);
        lppo_planning_simulation_statistics.save(output_folder + "/lppo_planning_simulation_stats.json");
        survival_rate_lppo_planning = lppo_planning_simulation_statistics.success_rate;
        if (run_fixed_lppo_trajectory && lppo_planning_simulation_statistics.success_rate > 0){
            fixed_trajectory_simulation = run_fixed_trajectory_simulation(lppo_planning_simulation, simulation_data, start_seed, end_seed);
            fixed_trajectory_simulation.save(output_folder + "/fixed_lppo_trajectory_simulation.json");
            Simulation_statistics fixed_trajectory_simulation_statistics(fixed_trajectory_simulation, simulation_data);
            survival_rate_fixed_lppo_trajectory = fixed_trajectory_simulation_statistics.success_rate;
            fixed_trajectory_simulation_statistics.save(output_folder + "/fixed_lppo_trajectory_simulation_stats.json");
        }
    }
    if (run_shortest_path) {
        shortest_path_simulation.save(output_folder + "/shortest_path_simulation.json");
        Simulation_statistics shortest_path_simulation_statistics(shortest_path_simulation, simulation_data);
        survival_rate_shortest_path = shortest_path_simulation_statistics.success_rate;
        shortest_path_simulation_statistics.save(output_folder + "/shortest_path_simulation_stats.json");
    }
    if (run_thigmotaxis) {
        thigmotaxis_simulation.save(output_folder + "/thigmotaxis_simulation.json");
        Simulation_statistics thigmotaxis_simulation_statistics(thigmotaxis_simulation, simulation_data);
        survival_rate_thigmotaxis = thigmotaxis_simulation_statistics.success_rate;
        thigmotaxis_simulation_statistics.save(output_folder + "/thigmotaxis_simulation_stats.json");
    }
    if (run_reactive_thigmotaxis) {
        reactive_thigmotaxis_simulation.save(output_folder + "/reactive_thigmotaxis_simulation.json");
        Simulation_statistics reactive_thigmotaxis_simulation_statistics(reactive_thigmotaxis_simulation, simulation_data);
        survival_rate_reactive_thigmotaxis = reactive_thigmotaxis_simulation_statistics.success_rate;
        reactive_thigmotaxis_simulation_statistics.save(output_folder + "/reactive_thigmotaxis_simulation_stats.json");
    }
    if (verbose) {
        cout << "Survival rates:" << endl;
        if (run_planning) {
            cout << " - Planning: " << survival_rate_planning << endl;
            if (run_fixed_trajectory) {
                cout << " - Fixed trajectory: " << survival_rate_fixed_trajectory << endl;
            }
        }
        if (run_lppo_planning) {
            cout << " - LPPO Planning: " << survival_rate_lppo_planning << endl;
            if (run_fixed_lppo_trajectory) {
                cout << " - Fixed LPPO trajectory: " << survival_rate_fixed_lppo_trajectory << endl;
            }
        }
        if (run_shortest_path) cout << " - Shortest path: " << survival_rate_shortest_path << endl;
        if (run_thigmotaxis) cout << " - Thigmotaxis: " << survival_rate_thigmotaxis << endl;
        if (run_reactive_thigmotaxis) cout << " - Reactive thigmotaxis: " << survival_rate_reactive_thigmotaxis << endl;
        cout << endl << "output folder " << output_folder << endl;
    }
}