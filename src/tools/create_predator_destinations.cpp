#include <cell_world.h>
#include <params_cpp.h>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;


int main (int argc, char **argv){
    Parser p(argc,argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"00_00");
    World world = World::get_from_parameters_name("hexagonal","canonical", occlusions);
    Graph graph = world.create_graph();
    Cell_group cells = world.create_cell_group().free_cells();
    Cell_group pd;
    for (const Cell &cell:cells) {
        if (graph[cell].size()==6){
            pd.add(cell);
        }
    }
    cout << pd << endl;
}

