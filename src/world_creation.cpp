#include <cellworld_planner/world_creation.h>
#include <thread_pool.h>

namespace cell_world::planner{

    Coordinates_list get_connections(Coordinates &coordinates, Map &map, Connection_pattern &connection_pattern){
        Coordinates_list connections;
        auto candidates = connection_pattern.get_candidates(coordinates);
        for (auto &c:candidates){
            if (map.find(c) != Not_found && !map[c].occluded) connections.push_back(c);
        }
        return connections;
    }

    void create_random_world(
            World world,
            Cell_group_builder &occlusions,
            const Cell &start_cell,
            const Cell &goal_cell,
            int occlusion_count,
            int max_cluster_size,
            int seed,
            std::atomic<int> &ready ){
        if (seed == -1){
            seed = time(0)*thread_pool::worker_id;
        }
        Chance::seed(seed * time(0));
        auto cells = world.create_cell_group();
        auto map = Map(cells);
        while (ready == -1) {
            occlusions = Cell_group_builder();
            world.set_occlusions(occlusions);
            int pending = occlusion_count;
            while (pending > 0){
                auto current_coordinates = cells.random_cell().coordinates;
                int l = Chance::dice(pending < max_cluster_size ? pending : max_cluster_size) + 1;
                for (int i = 0; i < l ; i++){
                    if (map[current_coordinates].occluded) {
                        break;
                    }
                    auto connections = get_connections(current_coordinates, map, world.connection_pattern);
                    if (connections.empty()){
                        break;
                    }
                    world[map[current_coordinates].id].occluded = true;
                    pending--;
                    current_coordinates = pick_random(connections);
                }
            }
            auto graph = world.create_graph();
            if (graph.is_connected(start_cell, goal_cell)) {
                if (ready == -1) {
                    for (auto &cell:world.cells){
                        if (!cell.occluded) {
                            if (!graph.is_connected(start_cell, cell)) {
                                cell.occluded = true;
                            }
                        }
                    }
                    occlusions = world.create_cell_group().occluded_cells().get_builder();
                    ready = seed;
                }
            }
        }
    }


    Cell_group_builder World_creation::get_valid_occlusions(World &world, unsigned int k, int max_cluster_size) {
        if (max_cluster_size == -1) max_cluster_size = (int)k;
        auto cells = world.create_cell_group();
        auto map = Map(cells);
        std::atomic<int> ready =-1;
        thread_pool::Thread_pool tp;
        std::vector<Cell_group_builder> occlusion_candidates(tp.workers.size());
        for (int i=0;i<tp.workers.size();i++){
            tp.run(create_random_world,
                    world,
                    std::ref(occlusion_candidates[i]),
                    std::ref(world.cells.front()),
                    std::ref(world.cells.back()),
                    k,
                    max_cluster_size,
                    i,
                    std::ref(ready));
        }
        tp.wait_all();
        return occlusion_candidates[ready];
    }
}