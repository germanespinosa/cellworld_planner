#pragma once
#include <cell_world.h>


namespace cell_world::planner {

    struct Statistics : json_cpp::Json_object {
        Json_object_members(
                Add_optional_member(length);
                Add_optional_member(visited_cells);
                Add_optional_member(survival_rate);
                Add_optional_member(capture_rate);
                Add_optional_member(value);
                Add_optional_member(belief_state_entropy);
                Add_optional_member(pursue_rate);
                Add_optional_member(distance);
                Add_optional_member(decision_difficulty);
                )
        float length{};
        float visited_cells{};
        float survival_rate{};
        float capture_rate{};
        float value{};
        float belief_state_entropy{};
        float pursue_rate{};
        float distance{};
        float decision_difficulty{};
    };

    struct Step_statistics : json_cpp::Json_object {
        Json_object_members(
                Add_member(value);
                Add_member(decision_difficulty);
                Add_member(belief_state_entropy);
        )
        float value{};
        float belief_state_entropy{};
        float decision_difficulty{};
    };

    struct Episode_statistics : Statistics {
        Json_object_members(
                Statistics::json_set_builder(jb);
                Add_optional_member(steps_stats);
        )
        json_cpp::Json_vector<Step_statistics> steps_stats;
    };

    struct Simulation_statistics : Statistics {
        Json_object_members(
                Statistics::json_set_builder(jb);
                Add_optional_member(episode_stats);
        )
        json_cpp::Json_vector<Episode_statistics> episode_stats;
    };
}
