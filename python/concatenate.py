from cellworld import *
from src import *
from sys import argv
from os import path, system
from glob import glob

if len(argv) < 2:
    print("missing folder parameter")
    exit(1)

if len(argv) < 3:
    print("missing destination parameter")
    exit(1)

folder = argv[1]

if not (path.exists(folder) and path.isdir(folder)):
    print("folder doesn't exists")
    exit(1)

dst = argv[2]

e = Simulation()
for f in glob(folder + "/*"):
    try:
        ne = Simulation.load_from_file(f)
    except:
        print("Ignoring ", f)
        continue
    print("processing ", f)
    e.world_info = ne.world_info
    e.parameters = ne.parameters
    e.episodes = e.episodes + ne.episodes

e.save(dst)
system("../cmake-build-debug/create_statistics -")