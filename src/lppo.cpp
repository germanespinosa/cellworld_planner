#include <cellworld_planner/lppo.h>

namespace cell_world::planner {

    Lppos::Lppos(const World &world, const World_statistics &world_statistics):
    world(world),
    world_statistic(world_statistics) {}

    Cell_group_builder Lppos::get_spatial_lppos(unsigned int count) {
        Cell_group_builder lppos;
        float threshold = 0;
        threshold = max(world_statistic.spatial_centrality_derivative);
        while(lppos.size()<count) {
            float next_threshold = 0;
            for (unsigned int i = 0; i < world.cells.size(); i++) {
                if (world_statistic.spatial_centrality_derivative[i] == threshold) {
                    lppos.push_back(i);
                } else {
                    if (world_statistic.spatial_centrality_derivative[i] < threshold &&
                        world_statistic.spatial_centrality_derivative[i] > next_threshold) {
                        next_threshold = world_statistic.spatial_centrality_derivative[i];
                    }
                }
            }
            threshold = next_threshold;
        }
        if (!lppos.contains(0)) lppos.push_back(0);
        if (!lppos.contains(world_statistic.spatial_centrality_derivative.size()-1)) lppos.push_back(world_statistic.spatial_centrality_derivative.size()-1);
        return lppos;
    }

    Cell_group_builder Lppos::get_visual_lppos(unsigned int count) {
        Cell_group_builder lppos;
        float threshold = 0;
        threshold = max(world_statistic.visual_centrality);
        while(lppos.size()<count) {
            float next_threshold = 0;
            for (unsigned int i = 0; i < world.cells.size(); i++) {
                if (world_statistic.visual_centrality[i] == threshold) {
                    lppos.push_back(i);
                } else {
                    if (world_statistic.visual_centrality[i] < threshold &&
                        world_statistic.visual_centrality[i] > next_threshold) {
                        next_threshold = world_statistic.visual_centrality[i];
                    }
                }
            }
            threshold = next_threshold;
        }
        return lppos;
    }

    Graph Options::get_options(const Paths &paths, Cell_group &lppos) {
        auto options = Graph(paths.cells);
        for (const Cell &cell:paths.cells){
            if (cell.occluded) continue;
            for (const Cell &lppo:lppos){
                auto option_path = paths.get_path(cell, lppo);
                bool is_option = true;
                for (const Cell &step:option_path){
                    if (step == cell || step == lppo) continue;
                    if (lppos.contains(step)) {
                        is_option = false;
                        break;
                    }
                }
                if (is_option) options[cell].add(lppo);
            }
        }
        return options;
    }
}