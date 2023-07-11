from cellworld import *
import math
import networkx
wc = World_configuration()
wc.cell_shape.sides = 4
wc.connection_pattern.append(Coordinates(1, 0))
wc.connection_pattern.append(Coordinates(0, 1))
wc.connection_pattern.append(Coordinates(-1, 0))
wc.connection_pattern.append(Coordinates(0, -1))
wi = World_implementation()
wi.cell_transformation.size = 2 ** .5
wi.cell_transformation.rotation = 0
wi.space.center = Location(2, 2)
wi.space.shape.sides = 4
wi.space.transformation.size = 4 * wi.cell_transformation.size
wi.space.transformation.rotation = 45
for y in range(3, -1, -1):
  for x in range(4):
    wc.cell_coordinates.append(Coordinates(x, y))
    wi.cell_locations.append(Location(x + .5, y + .5))
world = World.get_from_parameters(wc, wi)

occlusions_coordinates = Coordinates_list.parse('[{"x":1,"y":0},{"x":2,"y":0},{"x":0,"y":1},{"x":0,"y":2},{"x":1,"y":2},{"x":3,"y":2},{"x":0,"y":3},{"x":1,"y":3}]')
occlusions = world.cells.filter(lambda i: i.coordinates in occlusions_coordinates).get("id")
world.set_occlusions(occlusions)
graph = Graph.create_visibility_graph(world)
G = graph.to_nxgraph()
pos = []

for i,c in enumerate(world.cells):
  center=Location(0, 0)
  rotation = 2 * math.pi / len(world.cells) * i
  node_pos = center.move(rotation, 1)
  pos.append([node_pos.x, node_pos.y])
node_color=nx.get_node_attributes(G, 'color').values()
# nx.draw(G, pos=pos, with_labels=True, node_color=node_color, edge_color="black")


def is_between(self,
                src: Location = None,
                dst: Location = None,
                theta: float = None,
                dist: float = None):
    if dst is not None:
        theta = src.atan(dst)
        dist = src.dist(dst)
    else:
        if theta is None or dist is None:
            raise "either dst or theta and dist should be used"
    dist_center = src.dist(self.center)
    theta_center = src.atan(self.center)
    diff_theta_center, direction_center = angle_difference(theta, theta_center)
    if dist < dist_center - self.radius:
        return False
    for v in self.vertices:
        vertex_distance = src.dist(v)
        if vertex_distance <= dist:
            theta_vertex = src.atan(v)
            diff_theta_vertex, direction_vertex = angle_difference(theta, theta_vertex)
            if direction_center == -direction_vertex:
                if diff_theta_center + diff_theta_vertex < math.pi:
                    return True
    return False

def is_visible(self, src: Location, dst: Location) -> bool:
    theta = src.atan(dst)
    dist = src.dist(dst)
    for occlusion in self.occlusions:
        if is_between(occlusion, src, theta=theta, dist=dist):
            return False
    return True

lv = Location_visibility.from_world(world)
src = world.cells[3].location
dst = world.cells[12].location
theta = src.atan(dst)
dist = src.dist(dst)
# print(is_visible(lv, src, dst))
# print(src, dst)
# print(lv.occlusions[0].vertices)
# print(is_between(lv.occlusions[-2], src, theta=theta, dist=dist))

vertices = Polygon( dst,
                world.configuration.cell_shape.sides,
                world.implementation.cell_transformation.size / 2,
                world.implementation.space.transformation.rotation + world.implementation.cell_transformation.rotation
                ).vertices
print( dst,
       world.implementation.space.transformation.rotation + world.implementation.cell_transformation.rotation )

for i, v in enumerate(vertices):
    print(i, v)

vertices = lv.occlusions[0].vertices
print ("occlusion:", lv.occlusions[0].center)
for i, v in enumerate(vertices):
    print(i, v)
