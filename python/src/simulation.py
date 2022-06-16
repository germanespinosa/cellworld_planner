import math

from json_cpp import JsonObject, JsonList
from cellworld import Experiment, Episode, Episode_list, Step, Cell_group_builder, World_info, World
import math


def entropy(labels, base=None):
    n_labels = len(labels)
    if n_labels == 0:
        return 0
    total = sum(labels)
    if total == 0:
        return  0
    probs = [x / total for x in labels]

    ent = 0.

    # Compute entropy
    if base is None:
        base = math.e

    for i in probs:
        if i > 0:
            ent -= i * math.log(i, base)
    return ent / math.log(len(probs), base)


class Reward(JsonObject):
    def __init__(self, step_cost: float = 0.0, gamma: float = 0.0, capture_cost: float = 0.0, episode_reward: float = 0.0, default_value: float = 0.0):
        self.step_cost = step_cost
        self.gamma = gamma
        self.capture_cost = capture_cost
        self.episode_reward = episode_reward
        self.default_value = default_value


class Belief_state_representation(JsonList):
    def __init__(self):
        JsonList.__init__(self, list_type=int)


class Values(JsonList):
    def __init__(self):
        JsonList.__init__(self, list_type=float)


class Predator_state(JsonObject):
    def __init__(self, cell_id: int = 0, destination_id: int = 0, behavior: int = 0):
        self.cell_id = cell_id
        self.destination_id = destination_id
        self.behavior = behavior


class Prey_state(JsonObject):
    def __init__(self, cell_id: int = 0, options: Cell_group_builder = None, options_values: Values = None, plan: Cell_group_builder = None, belief_state: Belief_state_representation = None, capture: bool = False):
        self.cell_id = cell_id
        if options:
            self.options = options
        else:
            self.options = Cell_group_builder()
        if options_values:
            self.options_values = options_values
        else:
            self.options_values = Values()
        if plan:
            self.plan = plan
        else:
            self.plan = Cell_group_builder()
        if belief_state:
            self.belief_state = belief_state
        else:
            self.belief_state = Belief_state_representation()
        self.capture = capture


class Belief_state_parameters(JsonObject):

    def __init__(self, max_particle_count: int = 0, max_particle_creation_attempts: int = 0):
        self.max_particle_count = max_particle_count
        self.max_particle_creation_attempts = max_particle_creation_attempts


class Tree_search_parameters(JsonObject):
    def __init__(self, belief_state_parameter: Belief_state_parameters = None, simulations: int = 0, depth: int = 0):
        if belief_state_parameter:
            self.belief_state_parameters = belief_state_parameter
        else:
            self.belief_state_parameters = Belief_state_parameters()
        self.simulations = simulations
        self.depth = depth


class Predator_parameters(JsonObject):
    def __init__(self, exploration_speed: float = 0.0, pursue_speed: float = 0.0):
        self.exploration_speed = exploration_speed
        self.pursue_speed = pursue_speed


class Simulation_parameters(JsonObject):
    def __init__(self, reward: Reward = None, tree_search_parameters: Tree_search_parameters = None, predator_parameters: Predator_parameters = None):
        if reward:
            self.reward = reward
        else:
            self.reward = Reward()

        if tree_search_parameters:
            self.tree_search_parameters = tree_search_parameters
        else:
            self.tree_search_parameters = Tree_search_parameters()

        if predator_parameters:
            self.predator_parameters = predator_parameters
        else:
            self.predator_parameters = Predator_parameters()


class Prey_state_history(JsonList):
    def __init__(self):
        JsonList.__init__(self, list_type=Prey_state)


class Predator_state_history(JsonList):
    def __init__(self):
        JsonList.__init__(self, list_type=Predator_state)


class Simulation_step(JsonObject):
    def __init__(self, predator_state: Predator_state = None, prey_state: Prey_state = None, data: str = ""):
        if predator_state:
            self.predator_state = predator_state
        else:
            self.predator_state = Predator_state()

        if prey_state:
            self.prey_state = prey_state
        else:
            self.prey_state = Prey_state()
        self.data = data

class Stats(JsonObject):
    def __init__(self):
        self.length = 0.0
        self.visited_cells = 0.0
        self.survival_rate = 0.0
        self.capture_rate = 0.0
        self.value = 0.0
        self.belief_state_entropy = 0.0
        self.pursue_rate = 0.0
        self.distance = 0.0

