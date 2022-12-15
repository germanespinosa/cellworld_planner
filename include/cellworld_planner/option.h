#pragma once
#include <cell_world.h>

namespace cell_world::planner {
    struct Option {
        Option(const Cell &cell, const Graph &graph);
        const Cell &cell;
        const Graph &graph;
        void load();
        Option &get_best_option(float exploration);
        std::vector<float> rewards;
        std::vector<unsigned int> counters;
        std::vector<Option> options;
        std::vector<float> get_ucb1(float exploration);
        int best_option = Not_found;
        void update_reward(float);
    };
}