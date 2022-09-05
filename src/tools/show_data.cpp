#include <chrono>
#include <thread>
#include <cell_world.h>
#include <cellworld_planner/prey.h>
#include <params_cpp.h>
#include <cellworld_planner/simulation.h>
#include <thread_pool.h>
#include <fstream>

using namespace params_cpp;
using namespace std;
using namespace cell_world;
using namespace cell_world::planner;
using namespace thread_pool;


void write_file(ofstream &o, string &sim_type, string &clustering, string &world_number, string &entropy_bucket, string &stats_file, World_statistics world_stats) {

}
//def write_metrics(file, sim_type, clustering, world_number, entropy_bucket, sim_file, stats_file, metrics, world_stats, world_metrics):

int main(int argc, char **argv) {
    Parser p(argc, argv);
    auto output_file = p.get(Key("-o","--output"),"results.csv");

}