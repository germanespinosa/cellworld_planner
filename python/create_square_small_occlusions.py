from cellworld import *
import os


def convert_coordinates(p):
    return Coordinates(int(p.split(",")[1])-7, 7-int(p.split(",")[0]))


def load_occlusions(cell_map : Cell_map, file_name) -> Cell_group_builder:
    occlusions = Cell_group_builder()
    with open(file_name) as f:
        lines = f.readlines()
    for line in lines[1:-1]:
        c = convert_coordinates(line)
#        c = Coordinates(int(line.split(",")[0])-7, int(line.split(",")[1])-7)
        occlusions.append(cell_map[c])
    spawn_coordinates_lines = lines[-1].replace("\n","").replace("\"","").replace("),","_").replace(")","_").replace("(","").split("_")
    spawn_coordinates = [convert_coordinates(line) for line in spawn_coordinates_lines if line !=""]
    spawn_cell_ids = [cell_map[coordinates] for coordinates in spawn_coordinates]
    return occlusions, spawn_cell_ids


def find_disconnected_cells( w: World):
    cells = w.cells
    new_occlusions = Cell_group_builder()
    g = Graph.create_connection_graph(w)
    b = cells[0]
    for cell in cells:
        if not cell.occluded and not g.is_connected(b, cell):
            new_occlusions.append(cell.id)
    return new_occlusions

w = World.get_from_parameters_names("square_small", "canonical")
m = Cell_map(w.configuration.cell_coordinates)

data_folder = "c:/Research/cellworld_data/cell_group/"


def rr(e, n):
    p = data_folder + "square_small.%02d_%02d" % (n, e)
    occlusions_builder, spawn_cell_ids = load_occlusions(m, "c:/Research/square_small_occlusions/Entropy_" + str(e) + "/OcclusionCoordinates_Simulation" + str(n) + ".csv")
    w.set_occlusions(occlusions_builder)
    new_occlusions = find_disconnected_cells(w)
    plt.show()
    final_occlusions = (occlusions_builder + new_occlusions)
    final_occlusions.save(p + ".occlusions")
    final_spawn_cell_ids = Cell_group_builder()
    for id in spawn_cell_ids:
        if id == -1:
            continue
        if id not in final_occlusions:
            final_spawn_cell_ids.append(id)
    if len(final_spawn_cell_ids) == 0:
        print ("failed")
    final_spawn_cell_ids.save(p + ".spawn_locations")






for n in range(20):
    for e in range(10):
        rr(e,n)