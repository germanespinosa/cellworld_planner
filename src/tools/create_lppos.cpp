#include <cell_world.h>
#include <params_cpp.h>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;


int main (int argc, char **argv){
    Parser p(argc,argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"00_00");
    auto depth = stoi(p.get(Key("-d","--depth"),"1"));
    auto percentage = stoi(p.get(Key("-p","--percentage"),"10"));
    World world = World::get_from_parameters_name("hexagonal","canonical", occlusions);
    Graph graph = world.create_graph();
    Cell_group cells = world.create_cell_group().free_cells();
    Map map(cells);
    auto world_centrality = graph.get_centrality();
    int lppo_count = (float(world_centrality.size()) * percentage)/100;
    Json_vector<float> derivative_product (world_centrality.size(),0);
    auto pairs = world.connection_pattern.get_pairs();
    for (const Cell &cell:cells){
        float cell_centrality_derivative_product = 1;
        auto cell_coordinates = cell.coordinates;
        for (auto &pair : pairs){
            auto first_coordinates = cell_coordinates + pair[0];
            float first_centrality = 0;
            if (map.find(first_coordinates)!=Not_found) first_centrality = world_centrality[map[first_coordinates].id];

            auto second_coordinates = cell_coordinates + pair[1];
            float second_centrality = 0;
            if (map.find(second_coordinates)!=Not_found) second_centrality = world_centrality[map[second_coordinates].id];
            cell_centrality_derivative_product = cell_centrality_derivative_product * (first_centrality-second_centrality);
        }
        derivative_product[cell.id] = cell_centrality_derivative_product;
    }
    float threshold = 0;
    float next_threshold = 0;
    int candidates_counter = 0;
    for (auto cell_derivative_product:derivative_product) if (cell_derivative_product>next_threshold) next_threshold = cell_derivative_product;
    while (candidates_counter<lppo_count){
        threshold = next_threshold;
        next_threshold = 0;
        candidates_counter = 0;
        for (auto cell_derivative_product:derivative_product) {
            if (cell_derivative_product>=threshold) candidates_counter++;
            if (cell_derivative_product<threshold && cell_derivative_product>next_threshold) next_threshold=cell_derivative_product;
        }
    }
    Cell_group lppos;
    for (const Cell &cell:graph.cells) {
        if (derivative_product[cell.id]>=threshold || cell.coordinates==Coordinates(-20,0) || cell.coordinates==Coordinates(20,0)) lppos.add(cell);
    }
    cout << lppos << endl;
}

