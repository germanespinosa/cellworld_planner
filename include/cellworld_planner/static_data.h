#pragma once
#include <cell_world.h>

namespace cell_world::planner {
    struct Static_data{
        explicit Static_data(World &);
        World &world;
        Cell_group cells;
        Map map;
        Graph graph;
        const Cell &start_cell;
        const Cell &goal_cell;
        Paths paths;
        Cell_group possible_destinations;
        Cell_group predator_start_locations;
        Graph visibility;
        Graph inverted_visibility;
        Move_list predator_moves;
        float predator_best_move_probability;
        Move_list prey_moves;
        float prey_best_move_probability;
        unsigned int max_particle_count;
        unsigned int max_particle_creation_attempts;
    };
}