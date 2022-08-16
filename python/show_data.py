import glob
from cellworld import *
from src import *
from os.path import exists
import os


planning = [0.0 for x in range(10)]
lppo_planning = [0.0 for x in range(10)]
fixed_trajectory = [0.0 for x in range(10)]
lppo_fixed_trajectory = [0.0 for x in range(10)]
shortest_path = [0.0 for x in range(10)]
thigmotaxis_trajectory = [0.0 for x in range(10)]
reactive_thigmotaxis_trajectory = [0.0 for x in range(10)]

metrics = []
sim_stats = vars(Simulation_statistics())
for metric in sim_stats:
    if isinstance(sim_stats[metric], float):
        metrics.append(metric)

world_metrics = []
world_stats = vars(World_statistics())
for world_metric in world_stats:
    if isinstance(world_stats[world_metric], float):
        world_metrics.append(world_metric)

def write_metrics(file, sim_type, clustering, world_number, entropy_bucket, stats_file, metrics, world_stats, world_metrics):
    print(world_number, entropy_bucket, sim_type)
    simulation_statistics = vars(Simulation_statistics.load_from_file(stats_file))
    file.write(sim_type)
    file.write(",")
    file.write(clustering)
    file.write(",")
    file.write(world_number)
    file.write(",")
    file.write(entropy_bucket)
    for world_metric in world_metrics:
        results_file.write(",")
        results_file.write(str(world_stats[world_metric]))
    for metric in metrics:
        results_file.write(",")
        results_file.write(str(simulation_statistics[metric]))
    results_file.write("\n")


with open("results.csv", "w") as results_file:
    results_file.write("sim_type, clustering, world_number, entropy_bucket")

    for world_metric in world_metrics:
        results_file.write(",")
        results_file.write(world_metric)

    for metric in metrics:
        results_file.write(",")
        results_file.write(metric)
    results_file.write("\n")

    for x in glob.glob("*"):
        if os.path.isdir(x):
            if len(x.split(".")) == 1:
                continue
            world = x.split(".")[1]
            world_number = world.split("_")[0]
            entropy_bucket = world.split("_")[1]
            clustering = "no_limit"
            if world_number[0] == 2:
                clustering = "75%"
            elif world_number[0] == 3:
                clustering = "50%"
            elif world_number[0] == 4:
                clustering = "25%"
            elif world_number[0] == 5:
                clustering = "1"

            world_stats = vars(World_statistics.load_from_file(x + "/world_statistics"))

            if exists(x + "/planning_simulation_stats.json"):
                write_metrics(results_file, "POMCP", clustering, world_number, entropy_bucket, x + "/planning_simulation_stats.json", metrics, world_stats, world_metrics)

            if exists(x + "/fixed_trajectory_simulation_stats.json"):
                write_metrics(results_file, "FIXED_POMCP_TRAJECTORY", clustering, world_number, entropy_bucket, x + "/fixed_trajectory_simulation_stats.json", metrics, world_stats, world_metrics)

            if exists(x + "/lppo_planning_simulation_stats.json"):
                write_metrics(results_file, "LPPO", clustering, world_number, entropy_bucket, x + "/lppo_planning_simulation_stats.json", metrics, world_stats, world_metrics)

            if exists(x+"/fixed_lppo_trajectory_simulation_stats.json"):
                write_metrics(results_file, "FIXED_LPPO_TRAJECTORY", clustering, world_number, entropy_bucket, x + "/fixed_lppo_trajectory_simulation_stats.json", metrics, world_stats, world_metrics)

            if exists(x+"/thigmotaxis_simulation_stats.json"):
                write_metrics(results_file, "THIGMOTAXIS", clustering, world_number, entropy_bucket, x + "/thigmotaxis_simulation_stats.json", metrics, world_stats, world_metrics)

            if exists(x+"/reactive_thigmotaxis_simulation_stats.json"):
                write_metrics(results_file, "REACTIVE_THIGMOTAXIS", clustering, world_number, entropy_bucket, x + "/reactive_thigmotaxis_simulation_stats.json", metrics, world_stats, world_metrics)

            if exists(x+"/shortest_path_simulation_stats.json"):
                write_metrics(results_file, "ASTAR", clustering, world_number, entropy_bucket, x + "/shortest_path_simulation_stats.json", metrics, world_stats, world_metrics)




