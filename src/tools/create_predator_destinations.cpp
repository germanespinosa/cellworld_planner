#include <cell_world.h>
#include <params_cpp.h>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;


int main (int argc, char **argv){
    Parser p(argc,argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"00_00");
    World world = World::get_from_parameters_name("hexagonal","canonical");
    auto robot_occlusions = Cell_group_builder::get_from_parameters_name("hexagonal",  occlusions, "occlusions.robot");
    world.set_occlusions(robot_occlusions);
    Graph graph = world.create_graph();
    Cell_group cells = world.create_cell_group().free_cells();
    cout << cells.get_builder() << endl;
}

