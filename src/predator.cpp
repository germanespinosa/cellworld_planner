#include <cellworld_planner/predator.h>

using namespace cell_world;

const Cell &planner::Predator::start_episode() {
    internal_state().destination_id = Not_found;
    internal_state().behavior = Predator_state::Exploring;
    if (data.predator_start_locations.empty())
        return data.free_cells.random_cell();
    else
        return data.predator_start_locations.random_cell();
}

Move planner::Predator::get_move(const Model_public_state &public_state) {
    if (Chance::coin_toss(data.simulation_parameters.predator_parameters.randomness)) {
        auto index = rand() % data.predator_moves.size();
        auto move =  data.predator_moves[index];
        return move;
    }
    auto &prey_state = public_state.agents_state[0];
    auto &predator_state = public_state.agents_state[1];
    auto &internal_state = this->internal_state();
    if (data.visibility[predator_state.cell].contains(prey_state.cell)){ // prey is visible
        internal_state.behavior = Predator_state::Pursuing;
        internal_state.destination_id = prey_state.cell.id;
    } else {
        if (internal_state.destination_id == Not_found || internal_state.destination_id == predator_state.cell.id) { // destination reached
            auto not_visible_destinations = data.visibility[predator_state.cell];
            //if (!data.possible_destinations.empty()) not_visible_destinations = not_visible_destinations &data.possible_destinations;
            if (not_visible_destinations.empty())
                internal_state.destination_id = data.free_cells.random_cell().id;
            else
                internal_state.destination_id = not_visible_destinations.random_cell().id;
            internal_state.behavior = Predator_state::Exploring;
        }
    }
    auto speed = data.simulation_parameters.predator_parameters.exploration_speed;
    if (internal_state.behavior == Predator_state::Pursuing){
        speed = data.simulation_parameters.predator_parameters.pursue_speed;
    }
    int confirmed_moves = int (speed);
    float final_move_probability = speed - confirmed_moves;
    Move move(0,0);
    for (int move_count = 0; move_count < confirmed_moves; move_count++){
        auto &current_predator_cell =  data.map[predator_state.cell.coordinates + move];
        move  += data.paths.get_move(current_predator_cell, data.cells[internal_state.destination_id]);
    }
    if (Chance::coin_toss(final_move_probability)) {
        auto &current_predator_cell =  data.map[predator_state.cell.coordinates + move];
        move  += data.paths.get_move(current_predator_cell, data.cells[internal_state.destination_id]);
    }
    return move;
}

planner::Predator::Predator(const planner::Static_data &data):
        data(data) {
}

Agent_status_code planner::Predator::update_state(const Model_public_state &public_state) {
    auto &prey_state = public_state.agents_state[0];
    auto &predator_state = public_state.agents_state[1];
    auto &internal_state = this->internal_state();
    internal_state.cell_id = predator_state.cell.id;
    if (data.visibility[predator_state.cell].contains(prey_state.cell)){ // prey is visible
        internal_state.behavior = Predator_state::Pursuing;
        internal_state.destination_id = prey_state.cell.id;
    }
    return Agent_status_code::Running;
}
