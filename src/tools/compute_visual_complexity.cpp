#include <cell_world.h>
#include <params_cpp.h>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;

int main (int argc, char **argv){
    Parser p(argc,argv);
    World world = World::get_from_parameters_name("hexagonal","canonical");
    Cell_group cells = world.create_cell_group();
    Json_float_vector ndc;
    Json_float_vector oc;
    for (unsigned int i = 0; i<=1402 ; i++){
        string file_path = "okavango_mt/world_" + std::to_string(i) + ".json";
        Cell_group_builder cb;

        if (!cb.load(file_path)) continue;
        float comp = 0;
        float occ = 0;
        if (!cb.empty()){
            world.set_occlusions(cb);
            auto cell_shape = world.get_configuration().cell_shape;
            auto cell_transformation = world.get_implementation().cell_transformation;
            auto visibility = Coordinates_visibility::create_graph(cells, cell_shape, cell_transformation);
            comp = visibility.get_degree_complexity(2, true);
            occ = (float)cb.size() / (float) cells.size();
        }
        ndc.push_back(comp);
        oc.push_back(occ);
        cout << i << " " << occ << " " << comp << endl;
    }
    ndc.save("okavango_mt/netcomp.json");
    oc.save("okavango_mt/occupancy.json");
}
