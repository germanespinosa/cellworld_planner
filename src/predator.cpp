#include <cellworld_planner/predator.h>

using namespace cell_world;

const Cell &planner::Predator::start_episode() {
    return data.predator_start_locations.random_cell();
}

Move planner::Predator::get_move(const Model_public_state &public_state) {
    return Move(0,0);
    auto &prey_state = public_state.agents_state[0];
    auto &predator_state = public_state.agents_state[1];
    auto &internal_state = this->internal_state();
    if (data.visibility[predator_state.cell].contains(prey_state.cell)){ // prey is visible
        internal_state.behavior = Predator_state::Pursuing;
        internal_state.destination_id = prey_state.cell.id;
    } else {
        if (internal_state.destination_id == predator_state.cell.id) { // destination reached
            auto not_visible_destinations = data.visibility[predator_state.cell] & data.possible_destinations;
            internal_state.destination_id = not_visible_destinations.random_cell().id;
            internal_state.behavior = Predator_state::Exploring;
        }
    }
    Move best_move = data.paths.get_move(predator_state.cell, data.cells[internal_state.destination_id]);
    if (Chance::coin_toss(data.predator_best_move_probability)){
        return best_move;
    } else {
        return data.predator_moves[Chance::dice(data.predator_moves.size())];
    }
}

planner::Predator::Predator(const planner::Static_data &data):
        data(data) {
}

Agent_status_code planner::Predator::update_state(const Model_public_state &public_state) {
    return Agent_status_code::Running;
}
