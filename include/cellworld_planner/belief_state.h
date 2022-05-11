#pragma once
#include <cell_world.h>
#include <cellworld_planner/predator.h>
#include <cellworld_planner/static_data.h>
#include <cellworld_planner/agents.h>
#include <queue>

namespace cell_world::planner {

    struct Dummy : Stateless_agent {
        explicit Dummy (const Static_data &data);
        const Cell &start_episode() override;
        Move get_move(const Model_public_state &) override;
        const Static_data &data;
        unsigned int start_cell_id;
        Move next_move;
    };

    struct Belief_state {
        Belief_state(const Static_data &data);
        std::vector<Model_state> particles;
        std::vector<Move> history;
        const Static_data &data;
        Dummy prey;
        Predator predator;
        Model model;
        Model_state root_state;
        Coordinates previous_prey_coordinates;
        unsigned int history_length;
        void record_state(const Model_public_state &state);
        Model_state get_particle();
        Model_state get_root_state();
    };
}
