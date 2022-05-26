#include <cellworld_planner/tree_search.h>

using namespace std;
using namespace cell_world;
using namespace cell_world::planner;
using namespace json_cpp;

void planner::Tree_search::record(const cell_world::Model_public_state &state) {
    belief_state.record_state(state);
    auto prev_belief_state = history[history.size()-1].prey_state.belief_state;
    auto &history_step = history.emplace_back(state);
    auto &prey_current_cell = state.agents_state[PREY].cell;
    auto &predator_current_cell = state.agents_state[PREDATOR].cell;
    history_step.predator_state = predator.internal_state();
    history_step.predator_state.cell_id = predator_current_cell.id;
    history_step.prey_state.capture = capture.is_captured(predator_current_cell, prey_current_cell);
    history_step.prey_state.cell_id = prey_current_cell.id;
    history_step.prey_state.belief_state = prev_belief_state;
}

planner::Tree_search::Tree_search(const planner::Static_data &data, Predator &predator):
        data(data),
        predator(predator),
        belief_state(data),
        model(data.cells, 1000),
        sim_predator(data),
        sim_prey(data),
        capture(data.capture, data.visibility){
    model.add_agent(sim_prey);
    model.add_agent(sim_predator);
}

void planner::Tree_search::reset() {
    belief_state.reset();
    history.clear();
}

Move planner::Tree_search::get_best_move_ucb1(const Model_public_state &state) {
    belief_state.record_state(state);
    auto &history_step = history.emplace_back(state);
    auto &prey_current_cell = state.agents_state[PREY].cell;
    auto &predator_current_cell = state.agents_state[PREDATOR].cell;
    history_step.predator_state = predator.internal_state();
    history_step.predator_state.cell_id = predator_current_cell.id;
    history_step.prey_state.capture = capture.is_captured(predator_current_cell, prey_current_cell);
    history_step.prey_state.cell_id = prey_current_cell.id;

    history_step.prey_state.belief_state = Json_vector<unsigned int>(data.cells.size(),0);
    for (auto &particle:belief_state.particles){
        history_step.prey_state.belief_state[particle.public_state.agents_state[PREDATOR].cell.id] ++;
    }

    Option root(prey_current_cell,data.options);
    root.load();
    unsigned int option_cell_id;
    for (auto &prey_option:root.options){
        history_step.prey_state.options.push_back(prey_option.cell.id);
    }
    history_step.prey_state.options_values = Json_vector<float>(root.options.size(),0);
    if (belief_state.particles.empty()) {
        return data.paths.get_move(prey_current_cell,data.goal_cell);
    }
    for (unsigned int s=0; s<data.simulation_parameters.tree_search_parameters.simulations; s++) {
        auto *selected_option = &root.get_best_option(1);
        auto particle = belief_state.get_particle();
        float value = 0;
        float accum_gamma = 1;
        model.set_state(particle);
        unsigned int current_lppo_cell_id = selected_option->cell.id;
        bool episode_finished = false;
        int depth = 1;
        for (unsigned int t=0;t<data.simulation_parameters.tree_search_parameters.depth && !episode_finished; t++) {
            float step_value = -data.simulation_parameters.reward.step_cost;
            auto &prey_cell = sim_prey.public_state().cell;
            if (prey_cell==data.goal_cell){
                step_value += data.simulation_parameters.reward.episode_reward;
                episode_finished = true;
            } else {
                auto &predator_cell = sim_predator.public_state().cell;
                if (data.visibility[predator_cell].contains(prey_cell)) {
                    if (capture.is_captured(predator_cell, prey_cell)) {
                        step_value -= data.simulation_parameters.reward.capture_cost;
                    }
                }
            }
            value += accum_gamma * (step_value);
            accum_gamma *= data.simulation_parameters.reward.gamma;

            if (current_lppo_cell_id == prey_cell.id) {
                depth ++ ;
                selected_option->load();
                selected_option = &selected_option->get_best_option(1);
                current_lppo_cell_id = selected_option->cell.id;
            }
            auto &current_lppo_cell = data.cells[current_lppo_cell_id];
            auto move = data.paths.get_move(prey_cell,current_lppo_cell);
            sim_prey.next_move = move;
            model.update();
            model.update();
        }
        root.update_reward(value);
    }
    return data.paths.get_move(prey_current_cell,root.get_best_option(0).cell);
}

planner::History_step::History_step(const Model_public_state &state): state(state) {

}

Option::Option(const Cell &cell, const Graph &graph) :
        cell(cell), graph(graph){

}

void Option::load() {
    if (!counters.empty()) return;
    counters = vector<unsigned int>(graph[cell].size(),0);
    rewards = vector<double>(graph[cell].size(),0);
    for (const Cell &option: graph[cell]){
        options.emplace_back(option, graph);
    }
}

vector<double> Option::get_ucb1(double exploration) {
    if (counters.empty()) return {};
    auto r = vector<double>(counters.size(),0);
    double total = 0;
    for (auto c: counters) total += double(c);
    float p = 2 * log(total);
    for (unsigned int i=0; i<counters.size(); i++){
        if (counters[i] == 0) r[i] = std::numeric_limits<double>::max();
        else r[i] = rewards[i] + exploration * sqrt(p/double(counters[i]));
    }
    return r;
}

Option &Option::get_best_option(double exploration) {
    auto ucb1_values = get_ucb1(1);
    best_option = Chance::pick_best(1,ucb1_values);
    return options[best_option];
}

void Option::update_reward(double reward) {
    if (best_option == Not_found) return;
    rewards[best_option] = (rewards[best_option] * counters[best_option] + reward) / double(counters[best_option] + 1);
    counters[best_option] ++;
    options[best_option].update_reward(reward);
}

