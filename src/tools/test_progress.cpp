#include <thread_pool.h>
#include <gauges/include/gauges.h>
#include <cell_world.h>

//using namespace indicators;
using namespace std;
using namespace gauges;

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
    while(!completed){
        completed = true;
        auto &gg = pick_random(progress.gauges);
        if (!gg.is_complete()) gg.tick();
        for (auto &g : progress.gauges) {
            if (!g.is_complete()) {
                completed = false;
                break;
            }
        }
        cout << progress;
        this_thread::sleep_for(100ms);
    }
    //progress.auto_refresh_stop();
    cout << "test end" << endl;
}