class Simulation_episode(JsonList):
    def __init__(self):
        JsonList.__init__(self, list_type=Simulation_step)

    def get_stats(self, world:World) -> Stats:
        stats = Stats()
        visited_cells = []
        distance_sum = 0
        entropy_sum = 0
        pursue_sum = 0
        for simulation_step in self:
            prey_cell = world.cells[simulation_step.prey_state.cell_id]
            predator_cell = world.cells[simulation_step.predator_state.cell_id]
            if simulation_step.prey_state.capture:
                stats.capture_rate +=1
            if prey_cell.id not in visited_cells:
                visited_cells.append(prey_cell.id)
            distance_sum += prey_cell.location.dist(predator_cell.location)
            entropy_sum += entropy(simulation_step.prey_state.belief_state)
            if simulation_step.predator_state.behavior == 1:
                pursue_sum += 1
        stats.length = len(self)
        stats.visited_cells = len(visited_cells)
        if stats.length > 0:
            stats.pursue_rate = pursue_sum / stats.length
            stats.distance = distance_sum / stats.length
            stats.belief_state_entropy = entropy_sum / stats.length
        else:
            stats.pursue_rate = 0
            stats.distance = 0
            stats.belief_state_entropy = 0

        if stats.capture_rate == 0:
            stats.survival_rate = 1.0
        else:
            stats.survival_rate = 0.0
        return stats


class Simulation (JsonObject):
    def __init__(self, world_info: World_info = None, parameters: Simulation_parameters = None, episodes: JsonList = None):
        if world_info:
            self.world_info = world_info
        else:
            self.world_info = World_info()

        if parameters:
            self.parameters = parameters
        else:
            self.parameters = Simulation_parameters()

        if episodes:
            self.episodes = episodes
        else:
            self.episodes = JsonList(list_type=Simulation_episode)

    def get_stats(self, reward: Reward) -> Stats:
        world = World.get_from_world_info(self.world_info)
        simulation_stats = Stats()
        simulation_stats.episode_stats = JsonList(list_type=Stats)
        episode_count = float(len(self.episodes))
        for episode in self.episodes:
            stats = episode.get_stats(world=world)
            stats.value = - stats.capture_rate * reward.capture_cost - stats.length * reward.step_cost
            simulation_stats.episode_stats.append(stats)
            simulation_stats.length += stats.length / episode_count
            simulation_stats.value += stats.value / episode_count
            simulation_stats.capture_rate += stats.capture_rate / episode_count
            simulation_stats.survival_rate += stats.survival_rate / episode_count
            simulation_stats.pursue_rate += stats.pursue_rate / episode_count
            simulation_stats.belief_state_entropy += stats.belief_state_entropy / episode_count
            simulation_stats.distance += stats.distance / episode_count
            simulation_stats.visited_cells += stats.visited_cells / episode_count
        return simulation_stats

    @staticmethod
    def from_experiment(experiment: Experiment, prey_speed: float = .116):
        simulation = Simulation()
        simulation.world_info.world_implementation = "canonical"
        simulation.world_info.world_configuration = experiment.world_configuration_name
        simulation.world_info.occlusions = experiment.occlusions
        world = World.get_from_world_info(simulation.world_info)
        prey = Prey_state()
        predator = Predator_state()
        for episode in experiment.episodes:
            sim_episode = Simulation_episode()
            time_step = 0
            prey.cell_id = -1
            predator.cell_id = -1
            first_prey_cell = -1
            first_predator_cell = -1
            first = True
            for step_number in range(len(episode.trajectories)):
                step = episode.trajectories[step_number]
                cell_id = world.cells.find(step.location)
                if step.agent_name == "prey":
                    prey.cell_id = cell_id
                else:
                    predator.cell_id = cell_id

                if predator.cell_id == -1 or prey.cell_id == -1:
                    continue

                if step.time_stamp >= time_step and \
                        (first or first_prey_cell != prey.cell_id or first_predator_cell != predator.cell_id):
                    sim_step = Simulation_step()
                    sim_step.prey_state = prey.copy()
                    sim_step.predator_state = predator.copy()
                    sim_episode.append(sim_step)
                    time_step = step.time_stamp + prey_speed

                if first:
                    first_prey_cell = prey.cell_id
                    first_predator_cell = predator.cell_id
                    first = False

            simulation.episodes.append(sim_episode)
        return simulation


class Comparison_data(JsonObject):
    def __init__(self):
        self.label = ""
        self.file_name = ""


class Comparison(JsonObject):
    def __init__(self):
        self.name = ""
        self.data_points = JsonList(list_type=Comparison_data)
