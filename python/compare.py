from src import *
from glob import glob
from sys import argv
from matplotlib import pyplot as plt
if len(argv) < 2:
    exit(0)

comparison = Comparison.load_from_file(argv[1])
reward = Reward(step_cost=1, capture_cost=100)

a = Stats()

length = []
value = []
survival_rate = []
capture_rate = []
distance = []
belief_state_entropy = []
pursue_rate = []
labels = []
visited_cells = []
for dp in comparison.data_points:
    labels.append(dp.label)
    s = Simulation.load_from_file(dp.file_name)
    stats = s.get_stats(reward=reward)
    length.append(stats.length)
    value.append(stats.value)
    survival_rate.append(stats.survival_rate)
    capture_rate.append(stats.capture_rate)
    distance.append(stats.distance)
    belief_state_entropy.append(stats.belief_state_entropy)
    pursue_rate.append(stats.pursue_rate)
    visited_cells.append(stats.visited_cells)


fig, axs = plt.subplots(4, 2, figsize=(12, 10))
fig.suptitle(comparison.name)

plt.setp(axs,xticks=[0, 1, 2, 3, 4], xticklabels=labels)
axs[0, 0].plot(length)
axs[0, 0].set_title('Episode length')
axs[1, 0].plot(value)
axs[1, 0].set_title('Avg Value')
axs[2, 0].plot(distance)
axs[2, 0].set_title('Avg distance')
axs[0, 1].plot(survival_rate)
axs[0, 1].set_title('Survival rate')
axs[0, 1].set_ylim([0, 1])
axs[1, 1].plot(capture_rate)
axs[1, 1].set_title('Capture rate')
axs[2, 1].plot(belief_state_entropy)
axs[2, 1].set_title('Belief state entropy')
#axs[2, 1].set_ylim([0, 1])
axs[3, 0].plot(pursue_rate)
axs[3, 0].set_title('Pursue rate')
axs[3, 1].plot(visited_cells)
axs[3, 1].set_title('visited cells')
plt.tight_layout()
plt.show()