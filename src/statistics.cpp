#include <cellworld_planner/statistics.h>

using namespace std;

namespace cell_world::planner {
    float get_decision_difficulty(Move &first, Move &second) {
        if (first == -second) return 3;
        auto m = first.manhattan(second);
        if (m == 4)return 2;
        return 1;
    }

    void populate_episode_stats ( Simulation_episode &sim_episode,
                                  Episode_statistics &episode_stats,
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
            step_stats.options = (float)sim_step.prey_state.options_values.size();
            if (!sim_step.prey_state.options_values.empty()) {
                step_stats.value = max(sim_step.prey_state.options_values);
            }
            episode_stats.value += step_stats.value / episode_stats.length;
            episode_stats.distance += (float)prey_cell.location.dist(predator_cell.location);
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
        episode_stats.time_out_rate = 0;
        episode_stats.success_rate = 0;
        if (episode_stats.capture_rate == 0){
            episode_stats.survival_rate = 1;
            if (sim_episode.back().prey_state.cell_id != data.goal_cell().id){
                episode_stats.time_out_rate = 1;
            } else {
                episode_stats.success_rate = 1;
            }
        } else {
            episode_stats.survival_rate = 0;
            if (!data.simulation_parameters.prey_parameters.terminate_on_capture) {
                if (sim_episode.back().prey_state.cell_id != data.goal_cell().id) {
                    episode_stats.time_out_rate = 1;
                }
            }
        }

    }

    void Simulation_statistics::update() {
        float episode_count =  (float) episode_stats.size();
        for (auto &episode_stats: episode_stats){
            capture_rate += episode_stats.capture_rate / episode_count;
            pursue_rate += episode_stats.pursue_rate / episode_count;
            survival_rate += episode_stats.survival_rate / episode_count;
            success_rate += episode_stats.success_rate / episode_count;
            time_out_rate += episode_stats.time_out_rate / episode_count;
            belief_state_entropy += episode_stats.belief_state_entropy / episode_count;
            distance += episode_stats.distance / episode_count;
            visited_cells += episode_stats.visited_cells / episode_count;
            length += episode_stats.length / episode_count;
            value += episode_stats.value / episode_count;
            decision_difficulty += episode_stats.decision_difficulty / episode_count;
        }
    }

    Simulation_statistics::Simulation_statistics(Simulation &simulation, Static_data &data)
    {
        for (auto &sim_episode:simulation.episodes) {
            auto &es = episode_stats.emplace_back();
            populate_episode_stats (sim_episode,es,data);
        }
        update();
    }
}