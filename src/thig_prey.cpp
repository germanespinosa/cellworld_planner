#include <cellworld_planner/thig_prey.h>

using namespace cell_world;
using namespace std;


Coordinates planner::Thig_prey::border(int x){
    int maxy = 0;
    if (x == data.goal_cell().coordinates.x) return data.goal_cell().coordinates;
    if (x == data.start_cell().coordinates.x) return data.start_cell().coordinates;
    for (const Cell &cell: data.cells){
        if (cell.coordinates.x == x){
            if (cell.coordinates.y > maxy){
                maxy = cell.coordinates.y;
            }
        }
    }
    return Coordinates(x,maxy);
}

Cell_group_builder get_clean_route(const Cell_group_builder &route, const Graph &graph){
    Cell_group_builder clean_route;
    for (int s=0; s<route.size(); ){
        clean_route.push_back(route[s]);
        int last_connection = s + 1;
        for (int c=last_connection; c<route.size(); c++){
            if (graph[graph.cells[route[s]]].contains(graph.cells[route[c]])) last_connection = c;
        }
        s = last_connection;
    }
    return clean_route;
}

planner::Thig_prey::Thig_prey(const planner::Static_data &data, Predator &predator): predator (predator), data(data), capture(data.simulation_parameters.capture_parameters, data.visibility) {
    Cell_group_builder tmp_north_route;
    Cell_group_builder tmp_south_route;

    auto prev_north = data.start_cell();
    auto prev_south = data.start_cell();
    tmp_north_route.push_back(data.start_cell().id);
    tmp_south_route.push_back(data.start_cell().id);
    for (int x = data.start_cell().coordinates.x + 1; x <= data.goal_cell().coordinates.x; x++){
        auto b = border(x);
        if (data.map.find(b) == Not_found) continue;
        auto north_border = data.map[b];
        if (!north_border.occluded) {
            auto path = data.paths.get_path(prev_north, north_border);
            for(const Cell &step:path){
                if (step != prev_north)
                    tmp_north_route.push_back(step.id);
            }
            prev_north = north_border;
        }
        auto south_border = data.map[Coordinates(north_border.coordinates.x,-north_border.coordinates.y)];
        if (!south_border.occluded) {
            auto path = data.paths.get_path(prev_south, south_border);
            for(const Cell &step:path){
                if (step != prev_south)
                    tmp_south_route.push_back(step.id);
            }
            prev_south = south_border;
        }
    }
    north_route = get_clean_route (tmp_north_route, data.graph);
    south_route = get_clean_route (tmp_south_route, data.graph);
}

const Cell &planner::Thig_prey::start_episode() {
    history.clear();
    if (Chance::coin_toss(.5)){
        active_route = North;
    } else {
        active_route = South;
    }
    direction = Forward;
    return data.start_cell();
}

Move planner::Thig_prey::get_move(const Model_public_state &state) {
    auto &step = history.emplace_back();
    auto &prey_cell = state.agents_state[PREY].cell;
    step.prey_state.cell_id = prey_cell.id;
    auto &predator_cell = state.agents_state[PREDATOR].cell;
    step.predator_state = predator.internal_state();
    step.predator_state.cell_id = predator_cell.id;
    step.state = state;
    step.prey_state.capture = capture.is_captured(predator_cell, prey_cell);
    int next_cell_id = -1;
    if (reactive) {
        if (data.visibility[prey_cell].contains(predator_cell)) {
            if (predator_cell.coordinates.x > prey_cell.coordinates.x) {
                if ((active_route == North && predator_cell.coordinates.y > 0) ||
                    (active_route == South && predator_cell.coordinates.y < 0)) {
                    direction = Backward;
                }
            }
        }
    }
    if (prey_cell == data.start_cell()) {
        direction = Forward;
        if (active_route == North){
            active_route = South;
            next_cell_id = south_route[1];
        } else {
            active_route = North;
            next_cell_id = north_route[1];
        }
    } else {
        if (active_route == North){
            auto current_step = north_route.index_of(prey_cell.id);
            if (direction == Forward) {
                if (current_step == north_route.size() - 1){
                    next_cell_id = data.goal_cell().id;
                } else {
                    next_cell_id = north_route[current_step + 1];
                }
            } else {
                if (current_step == 0){
                    next_cell_id = data.start_cell().id;
                } else {
                    next_cell_id = north_route[current_step - 1];
                }
            }
        } else {
            auto current_step = south_route.index_of(prey_cell.id);
            if (direction == Forward) {
                if (current_step == south_route.size() - 1){
                    next_cell_id = data.goal_cell().id;
                } else {
                    next_cell_id = south_route[current_step + 1];
                }
            } else {
                if (current_step == 0){
                    next_cell_id = data.start_cell().id;
                } else {
                    next_cell_id = south_route[current_step - 1];
                }
            }

        }
    }
    Move move = data.cells[next_cell_id].coordinates - prey_cell.coordinates;
    return move;
}

Agent_status_code planner::Thig_prey::update_state(const Model_public_state &state) {
    auto &step = history.emplace_back();
    auto &prey_cell = state.agents_state[PREY].cell;
    step.prey_state.cell_id = prey_cell.id;
    auto &predator_cell = state.agents_state[PREDATOR].cell;
    step.predator_state = predator.internal_state();
    step.predator_state.cell_id = predator_cell.id;
    step.state = state;
    return Running;
}
