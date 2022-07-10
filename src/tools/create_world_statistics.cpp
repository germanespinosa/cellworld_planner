#include <cell_world.h>
#include <params_cpp.h>
#include <thread_pool.h>


using namespace thread_pool;
using namespace params_cpp;
using namespace std;
using namespace cell_world;

int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto configuration = p.get(Key("-c", "--world_configuration"), "");
    auto implementation = p.get(Key("-i", "--world_implementation"), "");
    auto occlusions = p.get(Key("-o", "--occlusions"), "");
    auto stats_file = p.get(Key("-s", "--world_statistics_file"), "");
    if (configuration.empty()) {
        cout << "Missing simulation file configuration." << endl;
        exit(1);
    }
    if (implementation.empty()) {
        cout << "Missing simulation file implementation." << endl;
        exit(1);
    }
    if (occlusions.empty()) {
        cout << "Missing simulation file occlusions." << endl;
        exit(1);
    }
    if (stats_file.empty()) {
        cout << "Missing simulation file stats_file." << endl;
        exit(1);
    }
    World w = World::get_from_parameters_name(configuration, implementation, occlusions);
    auto stats = w.get_statistics();
    stats.save(stats_file);
}