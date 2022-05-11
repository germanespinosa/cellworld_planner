#include <cellworld_planner/prey.h>

using namespace cell_world;

planner::Prey::Prey(const planner::Static_data &data):
        data(data),
        mcts(data){

}

const Cell &planner::Prey::start_episode() {
    return data.start_cell;
}

Move planner::Prey::get_move(const Model_public_state &public_state) {
    mcts.record(public_state);
    auto move = mcts.get_best_move();
    return move;
}

Agent_status_code planner::Prey::update_state(const Model_public_state &public_state) {
    mcts.record(public_state);
    return Agent_status_code::Running;
}