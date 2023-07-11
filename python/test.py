import os
from threading import Thread
from cellworld import *

configuration = "hexagonal"

entropies = 10
worlds = 20
file_path = configuration +"_visibility_graphs.json"

if os.path.exists(file_path):
  graph_builders_by_entropy = JsonList.create_type(JsonList.create_type(list_item_type=Graph_builder))().load_from_file(file_path)
else:
  graph_by_entropy = [list() for e in range(entropies)]
  graph_builders_by_entropy = JsonList([JsonList(list_type=Graph_builder) for e in range(entropies)])
  def process_entropy(e):
    global worlds_by_entropy
    global graph_by_entropy
    world = World.get_from_parameters_names(configuration, "canonical", "00_00")
    for n in range(worlds):
      occlusions_set = "%02i_%02i" % (n, e)
      occlusions = Cell_group_builder.get_from_name(configuration, occlusions_set + ".occlusions")
      world.set_occlusions(occlusions=occlusions)
      graph = Graph.create_visibility_graph(world)
      graph_builders_by_entropy[e].append(graph.get_builder())
      graph_by_entropy[e].append(graph)

  threads = []
  for e in range(entropies):
    print(e, end=" ")
    threads.append(Thread(target=process_entropy, args=(e,)))
    threads[-1].start()
  for t in threads:
    t.join()

  graph_builders_by_entropy.save(file_path)

import matplotlib.pyplot as plt

nc = []
graph_by_entropy = []

for gbwn in graph_builders_by_entropy:
  graph_by_entropy.append([])
  for gbw in gbwn:
    graph_by_entropy[-1].append(Graph(gbw))


def probability_entropy(prob):
  if sum(prob) > 1.01 or sum(prob) < .99:
    raise RuntimeError("Sum of probabilities should be 1")
  from math import log2
  ent = 0
  for p in prob:
    if p == 0:
      continue
    print (p)
    ent += p * log2(p)
  return -ent

def complexity(self, normalized: bool = False):
    prob = []
    bad_nodes = 0
    for c in self._connections:
      if len(c)==0:
        bad_nodes+=1
    total_nodes = len(self._connections) - bad_nodes
    connected_nodes = 0
    disconnected_nodes = 0
    for c in self._connections:
        connections = len(c)
        if connections == 0:
          continue
        connected_nodes += connections
        disconnected_nodes += total_nodes - connections
    total = connected_nodes + disconnected_nodes
    prob = [connected_nodes/total, disconnected_nodes/total]
    print(prob, sum(prob))
    return probability_entropy(prob)

for e in range(entropies):
  nc.append([])
  for n in range(worlds):
    graph = graph_by_entropy[e][n]
    wnc = graph.complexity()
    nc[-1].append(wnc/5)


for n in range(worlds):
  for e in range(entropies):
    print(nc[e][n], end=",")
  print()


from numpy import std
nca = [sum(nce) / len(nce) for nce in nc]
nsd = [std(nce) for nce in nc]
emax = [v + sd for v,sd in zip(nca,nsd)]
emin = [v - sd for v,sd in zip(nca,nsd)]
plt.fill_between(range(entropies), emin, emax, alpha=.4, color="red")
plt.plot(range(entropies), nca, color="blue")
plt.xticks(range(entropies))
plt.show()


