#pragma once
#include <cell_world.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/belief_state.h>
#include <json_cpp.h>

namespace cell_world::planner{

    struct Option {
        Option(const Cell &cell, const Graph &graph);
        const Cell &cell;
        const Graph &graph;
        void load();
        Option &get_best_option(double exploration);
        std::vector<double> rewards;
        std::vector<unsigned int> counters;
        std::vector<Option> options;
        std::vector<double> get_ucb1(double exploration);
        int best_option = Not_found;
        void update_reward(double );
    };

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