#pragma once
#include <cell_world.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/static_data.h>

namespace cell_world::planner {

    struct Statistics : json_cpp::Json_object {
        Json_object_members(
                Add_optional_member(length);
                Add_optional_member(visited_cells);
                Add_optional_member(survival_rate);
                Add_optional_member(capture_rate);
                Add_optional_member(time_out_rate);
                Add_optional_member(value);
                Add_optional_member(belief_state_entropy);
                Add_optional_member(pursue_rate);
                Add_optional_member(distance);
                Add_optional_member(decision_difficulty);
                )
        float length{};
        float visited_cells{};
        float survival_rate{};
        float time_out_rate{};
        float capture_rate{};
        float success_rate{};
        float value{};
        float belief_state_entropy{};
        float pursue_rate{};
        float distance{};
        float decision_difficulty{};
    };

    struct Step_statistics : json_cpp::Json_object {
        Json_object_members(
                Add_member(options);
                Add_member(value);
                Add_member(decision_difficulty);
                Add_member(belief_state_entropy);
        )
        float options{};
        float value{};
        float belief_state_entropy{};
        float decision_difficulty{};
    };

    struct Episode_statistics : Statistics {
        Json_object_members(
                Statistics::json_set_builder(jb);
                Add_optional_member(steps_stats);
                Add_optional_member(spatial_entropy);
                Add_optional_member(spatial_connections);
                Add_optional_member(spatial_connections_derivative);
                Add_optional_member(spatial_centrality);
                Add_optional_member(spatial_centrality_derivative);
                Add_optional_member(visual_entropy);
                Add_optional_member(visual_connections);
                Add_optional_member(visual_connections_derivative);
                Add_optional_member(visual_centrality);
                Add_optional_member(visual_centrality_derivative);
        )
        json_cpp::Json_vector<Step_statistics> steps_stats;
        float spatial_entropy{};
        float spatial_connections{};
        float spatial_connections_derivative{};
        float spatial_centrality{};
        float spatial_centrality_derivative{};
        float visual_entropy{};
        float visual_connections{};
        float visual_connections_derivative{};
        float visual_centrality{};
        float visual_centrality_derivative{};
    };

    struct Simulation_statistics : Statistics {
        Simulation_statistics() = default;
        Simulation_statistics(Simulation &, Static_data &);

        Json_object_members(
                Statistics::json_set_builder(jb);
                Add_optional_member(episode_stats);
        )
        json_cpp::Json_vector<Episode_statistics> episode_stats;

        void update();
    };
}
