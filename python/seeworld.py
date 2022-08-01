from console_input import console_input

a = ""
while a != "quit":
    a = console_input("Command here please!")
    print(" ", a)

# from cellworld import *
# for w in ["40","41"]:
#     for c in ["01","02","03","04","05","06","07","08","09"]:
#         for d in ["L", "H"]:
#             occlusions= w + "_" + c + "_" + d
#             print (occlusions)
#             world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
#             display = Display(world)
#             plt.show()