cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

project(cellworld_planner
        VERSION 2019.1.0
        DESCRIPTION "add basic planning capabilities to agents"
        LANGUAGES CXX)

find_package(Dependencies)


install_dependency(https://github.com/germanespinosa/cellworld Cellworld)
install_dependency(https://github.com/germanespinosa/params-cpp Params-cpp)
include_directories(dependencies/params-cpp/include)

string(APPEND CMAKE_CXX_FLAGS " -fno-strict-aliasing -pthread ")

include_directories(include)


set( cellworld_planner_files
        src/planner.cpp
        src/belief_state.cpp
        src/tree_search.cpp
        src/static_data.cpp
        src/predator.cpp
        src/prey.cpp
        )

add_library(cellworld_planner ${cellworld_planner_files})

target_link_libraries( cellworld_planner
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(create_lppos src/tools/create_lppos.cpp)

target_link_libraries( create_lppos
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(create_options src/tools/create_options.cpp)

target_link_libraries( create_options
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(create_paths src/tools/create_paths.cpp)

target_link_libraries( create_paths
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(create_visibility src/tools/create_visibility.cpp)

target_link_libraries( create_visibility
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(create_predator_destinations src/tools/create_predator_destinations.cpp)

target_link_libraries( create_predator_destinations
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(create_spawn_locations src/tools/create_spawn_locations.cpp)

target_link_libraries( create_spawn_locations
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(create_experiment src/tools/create_experiment.cpp)

target_link_libraries( create_experiment
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(simulation src/tools/simulation.cpp)

target_link_libraries( simulation
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)


#tests

install_dependency(https://github.com/germanespinosa/catch CatchTests)

test_library(cellworld)
