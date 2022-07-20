#include <chrono>
#include <thread>
#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <thread_pool.h>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;
using namespace thread_pool;


int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"20_05");
    auto world = World::get_from_parameters_name("hexagonal", "canonical", occlusions);
    auto world_statistics = World_statistics::get_from_parameters_name("hexagonal",occlusions);
    cout << occlusions << "," << world_statistics.spatial_entropy << "," << world_statistics.spatial_espinometry << "," << world_statistics.visual_entropy << "," << world_statistics.visual_espinometry << endl;
}