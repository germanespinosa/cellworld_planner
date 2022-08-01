#include <thread_pool.h>
#include <gauges/include/gauges.h>
#include <cell_world.h>

//using namespace indicators;
using namespace std;
using namespace gauges;

int main() {
    Gauges progress(100);
    Gauge gauge;
    gauge.set_total_work(1000);
    thread_pool::Thread_pool tp;
    progress.auto_refresh_start(250);
    for (int i=0;i<100;i++) {
        auto title = "episode " + to_string(i) + ": ";
        auto &bar = progress.add_gauge(gauge);
        bar.set_title(title);
        tp.run ([&progress, gauge, &bar](int i) {
            while (!bar.is_complete()) {
                bar.tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(cell_world::Chance::dice(5)));
            }
            bar.complete();
        }, i);
    }
    tp.wait_all();
    progress.auto_refresh_stop();
    return 0;
}
