#include <cell_world.h>
#include <params_cpp.h>
#include <thread_pool.h>

using namespace thread_pool;
using namespace params_cpp;
using namespace std;
using namespace cell_world;


Cell_group_builder random_cells(int count, int cell_count){
    Cell_group_builder res;
    for (int i = 0; i < count; i++){
        int x = Chance::dice(0, cell_count);
        while(find(res.begin(), res.end(), x) != res.end())
            x = Chance::dice(0, cell_count);
        res.push_back(x);
    }
    return res;
}

void set_stats(World &w, World_statistics &ws){
    ws = w.get_statistics();
}

int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto entropy_bucket = stoi(p.get(Key("-e", "--entropy_level"), ""));
    auto entropy_level = (float)entropy_bucket / 10.0;
    auto count = stoi(p.get(Key("-c", "--count"), ""));
    auto configuration = p.get(Key("-wc", "--world_configuration"), "hexagonal");
    auto wc = Resources::from("world_configuration").key(configuration).get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key(configuration).key("canonical").get_resource<World_implementation>();
    unsigned int occlusion_number = 0;
    unsigned int cells_count = wc.cell_coordinates.size();
    float map_entropy = -1;
    float old_map_entropy = -1;

    for (occlusion_number = 0; occlusion_number < cells_count; occlusion_number++){
        unsigned int non_occluded = cells_count - occlusion_number;
        map_entropy=weights_entropy(vector<unsigned int>{occlusion_number,non_occluded});
        if (map_entropy > entropy_level || map_entropy < old_map_entropy) break;
        old_map_entropy = map_entropy;
    }
    auto world = World(wc,wi);
    vector<World> test_worlds (count, world);
    vector<World_statistics> test_stats (count);
    thread_pool::Thread_pool tp;
    for (int i = 0; i<count; i++){
        auto occlusions_cell_ids = random_cells(occlusion_number,cells_count);
        test_worlds[i].set_occlusions(occlusions_cell_ids);
        tp.run(set_stats, ref(test_worlds[i]), ref(test_stats[i]));
    }
    tp.wait_all();
    for (auto world_statistics:test_stats){
        cout << entropy_bucket<< "," << world_statistics.spatial_entropy << "," << world_statistics.spatial_espinometry << "," << world_statistics.visual_entropy << "," << world_statistics.visual_espinometry << endl;
    }
}

