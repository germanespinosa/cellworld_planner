#pragma once
#include <cell_world.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/static_data.h>

namespace cell_world::planner {

    struct Predator_state : Agent_internal_state {
        enum Behavior{
            Exploring,
            Pursuing
        };
        unsigned int destination_id{};
        Behavior behavior{Exploring};
        Move last_move;
    };

    struct Predator : Stateful_agent<Predator_state> {
        explicit Predator (const Static_data &data);
        const Cell &start_episode() override;
        Move get_move(const Model_public_state &) override;
        Agent_status_code update_state(const Model_public_state &) override;
        const Static_data &data;
    };

}
