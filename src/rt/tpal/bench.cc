#include <limits.h>
#include <functional>
#include <vector>
#include <cstring>

#include "incr_array.hpp"
//#include "plus_reduce_array.hpp"
//#include "spmv.hpp"
//#include "fib.hpp"
//#include "knapsack_input.hpp"

#include "tpalrts.hpp"

extern "C"
uint32_t nk_get_num_cpus (void);

void call_thunks(std::vector<std::function<void()>>& thunks) {
  for (auto& f : thunks) {
    f();
  }
}

void print_prog(const char* s) {
  aprintf("prog %s\n", s);
}

/*---------------------------------------------------------------------*/
/* Control the setting of kappa */

namespace tpalrts {

void set_kappa_20() {
  nautilus_assign_kappa(20);
}

void set_kappa_40() {
  nautilus_assign_kappa(40);
}

void set_kappa_100() {
  nautilus_assign_kappa(100);
}

void set_kappa_400() {
  nautilus_assign_kappa(400);
}

void set_kappa_40000() {
  nautilus_assign_kappa(40000);
}

} // end namespace  

extern "C" {
  void handle_set_kappa_20(char *buf, void *priv) {
    tpalrts::set_kappa_20();
  }
  void handle_set_kappa_40(char *buf, void *priv) {
    tpalrts::set_kappa_40();
  }
  void handle_set_kappa_100(char *buf, void *priv) {
    tpalrts::set_kappa_100();
  }
  void handle_set_kappa_400(char *buf, void *priv) {
    tpalrts::set_kappa_400();
  }
  void handle_set_kappa_40000(char *buf, void *priv) {
    tpalrts::set_kappa_40000();
  }
}

/*---------------------------------------------------------------------*/
/* Control the number of worker threads */

namespace tpalrts {

std::size_t nb_workers = 1;

void set_nb_workers_1() {
  nb_workers = 1;
}

void set_nb_workers_3() {
  nb_workers = 3;
}
  
void set_nb_workers_7() {
  nb_workers = 7;
}

void set_nb_workers_15() {
  nb_workers = 15;
}

void set_nb_workers() {
  nb_workers = nk_get_num_cpus() - 1;    
}

} // end namespace

extern "C" {
  void handle_set_nb_workers_1(char *buf, void *priv) {
    tpalrts::set_nb_workers_1();
  }
  void handle_set_nb_workers_3(char *buf, void *priv) {
    tpalrts::set_nb_workers_3();
  }
  void handle_set_nb_workers_7(char *buf, void *priv) {
    tpalrts::set_nb_workers_7();
  }
  void handle_set_nb_workers_15(char *buf, void *priv) {
    tpalrts::set_nb_workers_15();
  }
}

namespace tpalrts {

void run_all(std::vector<std::function<void()>>& thunks) {
  set_nb_workers_1();
  
  set_kappa_100();
  call_thunks(thunks);
  set_kappa_40();
  call_thunks(thunks);
  set_kappa_20();
  call_thunks(thunks);

  set_nb_workers();
  
  set_kappa_100();
  call_thunks(thunks);
  set_kappa_40();
  call_thunks(thunks);
  set_kappa_20();
  call_thunks(thunks);
}
  
} // end namespace

/*---------------------------------------------------------------------*/
/* Incr array */

