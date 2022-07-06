#include <cell_world.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/statistics.h>
#include <cellworld_planner/static_data.h>
#include <thread_pool.h>


using namespace thread_pool;
using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;

float logb(float a, float b)
{
    return log(a) / log(b);
}

float entropy(vector<float> probabilities, float base = M_E) {
    if (probabilities.empty()) return 0;
    float ent = 0;
    for (auto p : probabilities){
        if (p>0)  ent -= p * logb(p, base);
    }
    return ent / logb((float)probabilities.size(), base);
}

template<typename T>
float weights_entropy(vector<T> weights, float base = M_E) {
    if (weights.empty()) return 0;
    auto total = sum(weights);
    if (total == 0) return 0;
    vector<float> probs(weights.size());
    for (int i = 0 ; i < weights.size(); i++) probs[i] = (float)weights[i] / total ;
    return entropy(probs);
}

float get_decision_difficulty(Move &first, Move &second) {
    if (first == -second) return 3;
    auto m = first.manhattan(second);
    if (m == 4)return 2;
    return 1;
}

void create_episode_stats ( Simulation_episode &sim_episode,
                           Episode_statistics &episode_stats,
                           Reward &reward,
                           Static_data &data) {
    episode_stats.length = sim_episode.size();
    Cell_group visited_cells;
    for (auto &sim_step: sim_episode){
        auto &step_stats = episode_stats.steps_stats.emplace_back();
        auto &prey_cell = data.cells[sim_step.prey_state.cell_id];
        auto &predator_cell = data.cells[sim_step.predator_state.cell_id];
        if (sim_step.prey_state.capture){
            episode_stats.capture_rate += 1;
        }
        visited_cells.add(prey_cell);
        step_stats.value = max(sim_step.prey_state.options_values);
        episode_stats.value += step_stats.value / episode_stats.length;
        episode_stats.distance += prey_cell.location.dist(predator_cell.location);
        step_stats.belief_state_entropy = weights_entropy(sim_step.prey_state.belief_state);
        episode_stats.belief_state_entropy += step_stats.belief_state_entropy / episode_stats.length;
        if (sim_step.predator_state.behavior == cell_world::planner::Predator_state::Pursuing){
            episode_stats.pursue_rate += 1 / episode_stats.length ;
        }
        vector<float> aggregated_values(data.world.connection_pattern.size(), 0 );
        vector<int> aggregated_counters(data.world.connection_pattern.size(), 0 );
        auto options_cells = data.world.create_cell_group(sim_step.prey_state.options);
        for (auto &option_cell: options_cells) {
            auto move = data.paths.get_move(prey_cell, option_cell);
            for (int i = 0;i < data.world.connection_pattern.size();i++){
                if (move == data.world.connection_pattern[i]) {
                    aggregated_values[i] += sim_step.prey_state.options_values[i];
                    aggregated_counters[i] ++;
                }
            }
        }
        int best = -1;
        int worst = -1;
        for (int i = 0;i < data.world.connection_pattern.size();i++) {
            if (aggregated_counters[i]==0) continue;
            aggregated_values[i] = sim_step.prey_state.options_values[i] / (float) aggregated_counters[i];
            if (best == -1 || aggregated_values[i]>aggregated_values[best]) best = i;
            if (worst == -1 || aggregated_values[i]<aggregated_values[worst]) worst = i;
        }
        int second = -1;
        for (int i = 0;i < data.world.connection_pattern.size();i++) {
            if (aggregated_counters[i]==0) continue;
            if (i==best) continue;
            if (second == -1 || aggregated_values[i]>aggregated_values[second]) second = i;
        }
        if (second != -1) {
            auto range = abs(aggregated_values[best] - aggregated_values[worst]);
            auto first_diff =  abs(aggregated_values[best] - aggregated_values[second]);
            if (first_diff < .2 * range){
                step_stats.decision_difficulty = get_decision_difficulty (data.world.connection_pattern[best],data.world.connection_pattern[second]);
                episode_stats.decision_difficulty += step_stats.decision_difficulty / episode_stats.length;
            }
        }

    }
    episode_stats.visited_cells = visited_cells.size();
    if (episode_stats.capture_rate == 0){
        episode_stats.survival_rate = 1;
    }
    //episode_stats.value = reward.episode_reward - reward.step_cost * episode_stats.length - reward.capture_cost * episode_stats.capture_rate;
}

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
    Thread_pool tp;
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
    Static_data data(simulation.world_info);
    Simulation_statistics sim_stats;
    sim_stats.episode_stats = json_cpp::Json_vector<Episode_statistics> (simulation.episodes.size());
    for (int e = 0 ; e < simulation.episodes.size(); e++) {
        auto &episode_stats = sim_stats.episode_stats[e];
        cout << "Processing trajectories " << e << endl;
        auto &sim_episode = simulation.episodes[e];
//        create_episode_stats (
//               std::ref(sim_episode),
//               std::ref(episode_stats),
//               std::ref(simulation.parameters.reward),
//               std::ref(data));
        tp.run(create_episode_stats,
               std::ref(sim_episode),
               std::ref(episode_stats),
               std::ref(simulation.parameters.reward),
               std::ref(data));
    }
    tp.wait_all();
    float episode_count =  (float) simulation.episodes.size();
    for (auto &episode_stats: sim_stats.episode_stats){
        sim_stats.capture_rate += episode_stats.capture_rate / episode_count;
        sim_stats.pursue_rate += episode_stats.pursue_rate / episode_count;
        sim_stats.survival_rate += episode_stats.survival_rate / episode_count;
        sim_stats.belief_state_entropy += episode_stats.belief_state_entropy / episode_count;
        sim_stats.distance += episode_stats.distance / episode_count;
        sim_stats.visited_cells += episode_stats.visited_cells / episode_count;
        sim_stats.length += episode_stats.length / episode_count;
        sim_stats.value += episode_stats.value / episode_count;
        sim_stats.decision_difficulty += episode_stats.decision_difficulty / episode_count;
    }
    sim_stats.save(stats_file);
}