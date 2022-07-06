#!/usr/bin/env python
from cellworld import *
from src import *
from sys import argv
from os import path, system
from glob import glob

if len(argv) < 2:
    print("missing pattern parameter")
    exit(1)

if len(argv) < 3:
    print("missing destination parameter")
    exit(1)

parameter = argv[1]
dst = argv[2]
e = Simulation()
for f in glob(parameter):
    try:
        ne = Simulation.load_from_file(f)
    except:
        print("Ignoring ", f)
        continue
    print("adding " + f + " to " + dst)
    e.world_info = ne.world_info
    e.parameters = ne.parameters
    e.episodes = e.episodes + ne.episodes

e.save(dst)
system("$CELLWORLD_PLANNER_BIN/create_statistics -s " + dst)