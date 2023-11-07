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
    auto occlusions = p.get(Key("-o", "--occlusions"), "");
    if (configuration.empty()) {
        cout << "Missing simulation file configuration." << endl;
        exit(1);
    }
    if (occlusions.empty()) {
        cout << "Missing simulation file occlusions." << endl;
        exit(1);
    }
    World w = World::get_from_parameters_name(configuration, "canonical", occlusions);
    auto stats = w.get_statistics();
    cout << stats;
}