namespace tpalrts {  
namespace bench_incr_array {

uint64_t nb_items = 1000 * 1000 * 100;
int64_t* a;
  
auto bench_pre() -> void {
  printk("allocating %lu\n",nb_items);
  a = (int64_t*)malloc(sizeof(int64_t)*nb_items);
  if (a == nullptr) {
    printk("ERROR\n");
    return;
  }
  printk("initializing %lu\n",nb_items);
  for (int64_t i = 0; i < nb_items; i++) {
    a[i] = 0;
  }
  print_header = [=] {
    print_prog("incr_array");
    aprintf("n %lu\n", nb_items);
  };
}

auto bench_body_interrupt(promotable* p) -> void {
  rollforward_table = {
    #include "incr_array_rollforward_map.hpp"
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
  sched_configuration = sched_configuration_interrupt;
  //  set_nb_workers_3();
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_incr_array_interrupt_nopromote() {
  sched_configuration = sched_configuration_nopromote_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_incr_array_software_polling() {
  sched_configuration = sched_configuration_software_polling;
  printk("STARTING TPAL on %d workers\n",nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_incr_array_serial() {
  sched_configuration = sched_configuration_serial;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_incr_array_manual() {
  sched_configuration = sched_configuration_manual;
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new incr_array_manual<microbench_scheduler_type>(a, 0, nb_items);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

std::vector<std::function<void()>> incr_array_thunks = {  std::bind(bench_incr_array_interrupt),
                                                          std::bind(bench_incr_array_interrupt_nopromote),
                                                          std::bind(bench_incr_array_software_polling),
                                                          std::bind(bench_incr_array_serial),
                                                          std::bind(bench_incr_array_manual)
};

} // end namespace
} // end namespace

extern "C" {
  void handle_incr_array_interrupt(char *buf, void *priv) {
    tpalrts::bench_incr_array::bench_incr_array_interrupt();
  }
  void handle_incr_array_interrupt_nopromote(char *buf, void *priv) {
    tpalrts::bench_incr_array::bench_incr_array_interrupt_nopromote();
  }
  void handle_incr_array_software_polling(char *buf, void *priv) {
    tpalrts::bench_incr_array::bench_incr_array_software_polling();
  }
  void handle_incr_array_manual(char *buf, void *priv) {
    tpalrts::bench_incr_array::bench_incr_array_manual();
  }
  void handle_incr_array_serial(char *buf, void *priv) {
    tpalrts::bench_incr_array::bench_incr_array_serial();
  }
  void handle_incr_array(char *buf, void *priv) {
    tpalrts::run_all(tpalrts::bench_incr_array::incr_array_thunks);
  }
}

/*---------------------------------------------------------------------*/
/* Plus reduce array */

#if 0

namespace tpalrts {
namespace plus_reduce_array {

uint64_t nb_items = 1000 * 1000 * 1000;
int64_t* a;
int64_t result = 0;
  
auto bench_pre() -> void {
  a = (int64_t*)malloc(sizeof(int64_t)*nb_items);
  for (int64_t i = 0; i < nb_items; i++) {
    a[i] = 1;
  }
  print_header = [=] {
    print_prog("plus_reduce_array");
    aprintf("n %lu\n", nb_items);
  };
}

auto bench_body_interrupt(promotable* p) -> void {
  rollforward_table = {
    mk_rollforward_entry(plus_reduce_array_interrupt_l0, plus_reduce_array_interrupt_rf_l0),
    mk_rollforward_entry(plus_reduce_array_interrupt_l1, plus_reduce_array_interrupt_rf_l1),
    mk_rollforward_entry(plus_reduce_array_interrupt_l2, plus_reduce_array_interrupt_rf_l2),
    mk_rollforward_entry(plus_reduce_array_interrupt_l3, plus_reduce_array_interrupt_rf_l3),
  };
  plus_reduce_array_interrupt(a, 0, nb_items, &result, p);
}

auto bench_body_software_polling(promotable* p) -> void {
  plus_reduce_array_software_polling(a, 0, nb_items, &result, p);
}

auto bench_body_serial(promotable* p) -> void {
  result = plus_reduce_array_serial(a, 0, nb_items);
}

auto bench_post() -> void {
  int64_t m = 0;
  for (int64_t i = 0; i < nb_items; i++) {
    m += a[i];
  }
  assert(m == nb_items);
  free(a);
}

void bench_plus_reduce_array_interrupt() {
  sched_configuration = sched_configuration_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_plus_reduce_array_interrupt_nopromote() {
  sched_configuration = sched_configuration_nopromote_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_plus_reduce_array_software_polling() {
  sched_configuration = sched_configuration_software_polling;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_plus_reduce_array_serial() {
  sched_configuration = sched_configuration_serial;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_plus_reduce_array_manual() {
  sched_configuration = sched_configuration_manual;
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new plus_reduce_array_manual<microbench_scheduler_type>(a, 0, nb_items, &result);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

std::vector<std::function<void()>> plus_reduce_array_thunks = {  std::bind(bench_plus_reduce_array_interrupt),
                                                                 std::bind(bench_plus_reduce_array_interrupt_nopromote),
                                                                 std::bind(bench_plus_reduce_array_software_polling),
                                                                 std::bind(bench_plus_reduce_array_serial),
                                                                 std::bind(bench_plus_reduce_array_manual)
};

} // end namespace
} // end namespace

extern "C" {
  void handle_plus_reduce_array_interrupt(char *buf, void *priv) {
    tpalrts::plus_reduce_array::bench_plus_reduce_array_interrupt();
  }
  void handle_plus_reduce_array_interrupt_nopromote(char *buf, void *priv) {
    tpalrts::plus_reduce_array::bench_plus_reduce_array_interrupt_nopromote();
  }
  void handle_plus_reduce_array_software_polling(char *buf, void *priv) {
    tpalrts::plus_reduce_array::bench_plus_reduce_array_software_polling();
  }
  void handle_plus_reduce_array_manual(char *buf, void *priv) {
    tpalrts::plus_reduce_array::bench_plus_reduce_array_manual();
  }
  void handle_plus_reduce_array_serial(char *buf, void *priv) {
    tpalrts::plus_reduce_array::bench_plus_reduce_array_serial();
  }
  void handle_plus_reduce_array(char *buf, void *priv) {
    tpalrts::run_all(tpalrts::plus_reduce_array::plus_reduce_array_thunks);
  }
}

#endif

/*---------------------------------------------------------------------*/
/* Sparse matrix dense vector product */

#if 0

namespace tpalrts {
namespace bench_spmv {

uint64_t hash64(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

int64_t n;
int64_t row_len;
size_t nb_rows;
int64_t nb_vals;
double* val;
int64_t* row_ptr;
int64_t* col_ind;
double* x;
double* y;
char* env;
  
auto bench_pre() -> void {
  n = 500 * 1000 * 1000;
  row_len = std::min(n, (int64_t)1000);
  nb_rows = n / row_len;
  nb_vals = n;
  val = (double*)malloc(sizeof(double) * nb_vals);
  row_ptr = (int64_t*)malloc(sizeof(int64_t) * (nb_rows + 1));
  col_ind = (int64_t*)malloc(sizeof(int64_t) * nb_vals);
  x = (double*)malloc(sizeof(double) * nb_rows);
  y = (double*)malloc(sizeof(double) * nb_rows);
  env = (char*)malloc(SPMV_SZB);
  {
    int64_t a = 0;
    for (int64_t i = 0; i != nb_rows; i++) {
      row_ptr[i] = a;
      a += row_len;
    }
    row_ptr[nb_rows] = a;
  }
  for (int64_t i = 0; i != nb_vals; i++) {
    col_ind[i] = hash64(i) % nb_rows;
  }
  for (int64_t i = 0; i != nb_rows; i++) {
    x[i] = 1.0;
    y[i] = 0.0;
  }
  for (int64_t i = 0; i != nb_vals; i++) {
    val[i] = 1.0;
  }
  print_header = [=] {
    print_prog("spmv");
    aprintf("n %lu\n", n);
  };
}

auto bench_body_interrupt(promotable* p) -> void {
  rollforward_table = {
    mk_rollforward_entry(spmv_l0, spmv_rf_l0),
    mk_rollforward_entry(spmv_l1, spmv_rf_l1),
    mk_rollforward_entry(spmv_l2, spmv_rf_l2),
    mk_rollforward_entry(spmv_l3, spmv_rf_l3),
    mk_rollforward_entry(spmv_l4, spmv_rf_l4),
    mk_rollforward_entry(spmv_l5, spmv_rf_l5),
    mk_rollforward_entry(spmv_l6, spmv_rf_l6),
    mk_rollforward_entry(spmv_l7, spmv_rf_l7),
    mk_rollforward_entry(spmv_l8, spmv_rf_l8),
    mk_rollforward_entry(spmv_l9, spmv_rf_l9),
    mk_rollforward_entry(spmv_l10, spmv_rf_l10),
    mk_rollforward_entry(spmv_l11, spmv_rf_l11),
    mk_rollforward_entry(spmv_l12, spmv_rf_l12),
    mk_rollforward_entry(spmv_l13, spmv_rf_l13),
    mk_rollforward_entry(spmv_l14, spmv_rf_l14),
    mk_rollforward_entry(spmv_l15, spmv_rf_l15), 

    mk_rollforward_entry(spmv_col_l0, spmv_col_rf_l0),
    mk_rollforward_entry(spmv_col_l1, spmv_col_rf_l1),
    mk_rollforward_entry(spmv_col_l2, spmv_col_rf_l2),
    mk_rollforward_entry(spmv_col_l3, spmv_col_rf_l3),
    mk_rollforward_entry(spmv_col_l4, spmv_col_rf_l4),
    mk_rollforward_entry(spmv_col_l5, spmv_col_rf_l5),
    mk_rollforward_entry(spmv_col_l6, spmv_col_rf_l6),
  };
  GET_FROM_ENV(double*, SPMV_OFF01, env) = val;
  GET_FROM_ENV(int64_t*, SPMV_OFF02, env) = row_ptr;
  GET_FROM_ENV(int64_t*, SPMV_OFF03, env) = col_ind;
  GET_FROM_ENV(double*, SPMV_OFF04, env) = x;
  GET_FROM_ENV(double*, SPMV_OFF05, env) = y;
  GET_FROM_ENV(int64_t, SPMV_OFF06, env) = 0;
  GET_FROM_ENV(int64_t, SPMV_OFF07, env) = nb_rows;
  GET_FROM_ENV(promotable*, SPMV_OFF10, env) = p;
  GET_FROM_ENV(double*, SPMV_OFF12, env) = nullptr;
  spmv_interrupt(env);
}

auto bench_body_software_polling(promotable* p) -> void {
  spmv_software_polling(val, row_ptr, col_ind, x, y, nb_rows, p);
}

auto bench_body_serial(promotable* p) -> void {
  //spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);
  GET_FROM_ENV(double*, SPMV_OFF01, env) = val;
  GET_FROM_ENV(int64_t*, SPMV_OFF02, env) = row_ptr;
  GET_FROM_ENV(int64_t*, SPMV_OFF03, env) = col_ind;
  GET_FROM_ENV(double*, SPMV_OFF04, env) = x;
  GET_FROM_ENV(double*, SPMV_OFF05, env) = y;
  GET_FROM_ENV(int64_t, SPMV_OFF06, env) = 0;
  GET_FROM_ENV(int64_t, SPMV_OFF07, env) = nb_rows;
  GET_FROM_ENV(promotable*, SPMV_OFF10, env) = p;
  GET_FROM_ENV(double*, SPMV_OFF12, env) = nullptr;
  spmv_interrupt(env);
}

auto bench_post() -> void {
#ifndef NDEBUG
    double* yref = (double*)malloc(sizeof(double) * nb_rows);
    {
      for (int64_t i = 0; i != nb_rows; i++) {
        yref[i] = 1.0;
      }
      spmv_serial(val, row_ptr, col_ind, x, yref, nb_rows);
    }
    int64_t nb_diffs = 0;
    double epsilon = 0.01;
    for (int64_t i = 0; i != nb_rows; i++) {
      auto diff = std::abs(y[i] - yref[i]);
      if (diff > epsilon) {
        //printf("diff=%f y[i]=%f yref[i]=%f at i=%ld\n", diff, y[i], yref[i], i);
        nb_diffs++;
      }
    }
    printk("nb_diffs %ld\n", nb_diffs);
    free(yref);
#endif
    free(env);
    free(val);
    free(row_ptr);
    free(x);
    free(y);
}

void bench_spmv_interrupt() {
  sched_configuration = sched_configuration_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_spmv_interrupt_nopromote() {
  sched_configuration = sched_configuration_nopromote_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_spmv_software_polling() {
  sched_configuration = sched_configuration_software_polling;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_spmv_serial() {
  sched_configuration = sched_configuration_serial;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_spmv_manual() {
  sched_configuration = sched_configuration_manual;
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new spmv_manual<microbench_scheduler_type>(val, row_ptr, col_ind, x, y, 0, nb_rows, 0, 0);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

std::vector<std::function<void()>> spmv_thunks = {  std::bind(bench_spmv_interrupt),
                                                    std::bind(bench_spmv_interrupt_nopromote),
                                                    std::bind(bench_spmv_software_polling),
                                                    std::bind(bench_spmv_serial),
                                                    std::bind(bench_spmv_manual)
};
  
} // end namespace
} // end namespace

extern "C" {
  void handle_spmv_interrupt(char *buf, void *priv) {
    tpalrts::bench_spmv::bench_spmv_interrupt();
  }
  void handle_spmv_interrupt_nopromote(char *buf, void *priv) {
    tpalrts::bench_spmv::bench_spmv_interrupt_nopromote();
  }
  void handle_spmv_software_polling(char *buf, void *priv) {
    tpalrts::bench_spmv::bench_spmv_software_polling();
  }
  void handle_spmv_manual(char *buf, void *priv) {
    tpalrts::bench_spmv::bench_spmv_manual();
  }
  void handle_spmv_serial(char *buf, void *priv) {
    tpalrts::bench_spmv::bench_spmv_serial();
  }
  void handle_spmv(char *buf, void *priv) {
    tpalrts::run_all(tpalrts::bench_spmv::spmv_thunks);
  }
}

#endif

/*---------------------------------------------------------------------*/
/* Fib */

#if 0

namespace tpalrts {
namespace fib {

uint64_t n = 40;
uint64_t r = 0;
tpalrts::stack_type s;
  
auto bench_pre() -> void {
  print_header = [=] {
    print_prog("fib");
    aprintf("n %lu\n", n);
  };
}

auto bench_body_interrupt(promotable* p) -> void {
  rollforward_table = {
  };
  s = tpalrts::snew();
  fib_heartbeat<heartbeat_mechanism_hardware_interrupt>(n, &r, p, 128, s);  
}

auto bench_body_software_polling(promotable* p) -> void {
  s = tpalrts::snew();
  fib_heartbeat<heartbeat_mechanism_software_polling>(n, &r, p, 128, s);
}

auto bench_body_serial(promotable* p) -> void {
  //  s = tpalrts::snew();
  //  r = fib_custom_stack_serial(n, 128, s);
  r = fib_serial(n);
}

auto bench_post() -> void {
  assert(r == fib_serial(n));
}

void bench_fib_interrupt() {
  sched_configuration = sched_configuration_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_fib_interrupt_nopromote() {
  sched_configuration = sched_configuration_nopromote_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_fib_software_polling() {
  sched_configuration = sched_configuration_software_polling;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_fib_serial() {
  sched_configuration = sched_configuration_serial;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_fib_manual() {
  sched_configuration = sched_configuration_manual;
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new fib_manual<microbench_scheduler_type>(n, &r);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

std::vector<std::function<void()>> fib_thunks = {  std::bind(bench_fib_interrupt),
                                                   std::bind(bench_fib_interrupt_nopromote),
                                                   std::bind(bench_fib_software_polling),
                                                   std::bind(bench_fib_serial),
                                                   std::bind(bench_fib_manual)
};
  
} // end namespace
} // end namespace

extern "C" {
  void handle_fib_interrupt(char *buf, void *priv) {
    tpalrts::fib::bench_fib_interrupt();
  }
  void handle_fib_interrupt_nopromote(char *buf, void *priv) {
    tpalrts::fib::bench_fib_interrupt_nopromote();
  }
  void handle_fib_software_polling(char *buf, void *priv) {
    tpalrts::fib::bench_fib_software_polling();
  }
  void handle_fib_manual(char *buf, void *priv) {
    tpalrts::fib::bench_fib_manual();
  }
  void handle_fib_serial(char *buf, void *priv) {
    tpalrts::fib::bench_fib_serial();
  }
  void handle_fib(char *buf, void *priv) {
    tpalrts::run_all(tpalrts::fib::fib_thunks);
  }
}

#endif

/*---------------------------------------------------------------------*/
/* Knapsack */

#if 0

namespace tpalrts {
namespace knapsack {

int n, capacity;
int sol = INT_MIN;
#define MAX_ITEMS 256
struct item items[MAX_ITEMS];
tpalrts::stack_type s;
  
auto bench_pre() -> void {
  n = knapsack_n;
  capacity = knapsack_capacity;
  knapsack_init(items);
  best_so_far.store(INT_MIN);
  seq_best_so_far = INT_MIN;
  print_header = [=] {
    print_prog("knapsack");
  };
}

auto bench_body_interrupt(promotable* p) -> void {
  rollforward_table = {
  };
  s = tpalrts::snew();
  knapsack_heartbeat<heartbeat_mechanism_hardware_interrupt>(items, capacity, n, 0, &sol, p, s);
}

auto bench_body_software_polling(promotable* p) -> void {
  s = tpalrts::snew();
  knapsack_heartbeat<heartbeat_mechanism_software_polling>(items, capacity, n, 0, &sol, p, s);
}

auto bench_body_serial(promotable* p) -> void {
  knapsack_seq(items, capacity, n, 0, &sol);
  //  s = tpalrts::snew();
  //  sol = knapsack_custom_stack_serial(items, capacity, n, 0, s);
}

auto bench_post() -> void {
  //  assert(r == knapsack_serial(n));
}

void bench_knapsack_interrupt() {
  sched_configuration = sched_configuration_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_knapsack_interrupt_nopromote() {
  sched_configuration = sched_configuration_nopromote_interrupt;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_knapsack_software_polling() {
  sched_configuration = sched_configuration_software_polling;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_knapsack_serial() {
  sched_configuration = sched_configuration_serial;
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_knapsack_manual() {
  sched_configuration = sched_configuration_manual;
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new knapsack_manual<microbench_scheduler_type>();
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

std::vector<std::function<void()>> knapsack_thunks = {  std::bind(bench_knapsack_interrupt),
                                                        std::bind(bench_knapsack_interrupt_nopromote),
                                                        std::bind(bench_knapsack_software_polling),
                                                        std::bind(bench_knapsack_serial),
                                                        std::bind(bench_knapsack_manual)
};
  
} // end namespace
} // end namespace

extern "C" {
  void handle_knapsack_interrupt(char *buf, void *priv) {
    tpalrts::knapsack::bench_knapsack_interrupt();
  }
  void handle_knapsack_interrupt_nopromote(char *buf, void *priv) {
    tpalrts::knapsack::bench_knapsack_interrupt_nopromote();
  }
  void handle_knapsack_software_polling(char *buf, void *priv) {
    tpalrts::knapsack::bench_knapsack_software_polling();
  }
  void handle_knapsack_manual(char *buf, void *priv) {
    tpalrts::knapsack::bench_knapsack_manual();
  }
  void handle_knapsack_serial(char *buf, void *priv) {
    tpalrts::knapsack::bench_knapsack_serial();
  }
  void handle_knapsack(char *buf, void *priv) {
    tpalrts::run_all(tpalrts::knapsack::knapsack_thunks);
  }
}


#endif

/*---------------------------------------------------------------------*/
/* Fills some C++ stdlib functions that cannot be linked w/ Nautilus */

namespace std {void __throw_bad_alloc() { while(1); }; }
namespace std { void __throw_length_error(char const*) { }}
namespace std { void __throw_logic_error(char const*) { }}
namespace std {void __throw_bad_function_call() { while(1); }; }
const struct std::nothrow_t std::nothrow{};

void* operator new(std::size_t n, std::nothrow_t const&) {
  void * const p = std::malloc(n);
  // handle p == 0
  return p;
}

void operator delete(void * p, std::size_t) { // or delete(void *, std::size_t)
  std::free(p);
}

void operator delete(void * p, std::nothrow_t const&) {
  std::free(p);
}

void* operator new(std::size_t n, std::align_val_t) {
  void * const p = std::malloc(n);
  // handle p == 0
  return p;
}

void operator delete(void* p, unsigned long, std::align_val_t) {
  std::free(p);
}
