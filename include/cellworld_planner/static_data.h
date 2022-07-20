#pragma once
#include <cell_world.h>
#include <cellworld_planner/simulation.h>

namespace cell_world::planner {
    struct Static_data{
        explicit Static_data(const World_info &);
        World world;
        Cell_group cells;
        Cell_group free_cells;
        Cell_group occluded_cells;
        Map map;
        Graph graph;
        const Cell &start_cell;
        const Cell &goal_cell;
        Paths paths;
        Cell_group possible_destinations;
        Cell_group lppos;
        Graph visibility;
        Graph inverted_visibility;
        Graph options;
        Cell_group predator_start_locations;
        Move_list predator_moves;
        Simulation_parameters simulation_parameters;
        Capture_parameters capture;
        World_statistics world_statistics;
    };
}