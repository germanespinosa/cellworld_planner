#pragma once
#include <cell_world.h>
#include <cellworld_planner/agents.h>
#include <cellworld_planner/belief_state.h>

namespace cell_world::planner{

    struct Node {
        const Cell &prey_cell;
        const Cell &predator_cell;
        unsigned int count;
        float value;
    };

    struct Tree {
        std::vector<Node> nodes;
    };

    struct Tree_search {
        explicit Tree_search(const Static_data &);
        void record(const Model_public_state &);
        Move get_best_move();
        const Static_data &data;
        Belief_state belief_state;
        Graph options;
        Tree tree;
    };
}