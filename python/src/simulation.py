from json_cpp import JsonObject, JsonList
from cellworld import Experiment, Cell_group_builder, World_info


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
    def __init__(self, cell_id: int = 0, options: Cell_group_builder = None, options_values: Values = None, belief_state: Belief_state_representation = None, capture: bool = False):
        self.cell_id = cell_id
        if options:
            self.options = options
        else:
            self.options = Cell_group_builder()
        if options_values:
            self.options_values = options_values
        else:
            self.options_values = Values()
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
    def __init__(self, predator_state: Predator_state = None, prey_state: Prey_state = None):
        if predator_state:
            self.predator_state = predator_state
        else:
            self.predator_state = Predator_state()

        if prey_state:
            self.prey_state = prey_state
        else:
            self.prey_state = Prey_state()


class Simulation_episode(JsonList):
    def __init__(self):
        JsonList.__init__(self, list_type=Simulation_step)


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
