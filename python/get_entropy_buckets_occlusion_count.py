import sys
from cellworld import *
from scipy.stats import entropy

world_size = len(World_configuration.get_from_name(sys.argv[1]).cell_coordinates)
for entropy_bucket in range(10):
    occlusion_count = 0
    while entropy([occlusion_count / world_size, 1 - occlusion_count / world_size], base=2) < entropy_bucket / 10:
        occlusion_count += 1
    print(occlusion_count, end=" ")
