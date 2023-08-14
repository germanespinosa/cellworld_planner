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

speed = "slow"
data = {}
for ent in [0, 2, 5, 7, 9]:
    data[ent] = []
    for wn in range(6):
        file_name = "../results/sim1/%s_%02i_%02i_survival_rate.json" % (speed, wn, ent)
        print(file_name)
        values = JsonList.create_type(float)().load_from_file(file_name)
        print(values)
        data[ent].append(values)


for ent in [0, 2, 5, 7, 9]:
    average = [0.0 for x in range(50)]
    for survival_rate in data[ent]:
        print(survival_rate)
        average = [x + y /len(data[ent]) for x, y in zip(average, survival_rate)]
    plt.plot(average, label=str(ent))
plt.legend()
plt.savefig("survival_rate.png")
plt.show()
