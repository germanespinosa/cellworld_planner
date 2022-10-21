#include <thread_pool.h>
#include <gauges.h>
#include <cell_world.h>
#include <performance.h>

//using namespace indicators;
using namespace std;
using namespace gauges;
using namespace performance;

int main() {
    cout << "test " << endl;
    Gauges progress(100);
    for (int i = 0; i < 10; i++) {
        auto &g = progress.new_gauge();
        g.set_title(to_string(i) + " :");
        g.set_total_work(10);
    }
    bool completed = false;
    //progress.auto_refresh_start(250);
    PERF_START("Test_counter 1");
    while(!completed) {
        completed = true;
        auto &gg = pick_random(progress.gauges);
        if (!gg.is_complete()) gg.tick();
        PERF_START("Test_counter 2");
        for (auto &g: progress.gauges) {
            if (!g.is_complete()) {
                completed = false;
                break;
            }
        }
        this_thread::sleep_for(30ms);
        cout << progress;
        PERF_STOP("Test_counter 2");
        PERF_START("Test_counter 3");
        this_thread::sleep_for(70ms);
        PERF_STOP("Test_counter 3");
    }
    PERF_STOP("Test_counter 1");
    //progress.auto_refresh_stop();
    cout << "test end" << endl;
}
