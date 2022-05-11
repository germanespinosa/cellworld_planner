#pragma once
#include <cell_world.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/static_data.h>
#include <cellworld_planner/tree_search.h>

namespace cell_world::planner {
    struct Prey : Stateless_agent {
        explicit Prey (const Static_data &data);
        const Cell &start_episode() override;
        Move get_move(const Model_public_state &) override;
        Agent_status_code update_state(const Model_public_state &) override;
        Tree_search mcts;
        const Static_data &data;
    };
}
