#pragma once
#include <cell_world.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/belief_state_location.h>
#include <cellworld_planner/tree_search.h>
#include <json_cpp.h>
#include <performance.h>

namespace cell_world::planner{
    struct Tree_search_location {
        explicit Tree_search_location(const Static_data &, Predator &);
        void record(const Model_public_state &, const Location &prey_location, float prey_orientation, const Location &predator_location, bool is_visible);
        Move get_best_move_ucb1(const Model_public_state &, const Location &prey_location, float prey_orientation, const Location &predator_location, bool is_visible);
        const Static_data &data;
        Predator &predator;
        Belief_state_location belief_state;
        Model model;
        Predator sim_predator;
        Dummy sim_prey;
        json_cpp::Json_vector<History_step> history;
        Cell_capture capture;
        void reset();
    };
}