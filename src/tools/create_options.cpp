#include <cell_world.h>
#include <params_cpp.h>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;

Json_vector<Coordinates_list> get_connection_pairs(Connection_pattern &cp){
    Json_vector<Coordinates_list> pairs;
    for (auto &cnn:cp){
        bool present = false;
        for (auto &pair: pairs){
            for (auto &c:pair) if (c==cnn) present = true;
        }
        if (!present) {
            Coordinates_list pair;
            pair.push_back(cnn);
            pair.push_back(-cnn);
            pairs.push_back(pair);
        }
    }
    return pairs;
}

int main (int argc, char **argv){
    Parser p(argc,argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"20_05");
    World world = World::get_from_parameters_name("hexagonal","canonical", occlusions);
    Graph graph = world.create_graph();
    Graph options(graph.cells);
    Path_builder paths_builder = Resources::from("paths").key("hexagonal").key(occlusions).key("astar").get_resource<Path_builder>();
    Cell_group_builder lppos_builder = Resources::from("cell_group").key("hexagonal").key(occlusions).key("lppo").get_resource<Cell_group_builder>();
    Paths paths = world.create_paths(paths_builder);
    Cell_group lppos = world.create_cell_group(lppos_builder);
    for (const Cell &cell:graph.cells){
        if (cell.occluded) continue;
        for (const Cell &lppo:lppos){
            auto option_path = paths.get_path(cell, lppo);
            if (cell.id == 137 && lppo.id == 94){
                cout << cell << endl;
                cout << lppo << endl;
                cout << "here: " << option_path << endl;
            }
            bool is_option = true;
            for (const Cell &step:option_path){
                if (step == cell || step == lppo) continue;
                if (lppos.contains(step)) {
                    is_option = false;
                    break;
                }
            }
            if (is_option) options[cell].add(lppo);
        }
    }
    cout <<  options << endl;
}

