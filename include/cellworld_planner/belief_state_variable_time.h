#pragma once
#include <cell_world.h>
#include <cellworld_planner/predator.h>
#include <cellworld_planner/static_data.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/belief_state.h>
#include <performance.h>
#include <queue>

namespace cell_world::planner {
    struct Move_location {
        Move move;
        Location prey_location;
        float prey_orientation;
        float  time;
    };
    struct Belief_state_variable_time {
        Belief_state_variable_time(const Static_data &data);
        std::vector<Model_state> particles;
        std::vector<Move_location> history;
        const Static_data &data;
        Location_visibility visibility;
        Dummy prey;
        Predator predator;
        Model model;
        Model_state root_state;
        Coordinates previous_prey_coordinates;
        unsigned int history_length;
        void record_state(const Model_public_state &state, const Location &prey_location, float prey_orientation, const Location &predator_location, bool is_visible, float time);
        Model_state get_particle();
        Model_state get_root_state();
        void reset();
        Json_unsigned_int_vector get_particle_count();
    };
}
