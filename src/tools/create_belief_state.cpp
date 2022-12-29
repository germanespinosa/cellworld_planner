#include <chrono>
#include <performance.h>
#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <cell_world/pose.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <cellworld_planner/statistics.h>
#include <cellworld_planner/belief_state_variable_time.h>
#include <thread_pool.h>

using namespace thread_pool;
using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;


struct Agents_cells : json_cpp::Json_object {
    Json_object_members(
            Add_member(prey);
            Add_member(prey_location);
            Add_member(prey_orientation);
            Add_member(predator_location);
            Add_member(predator);
            Add_member(los);
    )
    Agents_cells() = default;
    Agents_cells(int prey, int predator, unsigned int frame, Location &prey_location, float prey_orientation, Location &predator_location):
            prey(prey),
            predator(predator),
            frame(frame),
            prey_location(prey_location),
            prey_orientation(prey_orientation),
            predator_location(predator_location){}
    int prey{Not_found};
    int predator{Not_found};
    unsigned int frame{};
    Location prey_location{};
    float prey_orientation{};
    Location predator_location{};
    bool los{};
};

struct Simulation_step_data : json_cpp::Json_object {
    Simulation_step_data() = default;
    explicit Simulation_step_data (unsigned int frame) : frame(frame) {};
    Json_object_members(
            Add_member(frame);
    )
    unsigned int frame{};
};

struct Line_of_sight  : json_cpp::Json_object {
    Json_object_members(
            Add_member_with_name(visible,true,"LOS");
    )
    bool visible;
};

struct Step_additional_info : json_cpp::Json_object {
    Json_object_members(
            Add_member(frame);
            Add_member(time_stamp);
            Add_member(belief_state);
            Add_member(mouse_visibility);
            Add_member(mouse_exposure);
            Add_member(weighted_visibility);
            Add_member(weighted_exposure);
            Add_member(itor);
            Add_member(weighted_itor);
            Add_member(robot_vertices);
            Add_member(robot_theta);
            Add_member(mouse_head);
            Add_member(mouse_body);
            Add_member(mouse_theta);
            Add_member(mouse_speed);
            Add_member(mouse_acceleration);
            Add_member(los);
    )
    unsigned int frame;
    float time_stamp;
    Json_float_vector belief_state;
    Json_bool_vector mouse_visibility;
    Json_bool_vector mouse_exposure;
    float weighted_visibility;
    float weighted_exposure;
    float itor;
    float weighted_itor;
    Location_list robot_vertices;
    float robot_theta;
    Location mouse_head;
    Location mouse_body;
    float mouse_theta;
    float mouse_speed;
    float mouse_acceleration;
    bool los;
};

float robot_width = 0; //.08 / 2.34;
float robot_length = 0;//.12 / 2.34;

Location_list get_robot_vertices( const Location &robot_center, float theta) {
    Location_list robot_vertices;
    Location c;
    auto f = c.move(theta, robot_length / 2);
    auto l = c.move(theta - M_PI / 2, robot_width / 2);
    robot_vertices.emplace_back(robot_center + f + l);
    robot_vertices.emplace_back(robot_center + f - l);
    robot_vertices.emplace_back(robot_center - f + l);
    robot_vertices.emplace_back(robot_center - f - l);
    return robot_vertices;
}

