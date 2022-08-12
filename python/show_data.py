import glob
from cellworld import *
from src import *
from os.path import exists

planning = [0.0 for x in range(10)]
lppo_planning = [0.0 for x in range(10)]
fixed_trajectory = [0.0 for x in range(10)]
lppo_fixed_trajectory = [0.0 for x in range(10)]
shortest_path = [0.0 for x in range(10)]
thigmotaxis_trajectory = [0.0 for x in range(10)]
reactive_thigmotaxis_trajectory = [0.0 for x in range(10)]

metric = "capture_rate"

print("folder", "entropy_bucket", "planning_simulation", "fixed_trajectory_simulation", "lppo_planning_simulation", "fixed_lppo_trajectory_simulation", "thigmotaxis_simulation", "reactive_thigmotaxis_simulation", "shortest_path_simulation")
for x in glob.glob("*"):
    if "zip" not in x:
        if len(x.split(".")) == 1:
            continue
        world = x.split(".")[1]
        entropy_bucket = world.split("_")[1]
        planning_simulation_success_rate = "n/a"
        if exists(x+"/planning_simulation_stats.json"):
            planning_simulation = Simulation_statistics.load_from_file(x+"/planning_simulation_stats.json")
            planning_simulation_success_rate = getattr(planning_simulation, metric)

        fixed_trajectory_simulation_success_rate = "n/a"
        if exists(x+"/fixed_trajectory_simulation_stats.json"):
            fixed_trajectory_simulation = Simulation_statistics.load_from_file(x+"/fixed_trajectory_simulation_stats.json")
            fixed_trajectory_simulation_success_rate = getattr(fixed_trajectory_simulation, metric)

        lppo_planning_simulation_success_rate = "n/a"
        if exists(x+"/lppo_planning_simulation_stats.json"):
            lppo_planning_simulation = Simulation_statistics.load_from_file(x+"/lppo_planning_simulation_stats.json")
            lppo_planning_simulation_success_rate = getattr(lppo_planning_simulation, metric)

        fixed_lppo_trajectory_simulation_success_rate = "n/a"
        if exists(x+"/fixed_lppo_trajectory_simulation_stats.json"):
            fixed_lppo_trajectory_simulation = Simulation_statistics.load_from_file(x+"/fixed_lppo_trajectory_simulation_stats.json")
            fixed_lppo_trajectory_simulation_success_rate = getattr(fixed_lppo_trajectory_simulation, metric)

        thigmotaxis_simulation_success_rate = "n/a"
        if exists(x+"/thigmotaxis_simulation_stats.json"):
            thigmotaxis_simulation = Simulation_statistics.load_from_file(x+"/thigmotaxis_simulation_stats.json")
            thigmotaxis_simulation_success_rate = getattr(thigmotaxis_simulation, metric)

        reactive_thigmotaxis_simulation_success_rate = "n/a"
        if exists(x+"/reactive_thigmotaxis_simulation_stats.json"):
            reactive_thigmotaxis_simulation = Simulation_statistics.load_from_file(x+"/reactive_thigmotaxis_simulation_stats.json")
            reactive_thigmotaxis_simulation_success_rate = getattr(reactive_thigmotaxis_simulation, metric)

        shortest_path_simulation_success_rate = "n/a"
        if exists(x+"/shortest_path_simulation_stats.json"):
            shortest_path_simulation = Simulation_statistics.load_from_file(x+"/shortest_path_simulation_stats.json")
            reactive_thigmotaxis_simulation_success_rate = getattr(shortest_path_simulation, metric)

        print(x, entropy_bucket, planning_simulation_success_rate, fixed_trajectory_simulation_success_rate, lppo_planning_simulation_success_rate, fixed_lppo_trajectory_simulation_success_rate, thigmotaxis_simulation_success_rate, reactive_thigmotaxis_simulation_success_rate, shortest_path_simulation_success_rate)



