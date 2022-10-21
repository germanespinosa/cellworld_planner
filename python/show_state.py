from time import sleep
from cellworld import World, Display, Location, Agent_markers, Step, Timer, Cell_map, Coordinates
from cellworld_tracking import TrackingClient


class AgentData:
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name
        self.move_state = None
        self.move_done = False


def on_step(step: Step):
    if step.agent_name == "predator":
        predator.step = step
    else:
        prey.step = step


# GLOBALS
current_predator_destination = None


# SETUP
# worldprey
occlusions = "21_05"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
map = Cell_map(world.configuration.cell_coordinates)
# agent
predator = AgentData("predator")
prey = AgentData("prey")
display.set_agent_marker("predator", Agent_markers.arrow())

tracking = TrackingClient()
if not tracking.connect("127.0.0.1"):
    print("failed to connect to the tracking")
    exit(1)
tracking.set_throughput(5.0)
tracking.subscribe()
tracking.on_step = on_step

running = True
while running:
    display.agent(step=predator.step, color="blue", size=15)
    display.agent(step=prey.step, color="green", size=15)
    display.update()
    sleep(0.2)


