#pragma once
#include <cell_world.h>
#include <cellworld_planner/option.h>
#include <cellworld_planner/history_step.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/belief_state.h>
#include <json_cpp.h>
#include <performance.h>

namespace cell_world::planner{
    struct Tree_search {
        explicit Tree_search(const Static_data &, Predator &);
        void record(const Model_public_state &);
        Move get_best_move_ucb1(const Model_public_state &);
        const Static_data &data;
        Predator &predator;
        Belief_state belief_state;
        Model model;
        Predator sim_predator;
        Dummy sim_prey;
        json_cpp::Json_vector<History_step> history;
        Cell_capture capture;
        void reset();
    };
}