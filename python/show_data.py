import glob
from cellworld import *
from src import *
from os.path import exists
import os
import sys

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

def write_metrics(file, sim_type, branches, clustering, world_number, entropy_bucket, stats_file, metrics, world_stats, world_metrics):
    print(clustering, world_number, entropy_bucket, sim_type)
    simulation_statistics = vars(Simulation_statistics.load_from_file(stats_file))
    line = "%s,%i,%s,%s,%s" % (sim_type, branches, clustering, world_number, entropy_bucket)
    for world_metric in world_metrics:
        line += ",%f" % (world_stats[world_metric],)
    for metric in metrics:
        line += ",%f" % (simulation_statistics[metric],)
    #discounts time_outs
    adjusted_success_rate = simulation_statistics["success_rate"] / (1-simulation_statistics["time_out_rate"])
    line += ",%f" % adjusted_success_rate
    return line, adjusted_success_rate


results_file = "results.csv"
if len(sys.argv) > 1:
    results_file = sys.argv[1]

with open(results_file, "w") as results_file:
    results_file.write("sim_type,branches_sampled,clustering,world_number,entropy_bucket")

    for world_metric in world_metrics:
        results_file.write(",")
        results_file.write(world_metric)

    for metric in metrics:
        results_file.write(",")
        results_file.write(metric)
    results_file.write(",adjusted_success_rate,best_model_free_algorithm,best_model_free_algorithm_success_rate,planning_benefit,lppo_benefit\n")

    for x in glob.glob("*"):
        if os.path.isdir(x):
            world = "21_05"
            world_number = world.split("_")[0]
            entropy_bucket = world.split("_")[1]
            clustering = "100% of occlusion count"
            if world_number[0] == '2':
                clustering = "75% of occlusion count"
            elif world_number[0] == '3':
                clustering = "50% of occlusion count"
            elif world_number[0] == '4':
                clustering = "25% of occlusion count"
            elif world_number[0] == '5':
                clustering = "random"

            world_stats = vars(World_statistics.load_from_file(x + "/world_statistics"))
            branches = 0
            lines = []
            best_model_free_algorithm = ""
            best_model_free_algorithm_success_rate = 0
            lppo_success_rate = 0
            pomcp_success_rate = 0
            if exists(x+"/shortest_path_simulation_stats.json") and exists(x+"/shortest_path_simulation.json"):
                branches = Simulation.load_from_file(x+"/shortest_path_simulation.json").parameters.tree_search_parameters.simulations
                line, success_rate = write_metrics(results_file, "ASTAR", branches, clustering, world_number, entropy_bucket, x + "/shortest_path_simulation_stats.json", metrics, world_stats, world_metrics)
                lines.append(line)
                if success_rate > best_model_free_algorithm_success_rate:
                    best_model_free_algorithm_success_rate = success_rate
                    best_model_free_algorithm = "ASTAR"

            if exists(x + "/fixed_trajectory_simulation_stats.json") and exists(x + "/fixed_trajectory_simulation.json"):
                line, success_rate = write_metrics(results_file, "FIXED_POMCP_TRAJECTORY", branches, clustering, world_number, entropy_bucket, x + "/fixed_trajectory_simulation_stats.json", metrics, world_stats, world_metrics)
                lines.append(line)
                if success_rate > best_model_free_algorithm_success_rate:
                    best_model_free_algorithm_success_rate = success_rate
                    best_model_free_algorithm = "FIXED_POMCP_TRAJECTORY"

            if exists(x+"/fixed_lppo_trajectory_simulation_stats.json") and exists(x+"/fixed_lppo_trajectory_simulation.json"):
                line, success_rate = write_metrics(results_file, "FIXED_LPPO_TRAJECTORY", branches, clustering, world_number, entropy_bucket,  x + "/fixed_lppo_trajectory_simulation_stats.json", metrics, world_stats, world_metrics)
                lines.append(line)
                if success_rate > best_model_free_algorithm_success_rate:
                    best_model_free_algorithm_success_rate = success_rate
                    best_model_free_algorithm = "FIXED_LPPO_TRAJECTORY"

            if exists(x+"/thigmotaxis_simulation_stats.json") and exists(x+"/thigmotaxis_simulation.json"):
                line, success_rate = write_metrics(results_file, "THIGMOTAXIS", branches, clustering, world_number, entropy_bucket, x + "/thigmotaxis_simulation_stats.json", metrics, world_stats, world_metrics)
                lines.append(line)
                if success_rate > best_model_free_algorithm_success_rate:
                    best_model_free_algorithm_success_rate = success_rate
                    best_model_free_algorithm = "THIGMOTAXIS"

            if exists(x+"/reactive_thigmotaxis_simulation_stats.json") and exists(x+"/reactive_thigmotaxis_simulation.json"):
                line, success_rate = write_metrics(results_file, "REACTIVE_THIGMOTAXIS", branches, clustering, world_number, entropy_bucket, x + "/reactive_thigmotaxis_simulation_stats.json", metrics, world_stats, world_metrics)
                lines.append(line)
                if success_rate > best_model_free_algorithm_success_rate:
                    best_model_free_algorithm_success_rate = success_rate
                    best_model_free_algorithm = "REACTIVE_THIGMOTAXIS"

            if exists(x + "/planning_simulation_stats.json") and exists(x + "/planning_simulation.json"):
                line, success_rate = write_metrics(results_file, "POMCP", branches, clustering, world_number, entropy_bucket, x + "/planning_simulation_stats.json", metrics, world_stats, world_metrics)
                lines.append(line)
                pomcp_success_rate = success_rate

            if exists(x + "/lppo_planning_simulation_stats.json") and exists(x + "/lppo_planning_simulation.json"):
                line, success_rate = write_metrics(results_file, "LPPO", branches, clustering, world_number, entropy_bucket,  x + "/lppo_planning_simulation_stats.json", metrics, world_stats, world_metrics)
                lines.append(line)
                lppo_success_rate = success_rate

            completed_lines = [line + ",%s,%f,%f,%f\n" % (best_model_free_algorithm, best_model_free_algorithm_success_rate, pomcp_success_rate - best_model_free_algorithm_success_rate, lppo_success_rate - best_model_free_algorithm_success_rate) for line in lines]
            results_file.writelines(completed_lines)
