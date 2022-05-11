#include <cellworld_planner/belief_state.h>
#include <json_cpp.h>

using namespace json_cpp;
using namespace cell_world;
using namespace std;

planner::Dummy::Dummy(const planner::Static_data &data):
        data(data){
}

const Cell &planner::Dummy::start_episode() {
    return data.start_cell;
}

Move planner::Dummy::get_move(const Model_public_state &) {
    return next_move;
}

planner::Belief_state::Belief_state(const Static_data &data):

    data(data),
    prey(data),
    predator(data),
    model(data.cells){
    model.add_agent(prey);
    model.add_agent(predator);
    previous_prey_coordinates = data.start_cell.coordinates;
    for (int i=0;i<data.max_particle_creation_attempts && particles.size()<data.max_particle_count;i++){
        model.start_episode();
        particles.push_back(model.get_state());
        model.end_episode();
    }
}

struct Belief_state_progress : json_cpp::Json_object {
    Json_object_members(
            Add_member(prey_cell);
            Add_member(hits);
    )
    Coordinates prey_cell;
    Json_vector<int> hits;
};

void planner::Belief_state::record_state(const Model_public_state &state) {
    auto &prey_state = state.agents_state[PREY];
    auto &predator_state = state.agents_state[PREDATOR];
    // if the predator is visible
    if (data.visibility[prey_state.cell].contains(predator_state.cell)){
        // then history collapses
        history = {};

        model.set_public_state(state);
        // if the state is post-turn, evolve
        if (state.current_turn == PREDATOR) model.update();

        // this state becomes the root state for future particles
        root_state = model.get_state();

        // reset all particles
        particles.clear();
        for(int i=0;i<data.max_particle_count;i++) particles.push_back(root_state);
        previous_prey_coordinates = prey_state.cell.coordinates;
    } else
    // if the predator is not visible
    {
        // if the state is post-turn, no need to filter yet
        if (state.current_turn == PREY) {
            Move last_move = prey_state.cell.coordinates - previous_prey_coordinates;
            history.push_back(last_move);
            // evolve all particles
            for (auto &particle: particles) {
                model.set_state(particle);
                model.update();
            }
            vector<Model_state> new_particles;
            for (auto &particle: particles) {
                model.set_state(particle);
                prey.next_move = last_move;
                model.update(); // PREDATOR's turn
                model.update(); // PREY's turn
                auto &prey_cell = model.state.public_state.agents_state[PREY].cell;
                auto &predator_cell = model.state.public_state.agents_state[PREDATOR].cell;
                if (!data.visibility[prey_cell].contains(predator_cell)) {
                    new_particles.push_back(model.get_state());
                }
                model.end_episode();
            }
            for (int i=0; i<data.max_particle_creation_attempts && new_particles.size()<data.max_particle_count; i++){
                auto root_state = get_root_state();
                bool valid = true;
                model.set_state(root_state);
                for (auto move:history){
                    prey.next_move = move;
                    model.update(); // PREDATOR's turn
                    model.update(); // PREY's turn
                    auto &prey_cell = model.state.public_state.agents_state[PREY].cell;
                    auto &predator_cell = model.state.public_state.agents_state[PREDATOR].cell;
                    if (data.visibility[prey_cell].contains(predator_cell)) {
                        valid  = false;
                        break;
                    }
                }
               if (valid){
                    new_particles.push_back(model.get_state());
                }
                model.end_episode();
            }
            particles = new_particles;
            //show progress
            Belief_state_progress progress;
            progress.hits = Json_vector<int>(data.world.size(), 0);
            for (auto &p : particles){
                progress.hits[p.public_state.agents_state[PREDATOR].cell.id] ++;
            }
            auto &prey_cell = data.map[previous_prey_coordinates];
            progress.prey_cell = prey_cell.coordinates;
            cout << progress << "," << endl;
            //show progress

            previous_prey_coordinates = prey_state.cell.coordinates;
        }
    }
}

Model_state planner::Belief_state::get_particle() {
    return particles[Chance::dice(particles.size())];
}

Model_state planner::Belief_state::get_root_state() {
    if (root_state.public_state.agents_state.empty()) {
        model.start_episode();
        auto state = model.get_state();
        model.end_episode();
        return state;
    } else {
        return root_state;
    }
}
