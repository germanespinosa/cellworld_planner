#!/usr/bin/env python3
from src import *
from sys import argv
from matplotlib import pyplot as plt


def find_x_values(y, values) -> list:
    x_values = []
    for x, v in enumerate(values):
        if x == 0:
            continue
        if y == v:
            x_values.append(x)
        if values[x - 1] < y < v:
            new_x = x + (y - values[x - 1]) / (v - values[x - 1]) - 1
            x_values.append(new_x)
        if values[x - 1] > y > v:
            new_x = x - (y - v) / (values[x - 1] - v)
            x_values.append(new_x)
    return x_values


if len(argv) < 2:
    exit(0)

comparison = Comparison.load_from_file(argv[1])

length = []
value = []
survival_rate = []
capture_rate = []
distance = []
belief_state_entropy = []
pursue_rate = []
labels = []
visited_cells = []
decision_difficulty = []
for dp in comparison.data_points:
    print("Processing " + dp.label)
    labels.append(dp.label)
    stats = Simulation_statistics.load_from_sim_file_name(dp.file_name)
    if stats is None:
        s = Simulation.load_from_file(dp.file_name)
        stats = s.get_stats()
    length.append(stats.length)
    value.append(stats.value)
    survival_rate.append(stats.survival_rate)
    capture_rate.append(stats.capture_rate)
    distance.append(stats.distance)
    belief_state_entropy.append(stats.belief_state_entropy)
    pursue_rate.append(stats.pursue_rate)
    visited_cells.append(stats.visited_cells)
    decision_difficulty.append(stats.decision_difficulty)

fig, axs = plt.subplots(3, 3, figsize=(12, 10))
fig.suptitle(comparison.name)

plt.setp(axs, xticks=list(range(len(labels))), xticklabels=labels)
axs[0, 0].plot(length)
axs[0, 0].set_title('Episode length')
axs[1, 0].plot(value)
axs[1, 0].set_title('Avg Value')
axs[2, 0].plot(distance)
axs[2, 0].set_title('Avg predator-prey dist.')
axs[0, 1].plot(survival_rate)
axs[0, 1].set_title('Survival rate')
axs[0, 1].set_ylim([0, 1])
axs[1, 1].plot(capture_rate)
axs[1, 1].set_title('Capture rate')
axs[2, 1].plot(belief_state_entropy)
axs[2, 1].set_title('Belief state entropy')
#axs[2, 1].set_ylim([0, 1])
axs[0, 2].plot(pursue_rate)
axs[0, 2].set_title('Pursue rate')
axs[1, 2].plot(visited_cells)
axs[1, 2].set_title('Visited cells')
axs[2, 2].plot(decision_difficulty)
axs[2, 2].set_title('Decision difficulty')


if comparison.marks:
    for mark in comparison.marks:
        extra = Simulation_statistics.load_from_sim_file_name(mark.data_point.file_name)
        if extra is None:
            s = Simulation.load_from_file(mark.data_point.file_name)
            extra = s.get_stats()
        extra_length = find_x_values(extra.length, length)
        axs[0, 0].scatter(extra_length, [extra.length for x in range(len(extra_length))], c=mark.color)
        axs[0, 0].plot([extra.length for x in range(len(length))], c=mark.color)
        extra_value = find_x_values(extra.value, value)
        axs[1, 0].scatter(extra_value, [extra.value for x in range(len(extra_value))], c=mark.color)
        axs[1, 0].plot([extra.value for x in range(len(length))], c=mark.color)
        extra_distance = find_x_values(extra.distance, distance)
        axs[2, 0].scatter(extra_distance, [extra.distance for x in range(len(extra_distance))], c=mark.color)
        axs[2, 0].plot([extra.distance for x in range(len(length))], c=mark.color)
        extra_survival_rate = find_x_values(extra.survival_rate, survival_rate)
        axs[0, 1].scatter(extra_survival_rate, [extra.survival_rate for x in range(len(extra_survival_rate))], c=mark.color)
        axs[0, 1].plot([extra.survival_rate for x in range(len(length))], c=mark.color)
        extra_capture_rate = find_x_values(extra.capture_rate, capture_rate)
        axs[1, 1].scatter(extra_capture_rate, [extra.capture_rate for x in range(len(extra_capture_rate))], c=mark.color)
        axs[1, 1].plot([extra.capture_rate for x in range(len(length))], c=mark.color)
        extra_belief_state_entropy = find_x_values(extra.belief_state_entropy, belief_state_entropy)
        axs[2, 1].scatter(extra_belief_state_entropy, [extra.belief_state_entropy for x in range(len(extra_belief_state_entropy))], c=mark.color)
        axs[2, 1].plot([extra.belief_state_entropy for x in range(len(length))], c=mark.color)
        extra_pursue_rate = find_x_values(extra.pursue_rate, pursue_rate)
        axs[0, 2].scatter(extra_pursue_rate, [extra.pursue_rate for x in range(len(extra_pursue_rate))], c=mark.color)
        axs[0, 2].plot([extra.pursue_rate for x in range(len(length))], c=mark.color)
        extra_visited_cells = find_x_values(extra.visited_cells, visited_cells)
        axs[1, 2].scatter(extra_visited_cells, [extra.visited_cells for x in range(len(extra_visited_cells))], c=mark.color)
        axs[1, 2].plot([extra.visited_cells for x in range(len(length))], c=mark.color)
        extra_decision_difficulty = find_x_values(extra.decision_difficulty, decision_difficulty)
        axs[2, 2].scatter(extra_decision_difficulty, [extra.decision_difficulty for x in range(len(extra_decision_difficulty))], c=mark.color)
        axs[2, 2].plot([extra.decision_difficulty for x in range(len(length))], c=mark.color)
plt.tight_layout()
plt.show()

