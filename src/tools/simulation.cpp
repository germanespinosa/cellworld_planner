#include <cell_world.h>
#include <cellworld_planner/prey.h>

using namespace std;
using namespace cell_world;
using namespace cell_world::planner;

int main(int argc, char **argv){
    auto world = World::get_from_parameters_name("hexagonal","canonical","20_05");
    Static_data data(world);
    data.possible_destinations = data.cells.free_cells();
    Graph_builder visibility_builder = Resources::from("graph").key("hexagonal").key("20_05").key("cell_visibility").get_resource<Graph_builder>();
    data.visibility = world.create_graph(visibility_builder);
    data.inverted_visibility = data.visibility.invert();
    data.predator_start_locations = data.inverted_visibility[data.start_cell].free_cells();
    data.predator_moves = world.connection_pattern;
    data.predator_moves.push_back(Coordinates(0,0));
    data.predator_best_move_probability = 1;
    data.max_particle_count = 1000;
    data.max_particle_creation_attempts = 10000;
    Model model(data.cells);
    Prey prey(data);
    Predator predator(data);

    model.add_agent(prey);
    model.add_agent(predator);
    model.start_episode();
    cout << predator.public_state().cell.coordinates << endl;
    for(int i =0; i<20;i ++) {
        model.update();
        model.update();
    }
    model.end_episode();
}