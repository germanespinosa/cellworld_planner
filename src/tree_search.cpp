#include <cellworld_planner/tree_search.h>

using namespace std;
using namespace cell_world;
using namespace json_cpp;

Move planner::Tree_search::get_best_move() {
    auto &prey_cell = data.map[belief_state.previous_prey_coordinates];
    return data.paths.get_move(prey_cell,data.goal_cell);
}

void planner::Tree_search::record(const cell_world::Model_public_state &state) {
    belief_state.record_state(state);
}

planner::Tree_search::Tree_search(const planner::Static_data &data):
    data(data),
    belief_state(data){

}
