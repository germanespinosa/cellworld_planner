#include <cellworld_planner/prey.h>

using namespace cell_world;

planner::Prey::Prey(const planner::Static_data &data, Predator &predator):
        data(data),
        mcts(data, predator){

}

const Cell &planner::Prey::start_episode() {
    mcts.reset();
    return data.start_cell;
}

Move planner::Prey::get_move(const Model_public_state &public_state) {
    auto move = mcts.get_best_move_ucb1(public_state);
    return move;
}

Agent_status_code planner::Prey::update_state(const Model_public_state &public_state) {
    if (public_state.status == cell_world::Model_public_state::Starting) return Agent_status_code::Running;
    mcts.record(public_state);
    if (public_state.agents_state[PREY].cell == data.goal_cell) return Agent_status_code::Finished;
    return Agent_status_code::Running;
}