#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <string>
#include <iostream>
#include <filesystem>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;
using namespace std::filesystem;

int main(int argc, char **argv) {
    Experiment experiment;
    experiment.world_configuration_name = "hexagonal";
    experiment.world_implementation_name = "mice";
    experiment.occlusions = "20_05";
    experiment.name = "simulation";
    for (const auto & entry : directory_iterator("."))
        std::cout << entry.path() << std::endl;
}