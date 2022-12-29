#include <cellworld_planner/tree_search.h>
#include <performance.h>

using namespace std;
using namespace cell_world;
using namespace json_cpp;

namespace cell_world::planner {

    void Tree_search::record(const cell_world::Model_public_state &state) {
        PERF_SCOPE("Tree_search::record");
        belief_state.record_state(state);
        auto prev_belief_state = history[history.size() - 1].prey_state.belief_state;
        auto &history_step = history.emplace_back(state);
        auto &prey_current_cell = state.agents_state[PREY].cell;
        auto &predator_current_cell = state.agents_state[PREDATOR].cell;
        history_step.predator_state = predator.internal_state();
        history_step.predator_state.cell_id = predator_current_cell.id;
        history_step.prey_state.capture = capture.is_captured(predator_current_cell, prey_current_cell);
        history_step.prey_state.cell_id = prey_current_cell.id;
        history_step.prey_state.belief_state = prev_belief_state;
    }

    Tree_search::Tree_search(const Static_data &data, Predator &predator) :
            data(data),
            predator(predator),
            belief_state(data),
            model(data.cells, data.simulation_parameters.steps),
            sim_predator(data),
            sim_prey(data),
            capture(data.simulation_parameters.capture_parameters, data.visibility) {
        model.add_agent(sim_prey);
        model.add_agent(sim_predator);
    }

    void Tree_search::reset() {
        belief_state.reset();
        history.clear();
    }

    Move Tree_search::get_best_move_ucb1(const Model_public_state &state) {
        PERF_SCOPE("Tree_search::get_best_move_ucb1");
        PERF_START("Tree_search::get_best_move_ucb1:start");
        belief_state.record_state(state);
        auto &history_step = history.emplace_back(state);
        auto &prey_current_cell = state.agents_state[PREY].cell;
        auto &predator_current_cell = state.agents_state[PREDATOR].cell;
        history_step.predator_state = predator.internal_state();
        history_step.predator_state.cell_id = predator_current_cell.id;
        history_step.prey_state.capture = capture.is_captured(predator_current_cell, prey_current_cell);
        history_step.prey_state.cell_id = prey_current_cell.id;
        history_step.prey_state.belief_state = Json_vector<unsigned int>(data.cells.size(), 0);
        for (auto &particle: belief_state.particles) {
            history_step.prey_state.belief_state[particle.public_state.agents_state[PREDATOR].cell.id]++;
        }
        PERF_STOP("Tree_search::get_best_move_ucb1:start");
        if (history_step.prey_state.capture) {
            return No_move;
        }
        Option root(prey_current_cell, data.options);
        root.load();
        if (belief_state.particles.empty()) {
            return data.paths.get_move(prey_current_cell, data.goal_cell());
        }
        PERF_START("Tree_search::get_best_move_ucb1:simulations");
        for (unsigned int s = 0; s < data.simulation_parameters.tree_search_parameters.simulations; s++) {
            auto *selected_option = &root.get_best_option(1);
            auto particle = belief_state.get_particle();
            float value = 0;
            float accum_gamma = 1;
            model.set_state(particle);
            unsigned int current_lppo_cell_id = selected_option->cell.id;
            bool episode_finished = false;
            int depth = 1;
            PERF_START("Tree_search::get_best_move_ucb1:episode");
            for (unsigned int t = 0;
                t < data.simulation_parameters.tree_search_parameters.depth && !episode_finished; t++) {
                float step_value = -data.simulation_parameters.reward.step_cost;
                auto &prey_cell = sim_prey.public_state().cell;
                if (prey_cell == data.goal_cell()) {
                    step_value += data.simulation_parameters.reward.episode_reward;
                    episode_finished = true;
                } else {
                    PERF_START("Tree_search::get_best_move_ucb1:capture_test");
                    auto &predator_cell = sim_predator.public_state().cell;
                    if (data.visibility[predator_cell].contains(prey_cell)) {
                        if (capture.is_captured(predator_cell, prey_cell)) {
                            step_value -= data.simulation_parameters.reward.capture_cost;
                            if (data.simulation_parameters.prey_parameters.terminate_on_capture) {
                                episode_finished = true;
                            }
                        }
                    }
                    PERF_STOP("Tree_search::get_best_move_ucb1:capture_test");
                    PERF_START("Tree_search::get_best_move_ucb1:episode_not_finished");
                    if (!episode_finished) {
                        PERF_START("Tree_search::get_best_move_ucb1:lppo_reached");
                        if (current_lppo_cell_id == prey_cell.id) {
                            depth++;
                            selected_option->load();
                            selected_option = &selected_option->get_best_option(1);
                            current_lppo_cell_id = selected_option->cell.id;
                        }
                        PERF_STOP("Tree_search::get_best_move_ucb1:lppo_reached");
                        PERF_START("Tree_search::get_best_move_ucb1:set_next_step");
                        auto &current_lppo_cell = data.cells[current_lppo_cell_id];
                        if (Chance::coin_toss(data.simulation_parameters.prey_parameters.randomness)) {
                            sim_prey.next_move = pick_random(data.world.connection_pattern);
                        } else {
                            sim_prey.next_move = data.paths.get_move(prey_cell, current_lppo_cell);
                        }
                        PERF_STOP("Tree_search::get_best_move_ucb1:set_next_step");
                        PERF_START("Tree_search::get_best_move_ucb1:model_update");
                        model.update();
                        model.update();
                        PERF_STOP("Tree_search::get_best_move_ucb1:model_update");
                    }
                    PERF_STOP("Tree_search::get_best_move_ucb1:episode_not_finished");
                }
                PERF_START("Tree_search::get_best_move_ucb1:reward_calculation");
                value += accum_gamma * (step_value);
                accum_gamma *= data.simulation_parameters.reward.gamma;
                PERF_STOP("Tree_search::get_best_move_ucb1:reward_calculation");
            }
            PERF_STOP("Tree_search::get_best_move_ucb1:episode");
            if (!episode_finished) {
                value += accum_gamma * (data.simulation_parameters.reward.incompleteness);
            }
            root.update_reward(value);
        }
        PERF_STOP("Tree_search::get_best_move_ucb1:simulations");

        PERF_START("Tree_search::get_best_move_ucb1:evaluation");
        auto *best_option = &root.get_best_option(0);
        auto best_move = data.paths.get_move(prey_current_cell, best_option->cell);
        while (!best_option->counters.empty()) {
            history_step.prey_state.plan.push_back(best_option->cell.id);
            best_option = &best_option->get_best_option(0);
        }
        history_step.prey_state.plan.push_back(best_option->cell.id);
        for (int option_index = 0; option_index < root.options.size(); option_index++) {
            if (root.counters[option_index] > 0) {
                history_step.prey_state.options.push_back(root.options[option_index].cell.id);
                history_step.prey_state.options_values.push_back(root.rewards[option_index]);
            }
        }
        PERF_STOP("Tree_search::get_best_move_ucb1:evaluation");
        return best_move;
    }
}