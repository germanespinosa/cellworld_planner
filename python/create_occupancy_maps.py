from cellworld import *
from src import *
import matplotlib.pyplot as plt



def create_figs(result_folder, speed, world, max_budget=0, budgets=[]):
    w = World.get_from_parameters_names("hexagonal", "canonical", world)
    if max_budget:
        budgets = list(range(1, max_budget + 1))

    for budget in budgets:
        print("World:", world, "Budget: ", budget)
        heatmap_file = "%s/%s_%s_%i_occupancy.json" % (result_folder, speed, world, budget)
        fig_file = "%s/%s_%s_%i_occupancy.pdf" % (result_folder, speed, world, budget)
        hm = JsonList.create_type(int)().load_from_file(heatmap_file)
        d = Display(w)
        m = max(hm)
        d.heatmap([v ** (1/3) for v in hm], color_map=matplotlib.pyplot.cm.GnBu)
        f_e = fair_entropy(331)
        print(fig_file, probability_entropy([o/sum(hm) for o in hm if o > 0])/f_e, f_e)
        plt.savefig(fig_file)
        plt.show()
        plt.close()


result_folder = "../results/sim4"
speed = "slow"
max_budget = 500
budgets = [100]
create_figs(result_folder, speed, "21_05", budgets=budgets)
create_figs(result_folder, speed, "00_00", budgets=budgets)
