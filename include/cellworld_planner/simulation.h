#pragma once
#include <cell_world.h>

namespace cell_world::planner {

    struct Reward : json_cpp::Json_object {
        float step_cost;
        float gamma;
        float capture_cost;
        float episode_reward;

        Json_object_members(
                Add_member(step_cost);
                Add_member(gamma);
                Add_member(capture_cost);
                Add_member(episode_reward);
        )
    };
    using Belief_state_representation = json_cpp::Json_vector<unsigned int>;

    struct Prey_state : json_cpp::Json_object {
        unsigned int cell_id;
        Cell_group_builder options;
        json_cpp::Json_vector<float> options_values;
        Cell_group_builder plan;
        Belief_state_representation belief_state;
        bool capture;
        Json_object_members(
                Add_member(cell_id);
                Add_member(options);
                Add_member(options_values);
                Add_member(plan);
                Add_member(belief_state);
                Add_member(capture);
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

    struct Predator_state : Agent_internal_state {
        enum Behavior {
            Exploring,
            Pursuing
        };

        Json_object_members(
                Add_member(cell_id);
                Add_member(destination_id);
                Add_member(behavior);
        )
        unsigned int cell_id{};
        int destination_id{};
        Behavior behavior{Exploring};
    };


    struct Simulation_step : json_cpp::Json_object {
        Json_object_members(
                Add_member(predator_state);
                Add_member(prey_state);
                Add_optional_member(data);
                )
        Predator_state predator_state;
        Prey_state prey_state;
        std::string data;
    };

    using Simulation_episode = json_cpp::Json_vector<Simulation_step>;

    struct Simulation : json_cpp::Json_object {
        World_info world_info;
        Simulation_parameters parameters;
        json_cpp::Json_vector<Simulation_episode> episodes;
        Json_object_members(
                Add_member(world_info);
                Add_member(parameters);
                Add_member(episodes);
        )
    };
}