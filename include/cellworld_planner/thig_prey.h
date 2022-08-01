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
        Thig_prey (const Static_data &data, Predator &);
        const Cell &start_episode() override;
        Move get_move(const Model_public_state &) override;
        Agent_status_code update_state(const Model_public_state &) override;
        Coordinates border(int x);
        Predator &predator;
        const Static_data &data;
        Route active_route;
        Direction direction;
        Cell_group_builder north_route;
        Cell_group_builder south_route;
        json_cpp::Json_vector<History_step> history;
        Cell_capture capture;
        bool reactive{};
    };
}
