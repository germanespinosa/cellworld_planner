#include <cell_world.h>
#include <params_cpp.h>
#include <algorithm>
#include <random>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;


int main (int argc, char **argv){
    Parser p(argc,argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"00_00");
    auto depth = stoi(p.get(Key("-d","--depth"),"1"));
    auto lppo_count = stoi(p.get(Key("-k","--lppo_count"),"10"));
    World world = World::get_from_parameters_name("hexagonal","canonical", occlusions);
    Graph graph = world.create_graph();
    Cell_group cells = world.create_cell_group().free_cells();
    Map map(cells);
    auto world_centrality = graph.get_centrality(depth);
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

    next_threshold = derivative_product.max([](auto e){return e;});

    Cell_group_builder lppos_indexes;

    for (const Cell &cell :cells) {
        if (cell.coordinates==Coordinates(-20,0) || cell.coordinates==Coordinates(20,0)) lppos_indexes.push_back(cell.id);
    }

    auto index = cells.get_builder();
    auto rng = std::default_random_engine {};
    std::shuffle(index.begin(), index.end(), rng);

    while (lppos_indexes.size()<lppo_count){
        for (auto i: index) {
            if (lppos_indexes.size()>=lppo_count) break;
            if (lppos_indexes.contains(i)) continue;
            if (derivative_product[i]>=threshold) lppos_indexes.push_back(i);
        }
        threshold = next_threshold;
        next_threshold = 0;
        for (auto cell_derivative_product:derivative_product) {
            if (cell_derivative_product<threshold && cell_derivative_product>next_threshold) next_threshold=cell_derivative_product;
        }
    }
    Cell_group lppos;
    for (const Cell &cell :cells) {
        if (lppos_indexes.contains(cell.id)) lppos.add(cell);
    }
    cout << lppos << endl;
}

