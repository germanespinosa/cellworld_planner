#include <chrono>
#include <thread>
#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <thread_pool.h>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;
using namespace thread_pool;

void run_simulation( const Static_data &data,
                     Simulation_episode &episode,
                     unsigned int seed) {
    Chance::seed(seed);
    Model model(data.cells, 1000);
    Predator predator(data);
    Prey prey(data, predator);
    model.add_agent(prey);
    model.add_agent(predator);
    model.run();
    for (auto &dp:prey.mcts.history){
        auto &step = episode.emplace_back();
        step.prey_state = dp.prey_state;
        step.predator_state = dp.predator_state;
    }
}

int main(int argc, char **argv){
    Parser p(argc,argv);
    Simulation_parameters parameters;

    auto configuration_file = p.get(Key("-cf","--configuration_file"),"");
    if (!configuration_file.empty()){
        parameters = json_cpp::Json_from_file<Simulation_parameters>(configuration_file);
    }
    auto reward_file = p.get(Key("-rf","--reward_file"),"");
    if (!reward_file.empty()){
        parameters.reward = json_cpp::Json_from_file<Reward>(reward_file);
    }
    auto tree_search_file = p.get(Key("-tf","--tree_search_file"),"");
    if (!tree_search_file.empty()){
        parameters.tree_search_parameters = json_cpp::Json_from_file<Tree_search_parameters>(tree_search_file);
    }
    auto tree_search_simulations = stoi(p.get(Key("-tss","--tree_search_simulations"),"-1"));
    if (tree_search_simulations != -1) {
        parameters.tree_search_parameters.simulations = tree_search_simulations;
    }
    auto tree_search_depth = stoi(p.get(Key("-tsd","--tree_search_depth"),"-1"));
    if (tree_search_depth != -1) {
        parameters.tree_search_parameters.depth = tree_search_depth;
    }
    auto predator_file = p.get(Key("-prf","--predator_file"),"");
    if (!predator_file.empty()){
        parameters.predator_parameters = json_cpp::Json_from_file<Predator_parameters>(predator_file);
    }
    auto prey_file = p.get(Key("-pyf","--prey_file"),"");
    if (!prey_file.empty()){
        parameters.prey_parameters = json_cpp::Json_from_file<Prey_parameters>(prey_file);
    }
    auto step = stoi(p.get(Key("-s","--step"),"-1"));
    if (step!=-1){
        parameters.steps = step;
    }
    auto results_file = p.get(Key("-r","--results_file"),"");
    auto survival_results_file = p.get(Key("-sr","--survival_results_file"),"");
    auto occlusions = p.get(Key("-o","--occlusions"),"20_05");
    auto seed_start = stoi(p.get(Key("-ss","--seed_start"),"0"));
    auto seed_end = stoi(p.get(Key("-se","--seed_end"),"0"));
    auto workers = stoi(p.get(Key("-w","--workers"),"-1"));
    auto world = World::get_from_parameters_name("hexagonal","canonical",occlusions);

    if (workers==-1) workers = (int)Thread_pool::max_concurrency();
    Thread_pool tp(workers);

    World_info world_info;
    world_info.world_configuration = "hexagonal";
    world_info.world_implementation = "canonical";
    world_info.occlusions = occlusions;

    Static_data data(world_info);
    data.predator_moves = world.connection_pattern;
    data.predator_moves.push_back(Coordinates(0,0));
    data.simulation_parameters = parameters;

    Simulation simulation;
    simulation.world_info.world_configuration = "hexagonal";
    simulation.world_info.world_implementation = "canonical";
    simulation.world_info.occlusions = occlusions;
    simulation.parameters = data.simulation_parameters;
    simulation.episodes.reserve(seed_end-seed_start);
    for (unsigned int s = seed_start;s < seed_end;s++) {
        cout << "Processing seed " << s << endl;
        auto &episode = simulation.episodes.emplace_back();
        tp.run( run_simulation,
                std::ref(data),
                std::ref(episode),
                s);
    }
    tp.wait_all();
    auto survival_rate = simulation.episodes.count([](const Simulation_episode  &e){ return e.back().prey_state.cell_id==330;});
    if (!survival_results_file.empty()) {
        Json_float_vector survival_rate_results;
        if (file_exists(survival_results_file)) {
            survival_rate_results = json_cpp::Json_from_file<Json_float_vector>(survival_results_file);
        }
        survival_rate_results.push_back((float) survival_rate / (float(simulation.episodes.size())));
        survival_rate_results.save(survival_results_file);
    }
    cout << "Survival rate: " << survival_rate << endl;
    if (!results_file.empty()) {
        simulation.save(results_file);
    }
}