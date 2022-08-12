from cellworld import *
from random import randint, choice


occlusions_entropy = [0, 4, 8, 18, 27, 37, 49, 63, 81, 105]

data_folder = "c:/Research/cellworld_data/cell_group/"

configuration = "hexagonal"

world = World.get_from_parameters_names(configuration, "canonical")
cell_map = Cell_map(world.configuration.cell_coordinates)


def free_connections(coordinates):
    return [coordinates + cp for cp in world.configuration.connection_pattern if cell_map[coordinates + cp] != -1 and world.cells[cell_map[coordinates+cp]].occluded == False]


def clustered_world(e, n, walk=True):
    p = data_folder + "%s.%02d_%02d.occlusions" % (configuration, n, e)
    k = occlusions_entropy[e]
    r = k
    world.set_occlusions(Cell_group_builder())
    occlusions = Cell_group_builder()
    while r > 0:
        current_location = choice(world.configuration.cell_coordinates)
        if walk:
            l = randint(1, r)
        else:
            l = 1
        for i in range(l):
            current_cell_id = cell_map[current_location]
            if world.cells[current_cell_id].occluded:
                break
            connections = free_connections(current_location)
            if len(connections) == 0:
                break
            world.cells[current_cell_id].occluded = True
            occlusions.append(current_cell_id)
            r-=1
            current_location = choice(connections)
    occlusions.save(p)


for n in range(50, 60):
    for e in range(10):
        clustered_world(e, n)

for n in range(60, 70):
    for e in range(10):
        clustered_world(e, n, walk=False)

