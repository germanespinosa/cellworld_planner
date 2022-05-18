#include <cellworld_planner/tree_search.h>

using namespace std;
using namespace cell_world;
using namespace json_cpp;

Move planner::Tree_search::get_best_move(const Model_public_state &state) {
    belief_state.record_state(state);
    auto &history_step = history.emplace_back(state);
    auto &prey_current_cell = state.agents_state[PREY].cell;
    history_step.prey_state.belief_state = Json_vector<unsigned int>(data.cells.size(),0);
    for (auto &particle:belief_state.particles){
        history_step.prey_state.belief_state[particle.public_state.agents_state[PREDATOR].cell.id] ++;
    }
    unsigned int option_cell_id;
    auto &prey_options = options.get_options(prey_current_cell);
    for (const Cell &prey_option:prey_options){
        history_step.prey_state.options.push_back(prey_option.id);
    }
    history_step.prey_state.options_values = Json_vector<float>(prey_options.size(),data.simulation_parameters.reward.default_value);
    if (belief_state.particles.empty()) {
        return data.paths.get_move(prey_current_cell,data.goal_cell);
    }
    auto &options_values = history_step.prey_state.options_values;
    vector<unsigned int> options_counters (prey_options.size(),0);
    for (unsigned int s=0; s<data.simulation_parameters.tree_search_parameters.simulations; s++) {
        auto &selected_option = prey_options.random_cell();
        auto particle = belief_state.get_particle();
        float value = 0;
        float accum_gamma = 1;
        model.set_state(particle);
        unsigned int current_lppo_cell_id = selected_option.id;
        bool episode_finished = false;
        for (unsigned int t=0;t<data.simulation_parameters.tree_search_parameters.depth & !episode_finished; t++) {
            float step_value = -data.simulation_parameters.reward.step_cost;
            auto &prey_cell = prey.public_state().cell;
            if (prey_cell==data.goal_cell){
                step_value += data.simulation_parameters.reward.episode_reward;
                episode_finished = true;
            } else {
                auto &predator_cell = predator.public_state().cell;
                auto difference = prey_cell.coordinates - predator_cell.coordinates;
                auto distance = abs(difference.x) + abs(difference.y);
                if (distance <= 6) {
                    step_value -= data.simulation_parameters.reward.capture_cost;
                }
            }
            value += accum_gamma * (step_value);
            accum_gamma *= data.simulation_parameters.reward.gamma;

            if (current_lppo_cell_id == prey_cell.id) {
                auto &lppo_options = options.get_options(data.cells[current_lppo_cell_id]);
                current_lppo_cell_id = lppo_options.random_cell().id;
            }
            auto &current_lppo_cell = data.cells[current_lppo_cell_id];
            auto move = data.paths.get_move(prey_cell,current_lppo_cell);
            prey.next_move = move;
            model.update();
            model.update();
        }
        auto selected_option_index = prey_options.find(selected_option);
        options_values[selected_option_index] = (options_values[selected_option_index] * options_counters[selected_option_index] + value) / (options_counters[selected_option_index] + 1);
        options_counters[selected_option_index]++ ;
    }
    auto best_option_index = 0;
    for (int i = 1 ; i < options_values.size(); i++){
        if (options_values[i]>options_values[best_option_index]){
            best_option_index = i;
        }
    }

    return data.paths.get_move(prey_current_cell,prey_options[best_option_index]);
}

void planner::Tree_search::record(const cell_world::Model_public_state &state) {
    belief_state.record_state(state);
    history.emplace_back(state);
}

planner::Tree_search::Tree_search(const planner::Static_data &data):
    data(data),
    belief_state(data),
    options(data.options, data.lppos),
    model(data.cells, 1000),
    predator(data),
    prey(data){
    model.add_agent(prey);
    model.add_agent(predator);
    //for (auto &b : options.tree) for (auto &n : b) n.value = data.default_node_value;
}

void planner::Tree_search::reset() {
    belief_state.reset();
    history.clear();
}

planner::Options::Options(const Graph &options_graph, const Cell_group &lppos):
    option_graph(options_graph),
    lppos(lppos),
    tree(options_graph.size()) {
}

planner::Node &planner::Options::get_node(const Cell &prey_cell, const Cell &option) {
    return tree[prey_cell.id][option.id];
}

const Cell_group &planner::Options::get_options(const Cell &cell) {
    return option_graph[cell];
}

planner::Options_builder::Options_builder(size_t s):
        Json_vector(s, Json_vector<Node>(s)){
}

planner::History_step::History_step(const json_cpp::Json_vector<int> &belief_state, const Model_public_state &state):
prey_state(prey_state), state(state){

}

planner::History_step::History_step(const Model_public_state &state): state(state) {

}
