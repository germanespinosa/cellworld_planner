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

print ("folder", "entropy_bucket", "planning_simulation", "fixed_trajectory_simulation", "lppo_planning_simulation", "fixed_lppo_trajectory_simulation", "thigmotaxis_simulation", "reactive_thigmotaxis_simulation", "shortest_path_simulation")

for x in glob.glob("*"):
    if "zip" not in x:
        if len(x.split("."))==1:
            continue
        world = x.split(".")[1]
        entropy_bucket = world.split("_")[1]
        planning_simulation = Simulation_statistics.load_from_file(x+"/planning_simulation_stats.json")
        ft = 0
        if exists(x+"/fixed_trajectory_simulation_stats.json"):
            fixed_trajectory_simulation = Simulation_statistics.load_from_file(x+"/fixed_trajectory_simulation_stats.json")
        else:
            ft = fixed_trajectory_simulation.success_rate
        lppo_planning_simulation = Simulation_statistics.load_from_file(x+"/lppo_planning_simulation_stats.json")

        lft = 0
        if exists(x+"/fixed_lppo_trajectory_simulation_stats.json"):
            fixed_lppo_trajectory_simulation = Simulation_statistics.load_from_file(x+"/fixed_lppo_trajectory_simulation_stats.json")
        else:
            lft = fixed_lppo_trajectory_simulation.success_rate

        thigmotaxis_simulation = Simulation_statistics.load_from_file(x+"/thigmotaxis_simulation_stats.json")
        reactive_thigmotaxis_simulation = Simulation_statistics.load_from_file(x+"/reactive_thigmotaxis_simulation_stats.json")
        shortest_path_simulation = Simulation_statistics.load_from_file(x+"/shortest_path_simulation_stats.json")
        print(x, end=" ")
        print(entropy_bucket, end=" ")
        print(planning_simulation.success_rate, end=" ")
        print(ft, end=" ")
        print(lppo_planning_simulation.success_rate, end=" ")
        print(lft, end=" ")
        print(thigmotaxis_simulation.success_rate,end=" ")
        print(reactive_thigmotaxis_simulation.success_rate,end=" ")
        print(shortest_path_simulation.success_rate)



