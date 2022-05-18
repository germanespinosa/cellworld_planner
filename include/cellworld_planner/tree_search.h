#pragma once
#include <cell_world.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/belief_state.h>
#include <json_cpp.h>

namespace cell_world::planner{

    struct Node : json_cpp::Json_object {
        Json_object_members(
                Add_member(count);
                Add_member(value);
                )
        unsigned int count{};
        float value{};
    };

    struct Options_builder: json_cpp::Json_vector<json_cpp::Json_vector<Node>>{
        explicit Options_builder(size_t);
    };

    struct Options {
        explicit Options(const Graph &, const Cell_group &);
        Options(const Graph &, const Cell_group &, const Options_builder &);
        const Cell_group &lppos;
        const Graph &option_graph;
        Options_builder tree;
        Node & get_node(const Cell &, const Cell &);
        const Cell_group &get_options(const Cell &);
    };

    struct History_step : json_cpp::Json_object {
        History_step() = default;
        History_step(const json_cpp::Json_vector<int> &, const Model_public_state &);
        History_step(const Model_public_state &);
        Prey_state prey_state;
        Model_public_state state;
        Json_object_members(
                Add_member(prey_state);
                Add_member(state);
                )
    };

    struct Tree_search {
        explicit Tree_search(const Static_data &);
        void record(const Model_public_state &);
        Move get_best_move(const Model_public_state &);
        const Static_data &data;
        Belief_state belief_state;
        Options options;
        Model model;
        Predator predator;
        Dummy prey;
        json_cpp::Json_vector<History_step> history;
        void reset();
    };
}