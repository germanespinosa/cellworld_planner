#include <cell_world.h>
#include <params_cpp.h>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;

int main (int argc, char **argv){
    Parser p(argc,argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"20_05");
    auto configuration = p.get(Key("-c","--configuration"),"hexagonal");
    bool robot_paths = p.contains(Key("-r"));

    auto robot_occlusions = Cell_group_builder::get_from_parameters_name("hexagonal",  occlusions, robot_paths?"occlusions.robot":"occlusions");
    World world = World::get_from_parameters_name(configuration,"canonical");
    Graph graph = world.create_graph();
    Paths paths = Paths::get_astar(graph);
    cout << paths << endl;
}

