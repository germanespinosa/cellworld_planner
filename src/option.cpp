#include <cellworld_planner/option.h>

using namespace std;
using namespace cell_world;

namespace cell_world::planner {
    Option::Option(const Cell &cell, const Graph &graph) :
            cell(cell), graph(graph) {

    }

    void Option::load() {
        if (!counters.empty()) return;
        counters = vector<unsigned int>(graph[cell].size(), 0);
        rewards = vector<float>(graph[cell].size(), 0);
        for (const Cell &option: graph[cell]) {
            options.emplace_back(option, graph);
        }
    }

    vector<float> Option::get_ucb1(float exploration) {
        if (counters.empty()) return {};
        auto r = vector<float>(counters.size(), 0);
        double total = 0;
        for (auto c: counters) total += double(c);
        float p = 2 * log(total);
        for (unsigned int i = 0; i < counters.size(); i++) {
            if (counters[i] == 0) {
                if (exploration == 0) {
                    r[i] = std::numeric_limits<float>::min();
                } else {
                    r[i] = std::numeric_limits<float>::max();
                }
            } else {
                r[i] = rewards[i] + exploration * sqrt(p / float(counters[i]));
            }
        }
        return r;
    }

    Option &Option::get_best_option(float exploration) {
        auto ucb1_values = get_ucb1(1);
        best_option = Chance::pick_best(1, ucb1_values);
        return options[best_option];
    }

    void Option::update_reward(float reward) {
        if (best_option == Not_found) return;
        rewards[best_option] =
                (rewards[best_option] * counters[best_option] + reward) / double(counters[best_option] + 1);
        counters[best_option]++;
        options[best_option].update_reward(reward);
    }
}