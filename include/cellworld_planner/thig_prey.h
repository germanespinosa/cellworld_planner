#pragma once
#include <cell_world.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/static_data.h>
#include <cellworld_planner/tree_search.h>
#include <cellworld_planner/predator.h>

namespace cell_world::planner {
    struct Thig_prey : Stateless_agent {
        enum Route {
            North,
            South
        };
        enum Direction {
            Forward,
            Backward
        };
        explicit Thig_prey (const Static_data &data);
        const Cell &start_episode() override;
        Move get_move(const Model_public_state &) override;
        Agent_status_code update_state(const Model_public_state &) override;
        const Static_data &data;
        Route active_route;
        Direction direction;
        Cell_group_builder north_route;
        Cell_group_builder south_route;
        json_cpp::Json_vector<History_step> history;
        Cell_capture capture;
    };
}
