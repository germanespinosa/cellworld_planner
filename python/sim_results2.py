from cellworld import *
from src import *
import matplotlib.pyplot as plt


plt.show()

result_folder = "../results/sim4"
speed = "slow"

worlds = ["00_00", "21_05"]
reps = 100
max_budget = 500

budgets = range(1, max_budget + 1)

budgets = [100]

clusters = StreamLineClusters(1, .15, 100)

for world in worlds:
    w = World.get_from_parameters_names("hexagonal", "canonical", world)
    for budget in budgets:
        print("Budget: ", budget)
        result_file = "%s/%s_%s_%i.json" % (result_folder, speed, world, budget)
        heatmap_file = "%s/%s_%s_%i_occupancy.json" % (result_folder, speed, world, budget)
        results = Simulation()
        results = results.load_from_file(result_file)
        hm = JsonList.create_type(int)([0 for c in w.cells])
        for episode in results.episodes:
            if episode[-1].prey_state.cell_id != 330:
                continue
            visited_cells = []
            episode.process(lambda s: w.cells[s.cell_id])
            sl = StreamLine
            for step in episode:
                if step.prey_state.cell_id in visited_cells:
                    continue
                visited_cells.append(step.prey_state.cell_id)
                hm[step.prey_state.cell_id] += 1
        hm.save(heatmap_file)