void create_additional_step_data (const Static_data &data,
                                  json_cpp::Json_vector<Step_additional_info>& belief_state_progress,
                                  const Episode &experiment_episode,
                                  size_t episode_number) {
    Agents_cells agent_cells;
    agent_cells.prey = data.start_cell_id;
    agent_cells.prey_orientation = Location(0,.5).atan(Location(1,.5));
    agent_cells.prey_location = Location(0,.5);
    Location_list cell_centers = data.cells.process<Location>([](const Cell &cell){return cell.location;});
    Location_visibility lv(data.cells, data.world.cell_shape, data.world.cell_transformation);
    Belief_state_variable_time belief_state (data);
    Model_public_state state;
    auto prey_state = state.agents_state.emplace_back();
    auto predator_state = state.agents_state.emplace_back();
    prey_state.agent_index = PREY;
    predator_state.agent_index = PREDATOR;
    int last_f = -1;
    float last_t = experiment_episode.trajectories[0].time_stamp;
    float delta_t = 0;
    bool new_data = false;
    float mouse_orientation;
    float robot_orientation;
    Pose mouse_pose;
    Location mouse_body, mouse_head;
    bool los = false;

    bool new_frame = false;
    bool new_frame_prey_data = false;
    bool new_frame_predator_data = false;
    Location last_mouse_body;
    float last_mouse_speed = 0;
    bool first = true;
    Json_unsigned_int_vector prev_particles = data.cells.process<unsigned int>([](const Cell &c){return c.occluded?0:1;});
    for (auto &step : experiment_episode.trajectories) {
        if (last_f != step.frame) {
            delta_t = step.time_stamp - last_t;
            last_t = step.time_stamp;
            last_f = step.frame;
            new_frame = true;
            new_frame_predator_data = false;
            new_frame_prey_data = false;
        }
        int cell_id = data.cells.find(step.location);
        if (step.agent_name == "prey") {
            new_frame_prey_data = true;
            agent_cells.prey = cell_id;
            agent_cells.prey_orientation = to_radians(step.rotation);
            mouse_orientation = agent_cells.prey_orientation;
            last_mouse_body = mouse_body;
            mouse_body = step.location;
            bool worked = false;
            try {
                mouse_pose.from_json(step.data);
                worked = true;
            } catch (...) {

            }
            if (worked) {
                agent_cells.prey_location = mouse_pose.find_first([](const Pose_part &pp) {
                    return pp.part == "head_base";
                }).location;
            } else {
                agent_cells.prey_location = step.location;
            }
            mouse_head = agent_cells.prey_location;
            agent_cells.los = lv.is_visible(agent_cells.prey_location, agent_cells.predator_location);
        } else {
            new_frame_predator_data = true;
            agent_cells.predator = cell_id;
            agent_cells.predator_location = step.location;
            robot_orientation = to_radians(step.rotation);
        }
        if (!new_frame || !new_frame_prey_data || !new_frame_predator_data) continue;
        new_frame = false;
        if (delta_t <= 0) continue;

        auto vertices = get_robot_vertices(agent_cells.predator_location, agent_cells.prey_orientation);

        los = false;
        for (auto &vertex: vertices) {
            los = los || lv.is_visible(agent_cells.prey_location, vertex);
            if (los) break;
        }
        state.agents_state[PREY].cell = data.cells[agent_cells.prey];
        state.agents_state[PREDATOR].cell = data.cells[agent_cells.predator];
        state.status = cell_world::Model_public_state::Running;
        state.current_turn = PREY;
        belief_state.record_state(state,
                                  agent_cells.prey_location,
                                  agent_cells.prey_orientation,
                                  agent_cells.predator_location,
                                  los,
                                  delta_t);
        state.agents_state[PREY].iteration++;
        state.agents_state[PREDATOR].iteration++;

        auto &particle_count = belief_state_progress.emplace_back();
        particle_count.frame = last_f;
        Json_unsigned_int_vector particles = belief_state.get_particle_count();
        float total_particles = prev_particles.sum([](auto c){ return c; });
        particle_count.belief_state = prev_particles.process<float>([total_particles](auto c){ return total_particles>0?((float)c) / total_particles:0; });
        particle_count.robot_vertices = vertices;
        particle_count.mouse_body = mouse_body;
        particle_count.mouse_head = mouse_head;
        particle_count.los = los;
        particle_count.mouse_theta = mouse_orientation;
        particle_count.robot_theta = robot_orientation;
        particle_count.time_stamp = step.time_stamp;
        if (first) {
            particle_count.mouse_speed = 0;
            particle_count.mouse_acceleration = 0;
            first = false;
        } else {
            particle_count.mouse_speed = last_mouse_body.dist(mouse_body) / delta_t * 2.34;
        }
        particle_count.mouse_acceleration = (last_mouse_speed - particle_count.mouse_speed) / delta_t * 2.34;
        last_mouse_speed = particle_count.mouse_speed;
        particle_count.mouse_visibility = lv.is_visible_multi(particle_count.mouse_head, 0, M_PI * 2, cell_centers);
        particle_count.mouse_exposure = lv.is_visible_multi(particle_count.mouse_body, 0, M_PI * 2, cell_centers);
        particle_count.weighted_visibility = 0;
        particle_count.weighted_exposure = 0;
        for(size_t i = 0; i < particle_count.belief_state.size(); i++){
            particle_count.weighted_visibility += particle_count.mouse_visibility[i] ? particle_count.belief_state[i] : 0;
            particle_count.weighted_exposure += particle_count.mouse_exposure[i] ? particle_count.belief_state[i] : 0;
        }
        auto v = particle_count.mouse_visibility.sum([](auto v){ return v?1:0;});
        auto e = particle_count.mouse_exposure.sum([](auto v){ return v?1:0;});
        if (v!=0)
            particle_count.itor = (v-e) / v;
        else
            particle_count.itor = 0;

        if (particle_count.weighted_visibility!=0)
            particle_count.weighted_itor = (particle_count.weighted_visibility - particle_count.weighted_exposure) / particle_count.weighted_visibility ;
        else
            particle_count.weighted_itor = 0;

        prev_particles = particles;
    }
    belief_state_progress.save("../results/episode_" + to_string(episode_number));
}

int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto experiment_file = p.get(Key("-e", "--experiment"), "");
    auto simulation_file = p.get(Key("-b", "--belief_state_file"), "");
    auto configuration_file = p.get(Key("-c","--configuration_file"),"");

    if (experiment_file.empty()) {
        cout << "Missing experiment file parameter." << endl;
        exit(1);
    }

    if (configuration_file.empty()){
        cout << "Missing simulation configuration file parameter." << endl;
        exit(1);
    }

    int workers = 8;
    Thread_pool tp;
    Experiment experiment;
    if (!experiment.load(experiment_file)){
        cout << "Failed to open experiment file." << endl;
        exit(1);
    }
    World_info world_info (experiment.world_configuration_name, "canonical", experiment.occlusions);
    World world = World::get_from_world_info(world_info);

    Static_data data(world_info);
    data.predator_moves = world.connection_pattern;
    data.predator_moves.push_back(Coordinates(0,0));
    data.simulation_parameters = json_cpp::Json_from_file<Simulation_parameters>(configuration_file);

    auto cells = world.create_cell_group();
    Prey_state prey_state;

    json_cpp::Json_vector<json_cpp::Json_vector<Step_additional_info>> episodes_additional_data(experiment.episodes.size());
    for (int e = 0 ; e < experiment.episodes.size(); e++) {
        auto &episode = experiment.episodes[e];
        cout << "Processing trajectories " << e << endl;
        auto &sim_episode = episodes_additional_data[e];
        tp.run([&data, &sim_episode, &episode, e]() {
            create_additional_step_data(data, sim_episode, episode, e);
        });
    }
    tp.wait_all();
    episodes_additional_data.save(simulation_file + ".json");
}
