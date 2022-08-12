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

metric = "success_rate"

counters = {}
for e in range(10):
    for n in range(20):
        world = "%02d_%02d" % (n, e)
        counters[world] = 0

print("folder", "entropy_bucket", "planning_simulation", "fixed_trajectory_simulation", "lppo_planning_simulation", "fixed_lppo_trajectory_simulation", "thigmotaxis_simulation", "reactive_thigmotaxis_simulation", "shortest_path_simulation")
for x in glob.glob("*"):
    if "zip" not in x:
        if len(x.split(".")) == 1:
            continue
        world = x.split(".")[1]
        counters[world] = 1

missing = 0
for world in counters.keys():
    if counters[world] == 0:
        print (world, " results are missing.")
        missing+=1

print("total missing:", missing)