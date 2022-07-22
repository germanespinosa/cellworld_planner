from cellworld import *
from src.simulation import Simulation, Simulation_step, Simulation_episode
from glob import glob


def get_pause_proportion(simulation : Simulation, duration: int = 4, distance: int = 1):
    world = World.get_from_world_info(simulation.world_info)
#    world_stats = World_statistics.get_from_parameters_names(simulation.world_info.world_configuration, simulation.world_info.occlusions)
    stopped_steps = 0
    steps = 0
    for episode in simulation.episodes:
        steps += len(episode)
        stop_ends = -1
        for step_number, step in enumerate(episode):
            prey_cell = world.cells[step.prey_state.cell_id]
            if prey_cell.coordinates.x <= -18:
                continue
            if step_number < stop_ends:
                continue
            stop_ends = -1
            for step_counter, stop_step in enumerate(episode[step_number:]):
                stop_cell = world.cells[step.prey_state.cell_id]
                if stop_cell.coordinates.manhattan(prey_cell.coordinates) > 2 * distance:
                    break
                if step_counter >= duration:
                    stop_ends = step_number + step_counter
            if stop_ends != -1:
                stopped_steps += step_counter
    return stopped_steps/steps

for entropy_bucket_int in range(10):
    entropy_bucket = "%02d" % (entropy_bucket_int,)
    for folder in glob("../poster/plan/%s/*" % (entropy_bucket,)):
        for file in glob(folder + "/*"):
            if not "_stats" in file:
                simulation = Simulation.load_from_file(file)
                print(file, get_pause_proportion(simulation))

