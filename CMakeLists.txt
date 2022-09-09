cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

project(cellworld_planner
        VERSION 2019.1.0
        DESCRIPTION "add basic planning capabilities to agents"
        LANGUAGES CXX)

find_package(Dependencies)

install_dependency(https://github.com/germanespinosa/thread_pool)
include_directories(${dependencies_folder}/thread_pool/include)

install_dependency(https://github.com/germanespinosa/cellworld Cellworld)
install_dependency(https://github.com/germanespinosa/params-cpp Params-cpp)
include_directories(${dependencies_folder}/params-cpp/include)

install_dependency(https://github.com/germanespinosa/gauges)
include_directories(${dependencies_folder}/gauges/include)

install_dependency(https://github.com/germanespinosa/perf-analysis PerfAnalysis)
include_directories(${dependencies_folder}/perf-analysis/include)

install_dependency(https://github.com/germanespinosa/robot_library Robot_lib)

string(APPEND CMAKE_CXX_FLAGS " -fno-strict-aliasing -pthread ")

include_directories(include)
include_directories(include/performance)

set( cellworld_planner_files
        src/planner.cpp
        src/belief_state.cpp
        src/tree_search.cpp
        src/static_data.cpp
        src/predator.cpp
        src/prey.cpp
        src/thig_prey.cpp
        src/lppo.cpp
        src/statistics.cpp
        src/world_creation.cpp
        src/belief_state_location.cpp
        src/tree_search_location.cpp)

add_library(cellworld_planner ${cellworld_planner_files})

target_link_libraries( cellworld_planner
        LINK_PUBLIC
        cellworld
        params-cpp
        perf-analysis)

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


add_executable(create_thigmotaxis src/tools/create_thigmotaxis.cpp)

target_link_libraries( create_thigmotaxis
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(simulation src/tools/simulation.cpp)

target_link_libraries( simulation
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(thig_simulation src/tools/thig_simulation.cpp)

target_link_libraries( thig_simulation
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)


add_executable(create_simulation src/tools/create_simulation.cpp)

target_link_libraries( create_simulation
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)


add_executable(create_statistics src/tools/create_statistics.cpp)

target_link_libraries( create_statistics
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(create_world_statistics src/tools/create_world_statistics.cpp)

target_link_libraries( create_world_statistics
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(show_stats src/tools/show_stats.cpp)

target_link_libraries( show_stats
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(test_map_entropy src/tools/test_map_entropy.cpp)

target_link_libraries( test_map_entropy
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(process_poster_data src/tools/process_poster_data.cpp)

target_link_libraries( process_poster_data
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(create_poster_worlds src/tools/create_poster_worlds.cpp)

target_link_libraries( create_poster_worlds
        LINK_PUBLIC
        cellworld
        params-cpp)

add_executable(random_world_sim src/tools/random_world_sim.cpp)

target_link_libraries( random_world_sim
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(test_progress src/tools/test_progress.cpp)
target_link_libraries( test_progress
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)


add_executable(create_occlusions src/tools/create_occlusions.cpp)
target_link_libraries( create_occlusions
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)

add_executable(show_data src/tools/show_data.cpp)

target_link_libraries( show_data
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp)


add_executable(continuous_space_simulation src/tools/continuous_space_simulation.cpp)

target_link_libraries( continuous_space_simulation
        LINK_PUBLIC
        cellworld_planner
        cellworld
        params-cpp
        controller
        robot_lib)

#tests

install_dependency(https://github.com/germanespinosa/catch CatchTests)

test_library(cellworld)
