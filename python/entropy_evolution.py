from cellworld_simulation import *
from json_cpp import JsonObject
from matplotlib import pyplot as plt


experiment_name = "MICE_20220614_1637_DMM1_21_05_JR5_experiment"

experiment = Experiment.get_from_file("../../../experiments/dredds/" + experiment_name + ".json")
simulation = Simulation.load_from_file("../../simulation_results/dredds/" + experiment_name + ".json")

stats = simulation.get_stats(simulation.parameters.reward)

stop_velocities = []
in_stop_velocities = []

stop_belief_state_entropy = []

post_velocities = []
post_belief_state_entropy = []

bf_entropies = [0.0 for x in range(200)]

for i, episode in enumerate(experiment.episodes):
    sim_episode = simulation.episodes[i]
    prey_trajectory = episode.trajectories.get_agent_trajectory("prey")
    episode_velocities_filtered = prey_trajectory.get_filtered_velocities(complementary=.6, outliers=.8)["prey"]
    episode_velocities = prey_trajectory.get_velocities()["prey"]
    stops = prey_trajectory.get_stops()
    post_stops = []

    for b, e in stops:
        end_step = prey_trajectory.get_step_by_frame(e)
        for x in range(200):
            td = - 1.0 + x / 100
            step = prey_trajectory.get_step_by_time_stamp(end_step.time_stamp + td)

    for sim_step in sim_episode:
        frame = JsonObject.load(sim_step.data).frame
        index = prey_trajectory.get_step_index_by_frame(frame)
        if [True for b, e in in_stop_velocities if frame > b and frame < e]:
            stop_velocities.append(episode_velocities[index])
            stop_belief_state_entropy.append(entropy(sim_step.prey_state.belief_state))
        if [True for b, e in post_stops if frame > b and frame < e]:
            post_velocities.append(episode_velocities[index])
            post_belief_state_entropy.append(entropy(sim_step.prey_state.belief_state))


#plt.scatter(stop_belief_state_entropy, stop_velocities, color="red")
print (sum(stop_belief_state_entropy)/len(stop_belief_state_entropy))
plt.plot(episode_velocities)
plt.plot(episode_velocities_filtered)

plt.show()

# plt.scatter(post_belief_state_entropy, post_velocities, color="blue")
# print (sum(post_belief_state_entropy)/len(post_belief_state_entropy))


