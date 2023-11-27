from cellworld import Graph, World
from simulation import Simulation, Simulation_step
from json_cpp import JsonObject, JsonList


class Detection(JsonObject):
    def __init__(self, episode_number: int, step_start: int, step_end: int):
        self.episode_number = episode_number
        self.step_start = step_start
        self.step_end = step_end
        JsonObject.__init__(self)


class Detections(JsonList):
    def __init__(self):
        JsonList.__init__(self, list_type=Detection)


class WaitDetector:

    '''
    Condition 1: No safe path to the goal
    Sparse belief state in the direction of the goal.
    At least 1/5 of the free cells closer to the goal than the prey

    Condition 2: Prey remains in a small section of the arena.(3 cells radius)

    Condition 3: Conditions 1 & 2 must be met for at least 10 steps
    '''

    @staticmethod
    def detect(simulation: Simulation, world: World):
        detections = Detections()
        goal = world.cells[-1]
        area_threshold = world.implementation.cell_transformation.size
        for episode_number, episode in enumerate(simulation.episodes):
            condition1 = 0
            condition2 = 0
            past_cell_locations = []
            detection = None
            for step_number, step in enumerate(episode):
                step = Simulation_step()

                # Condition 1
                prey_cell = world.cells[step.prey_state.cell_id]
                prey_goal_dist = goal.location.dist(prey_cell.location)
                closer_cells = []
                for cell in world.cells:
                    if cell.dist(goal) < prey_goal_dist:
                        closer_cells.append(cell.id)

                sparse_threshold = len(closer_cells) / 5
                closer_bs = 0
                for id in closer_cells:
                    if step.prey_state.belief_state[id] > 0:
                        closer_bs += 1

                if closer_bs > sparse_threshold:
                    condition1 += 1
                else:
                    condition1 = 0

                # Condition 2
                if len(past_cell_locations) == 10:
                    past_cell_locations = past_cell_locations[1:]

                past_cell_locations.append(prey_cell.location)

                max_distance = max([max([l1.dist(l2) for l1 in past_cell_locations]) for l2 in past_cell_locations])

                if max_distance <= area_threshold:
                    condition2 += 1
                else:
                    condition2 = 0

                # Condition 3
                if condition1 >= 10 and condition2 >= 10:
                    detection = Detection(episode_number=episode_number,
                                          step_start=step_number - min(condition1, condition2),
                                          step_end=step_number)
                else:
                    if detection:
                        detections.append(detection)
                        detection = None
        return detections


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    simulation = Simulation.load_from_file("../../../results/planning_sim/1000/planning_10000_batch_00000.json")
    world = World.get_from_world_info("hexagonal", "canonical", simulation.world_info.occlusions)
    detections = WaitDetector.detect(simulation, world)
    print (detections)
