import sys
from src import *

arguments = {}

arguments["depth"] = 50
arguments["capture_cost"] = 100
arguments["step_cost"] = 1
arguments["episode_reward"] = 0
arguments["gamma"] = .95

for i, parameter in enumerate(sys.argv[:-1]):
    if parameter[0] == "-":
        arguments[parameter[1:]] = sys.argv[i + 1]

if "simulations" not in arguments:
    print("Number of simulations per iteration must be specified")
    exit(1)

if "max_particle_count" not in arguments:
    arguments["max_particle_count"] = max(int(arguments["simulations"]) // 10, 10)

if "max_particle_creation_attempts" not in arguments:
    arguments["max_particle_creation_attempts"] = max(int(arguments["max_particle_count"]) * 10, 10)

if "pursue_speed" not in arguments:
    print("Predator pursue speed must be specified")
    exit(1)

if "exploration_speed" not in arguments:
    arguments["exploration_speed"] = float(arguments["pursue_speed"]) / 2

parameters = Simulation_parameters()
parameters.tree_search_parameters.belief_state_parameters.max_particle_count = int(arguments["max_particle_count"])
parameters.tree_search_parameters.belief_state_parameters.max_particle_creation_attempts = int(arguments["max_particle_creation_attempts"])
parameters.tree_search_parameters.simulations = int(arguments["simulations"])
parameters.tree_search_parameters.depth = int(arguments["depth"])
parameters.predator_parameters.pursue_speed = float(arguments["pursue_speed"])
parameters.predator_parameters.exploration_speed = float(arguments["exploration_speed"])
parameters.reward.capture_cost = float(arguments["capture_cost"])
parameters.reward.step_cost = float(arguments["step_cost"])
parameters.reward.episode_reward = float(arguments["episode_reward"])
parameters.reward.gamma = float(arguments["gamma"])
parameters.save(arguments["o"])

#
# parameters.tree_search_parameters
#
# {
#     "tree_search_parameters": {
#         "belief_state_parameters": {
#             "max_particle_count": 10,
#             "max_particle_creation_attempts": 100
#         },
#         "simulations": 30,
#         "depth": 50
#     },
#     "predator_parameters": {
#         "pursue_speed": 0.35,
#         "exploration_speed": 0.17
#     },
#     "reward": {
#         "capture_cost": 100,
#         "step_cost": 1,
#         "episode_reward": 0,
#         "gamma": 0.95
#     }
# }