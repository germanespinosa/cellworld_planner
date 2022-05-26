#pragma once
#include <cell_world.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/static_data.h>

namespace cell_world::planner {
    struct Predator : Stateful_agent<Predator_state> {
        explicit Predator (const Static_data &data);
        const Cell &start_episode() override;
        Move get_move(const Model_public_state &) override;
        Agent_status_code update_state(const Model_public_state &) override;
        const Static_data &data;
    };

}
