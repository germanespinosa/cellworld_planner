#pragma once
#include <cell_world.h>
#include <json_cpp.h>

namespace cell_world::planner {

    struct Reward : json_cpp::Json_object {
        float step_cost;
        float gamma;
        float capture_cost;
        float episode_reward;
        float default_value;

        Json_object_members(
                Add_member(step_cost);
                Add_member(gamma);
                Add_member(capture_cost);
                Add_member(episode_reward);
                Add_member(default_value);
        )
    };
    using Belief_state_representation = json_cpp::Json_vector<unsigned int>;

    struct Prey_state : json_cpp::Json_object {
        unsigned int frame;
        Cell_group_builder options;
        json_cpp::Json_vector<float> options_values;
        Belief_state_representation belief_state;

        Json_object_members(
                Add_member(frame);
                Add_member(options);
                Add_member(options_values);
                Add_member(belief_state);
                )
    };

    struct Belief_state_parameters : json_cpp::Json_object {
        unsigned int max_particle_count{};
        unsigned int max_particle_creation_attempts{};
        Json_object_members(
                Add_member(max_particle_count);
                Add_member(max_particle_creation_attempts);
        )
    };

    struct Tree_search_parameters : json_cpp::Json_object {
        Belief_state_parameters belief_state_parameters;
        int simulations{};
        int depth{};
        Json_object_members(
                Add_member(belief_state_parameters);
                Add_member(simulations);
                Add_member(depth);
        )
    };

    struct Predator_parameters : json_cpp::Json_object {
        float exploration_speed{};
        float pursue_speed{};
        Json_object_members(
                Add_member(exploration_speed);
                Add_member(pursue_speed);
        )
    };

    struct Simulation_parameters : json_cpp::Json_object {
        Reward reward;
        Tree_search_parameters tree_search_parameters;
        Predator_parameters predator_parameters;
        Json_object_members(
                Add_member(reward);
                Add_member(tree_search_parameters);
                Add_member(predator_parameters);
        )
    };

    struct Simulation : json_cpp::Json_object {
        Simulation_parameters parameters;
        Experiment experiment;
        json_cpp::Json_vector<json_cpp::Json_vector<Prey_state>> prey_data;
        Json_object_members(
                Add_member(parameters);
                Add_member(experiment);
                Add_member(prey_data);
        )
    };
}