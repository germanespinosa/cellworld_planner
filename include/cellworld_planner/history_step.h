#pragma once
#include <cell_world.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/history_step.h>

namespace cell_world::planner {
    struct History_step : json_cpp::Json_object {
        History_step() = default;
        explicit History_step(const Model_public_state &);
        Prey_state prey_state;
        Predator_state predator_state;
        Model_public_state state;
        Json_object_members(
                Add_member(prey_state);
                Add_member(predator_state);
                Add_member(state);
        )
    };
}