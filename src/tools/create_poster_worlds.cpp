#include <cell_world.h>
#include <params_cpp.h>
#include <thread_pool.h>
#include<ctime>
#include <mutex>

using namespace thread_pool;
using namespace params_cpp;
using namespace std;
using namespace cell_world;

vector<World> test_worlds;
vector<World_statistics> test_stats;
mutex mtx;

Cell_group_builder random_cells(int count, int cell_count){
    Cell_group_builder res;
    for (int i = 0; i < count; i++){
        int x = Chance::dice(cell_count);
        while(find(res.begin(), res.end(), x) != res.end())
            x = Chance::dice(cell_count);
        res.push_back(x);
    }
    return res;
}

void create_world(float minv, float maxv, bool high, string occlusions_output, string statistics_output){
    Chance::seed(time(0)*thread_pool::worker_id);
    auto &world = test_worlds[thread_pool::worker_id];
    auto &statistics = test_stats[thread_pool::worker_id];
    int occlusion_count;
    while (true) {
        Chance::seed(time(0)*thread_pool::worker_id);
        occlusion_count = Chance::dice(world.size());
        if (high && occlusion_count< 70) continue;
        auto occlusions = random_cells(occlusion_count, world.size());
        world.set_occlusions(occlusions);
        statistics = world.get_statistics();
        if (statistics.spatial_espinometry >= minv && statistics.spatial_espinometry < maxv) {
            auto graph = world.create_graph();
            cout << "occlusions: " << occlusion_count << " - complexity: " << statistics.spatial_espinometry << endl;
            cout << "checking path from source to destination" << endl;
            if (graph.is_connected(world.cells[0], world.cells[330])) {
                mtx.lock();
                occlusions.save(occlusions_output);
                statistics.save(statistics_output);
                cout << "is connected" << endl;
                exit(0);
                return;
            } else {
                cout << "is not connected" << endl;
            }
        }
    }
}

int main(int argc, char **argv) {
    Parser p(argc, argv);
    bool high_density = p.get(Key("-d", "--density"), "H")=="H";
    auto complexity_level_min = stof(p.get(Key("-cmin", "--minimum_complexity"), ""));
    auto complexity_level_max = stof(p.get(Key("-cmax", "--maximum_complexity"), ""));
    auto configuration = p.get(Key("-wc", "--world_configuration"), "hexagonal");
    auto occlusions_output = p.get(Key("-o", "--output"), "");
    auto statistics_output = p.get(Key("-s", "--statistics"), "");
    auto wc = Resources::from("world_configuration").key(configuration).get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key(configuration).key("canonical").get_resource<World_implementation>();
    auto world = World(wc,wi);
    thread_pool::Thread_pool tp;
    test_worlds=vector<World>(tp.workers.size(), world);
    test_stats=vector<World_statistics>(tp.workers.size());
    cout << "looking for complexity between " << complexity_level_min << " and " << complexity_level_max << (high_density?" HIGH DENSITY":" LOW DENSITY") << endl;
    for (auto &worker:tp.workers){
        tp.run(create_world, complexity_level_min, complexity_level_max, high_density, occlusions_output, statistics_output);
    }
}

