from cellworld import *
from src import *


# survival_rate = []
#
# for i in range(1, 101):
#     print(i)
#     f = "../results/sim2/results_%d.json" % i
#     s = Simulation()
#     s = s.load_from_file(f)
#     p = sum(s.episodes.process(lambda e: 1 if e[-1].prey_state.cell_id == 330 else 0))
#     survival_rate.append(p)

import matplotlib.pyplot as plt

plt.plot(JsonList.create_type(float)().load_from_file("../results/03_21_05_survival_rate.json"))
plt.plot(JsonList.create_type(float)().load_from_file("../results/15_00_00_survival_rate.json"))

plt.savefig("21_05_1000.png")
plt.show()
