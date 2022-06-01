#include <cell_world.h>
#include <params_cpp.h>

using namespace params_cpp;
using namespace cell_world;
using namespace std;
using namespace json_cpp;

Coordinates border(int x){
    Coordinates border(x,20);
    border.y -= abs(x);
    if (border.y>10) border.y=10;
    return border;
}

Cell_group_builder get_clean_route(const Cell_group_builder &route, const Graph &graph){
    Cell_group_builder clean_route;
    for (int s=0; s<route.size(); ){
        clean_route.push_back(route[s]);
        int last_connection = s + 1;
        for (int c=last_connection; c<route.size(); c++){
            if (graph[graph.cells[route[s]]].contains(graph.cells[route[c]])) last_connection = c;
        }
        s = last_connection;
    }
    return clean_route;
}

int main (int argc, char **argv){
    Parser p(argc,argv);
    auto occlusions = p.get(Key("-o","--occlusions"),"00_00");
    World world = World::get_from_parameters_name("hexagonal","canonical", occlusions);
    Graph graph = world.create_graph();
    Cell_group cells = world.create_cell_group();
    Map map = Map(cells);
    Paths paths(world.create_paths(Resources::from("paths").key("hexagonal").key(occlusions).key("astar").get_resource<Path_builder>()));
    auto start = Coordinates(-20,0);

    Cell_group_builder north_route;
    Cell_group_builder south_route;
    auto prev_north = map[start];
    auto prev_south = map[start];
    for (int x = -19; x <=19; x++){
        if (map.find(border(x)) == Not_found) continue;
        auto north_border = map[border(x)];
        if (!north_border.occluded) {
            auto path = paths.get_path(prev_north, north_border);
            for(const Cell &step:path){
                if (step != prev_north)
                    north_route.push_back(step.id);
            }
            prev_north = north_border;
        }
        auto south_border = map[Coordinates(north_border.coordinates.x,-north_border.coordinates.y)];
        if (!south_border.occluded) {
            auto path = paths.get_path(prev_south, south_border);
            for(const Cell &step:path){
                if (step != prev_south)
                    south_route.push_back(step.id);
            }
            prev_south = south_border;
        }
    }
    north_route = get_clean_route (north_route, graph);
    south_route = get_clean_route (south_route, graph);
    cout << north_route << endl;
    cout << south_route << endl;

    exit(0);
}

