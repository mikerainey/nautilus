#include "incr_array.hpp"

#include "tpalrts.hpp"

/*---------------------------------------------------------------------*/

namespace tpalrts {

std::size_t nb_workers = 3;
uint64_t nb_items = 600000000;
int64_t* a;

auto bench_pre() -> void {
  a = (int64_t*)malloc(sizeof(int64_t)*nb_items);
  for (int64_t i = 0; i < nb_items; i++) {
    a[i] = 0;
  }
}

auto bench_body_interrupt(promotable* p) -> void {
  rollforward_table = {
    mk_rollforward_entry(incr_array_interrupt_l0, incr_array_interrupt_rf_l0),
    mk_rollforward_entry(incr_array_interrupt_l1, incr_array_interrupt_rf_l1),
    mk_rollforward_entry(incr_array_interrupt_l2, incr_array_interrupt_rf_l2),
    mk_rollforward_entry(incr_array_interrupt_l3, incr_array_interrupt_rf_l3),
  };
  incr_array_interrupt(a, 0, nb_items, p);
}

auto bench_body_software_polling(promotable* p) -> void {
  incr_array_software_polling(a, 0, nb_items, p);
}

auto bench_body_serial(promotable* p) -> void {
  incr_array_serial(a, 0, nb_items);
}

auto bench_post() -> void {
  int64_t m = 0;
  for (int64_t i = 0; i < nb_items; i++) {
    m += a[i];
  }
  assert(m == nb_items);
  free(a);
}

void bench_incr_array_interrupt() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_incr_array_software_polling() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_incr_array_serial() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_incr_array_manual() {
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new incr_array_manual<microbench_scheduler_type>(a, 0, nb_items);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

}

extern "C" {
  void handle_incr_array_interrupt(char *buf, void *priv) {
    tpalrts::bench_incr_array_interrupt();
  }
  void handle_incr_array_software_polling(char *buf, void *priv) {
    tpalrts::bench_incr_array_software_polling();
  }
  void handle_incr_array_manual(char *buf, void *priv) {
    tpalrts::bench_incr_array_manual();
  }
  void handle_incr_array_serial(char *buf, void *priv) {
    tpalrts::bench_incr_array_serial();
  }
}
