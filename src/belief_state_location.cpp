#include <cellworld_planner/belief_state_location.h>
#include <json_cpp.h>

using namespace json_cpp;
using namespace cell_world;
using namespace std;

planner::Belief_state_location::Belief_state_location(const Static_data &data):
    data(data),
    visibility(data.cells,data.world.cell_shape,data.world.cell_transformation),
    prey(data),
    predator(data),
    model(data.cells, 1000){
    model.add_agent(prey);
    model.add_agent(predator);
}

void planner::Belief_state_location::record_state(const Model_public_state &state, const Location &prey_location, float prey_orientation, const Location &predator_location, bool is_visible) {
    PERF_SCOPE("Belief_state::record_state");
    auto &prey_state = state.agents_state[PREY];
    auto &predator_state = state.agents_state[PREDATOR];
    // if the predator is visible
    if (is_visible){
        // then history collapses
        history = {};
        model.set_public_state(state);
        if (state.current_turn == PREDATOR) {
            model.update(); // if the state is post-turn, evolve the particle
        } else {
            predator.update_state(state); // lets the predator update the internal state
        }
        // this state becomes the root state for future particles
        root_state = model.get_state();
        // reset all particles
        particles.clear();
        for(int i=0;i<data.simulation_parameters.tree_search_parameters.belief_state_parameters.max_particle_count;i++) particles.push_back(root_state);
        previous_prey_coordinates = prey_state.cell.coordinates;
    } else
    // if the predator is not visible
    {
        // if the state is post-turn, no need to filter yet
        if (state.current_turn == PREY) {
            Move last_move = prey_state.cell.coordinates - previous_prey_coordinates;
            history.push_back({last_move,prey_location,prey_orientation});
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
                if (!visibility.is_visible(prey_location, prey_orientation, to_radians(270), predator_cell.location)) {
                    new_particles.push_back(model.get_state());
                }
                model.end_episode();
            }
            for (int i = 0;
                 i < data.simulation_parameters.tree_search_parameters.belief_state_parameters.max_particle_creation_attempts && new_particles.size() < data.simulation_parameters.tree_search_parameters.belief_state_parameters.max_particle_count; i++) {
                auto root_state = get_root_state();
                bool valid = true;
                model.set_state(root_state);
                for (auto move: history) {
                    prey.next_move = move.move;
                    model.update(); // PREDATOR's turn
                    model.update(); // PREY's turn
                    auto &prey_cell = model.state.public_state.agents_state[PREY].cell;
                    auto &predator_cell = model.state.public_state.agents_state[PREDATOR].cell;
                    if (visibility.is_visible(move.prey_location, move.prey_orientation, to_radians(270), predator_cell.location)) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    new_particles.push_back(model.get_state());
                }
                model.end_episode();
            }
            particles = new_particles;
            previous_prey_coordinates = prey_state.cell.coordinates;
        }
    }
}

Model_state planner::Belief_state_location::get_particle() {
    return pick_random(particles);
}

Model_state planner::Belief_state_location::get_root_state() {
    PERF_SCOPE("Belief_state::get_root_state");
    if (root_state.public_state.agents_state.empty()) {
        model.start_episode();
        auto state = model.get_state();
        model.end_episode();
        return state;
    } else {
        return root_state;
    }
}

void planner::Belief_state_location::reset() {
    PERF_SCOPE("Belief_state::reset");
    history.clear();
    previous_prey_coordinates = data.start_cell().coordinates;
    particles.clear();
    if (model.state.public_state.status == Model_public_state::Status::Running) model.end_episode();
    for (int i=0;i<data.simulation_parameters.tree_search_parameters.belief_state_parameters.max_particle_creation_attempts && particles.size()<data.simulation_parameters.tree_search_parameters.belief_state_parameters.max_particle_count;i++){
        model.start_episode();
        particles.push_back(model.get_state());
        model.end_episode();
    }
}

Json_unsigned_int_vector planner::Belief_state_location::get_particle_count() {
    cell_world::Json_unsigned_int_vector particle_count(data.cells.size(), 0);
    for (auto &particle: particles) {
        particle_count[particle.public_state.agents_state[PREDATOR].cell.id]++;
    }
    return particle_count;
}
