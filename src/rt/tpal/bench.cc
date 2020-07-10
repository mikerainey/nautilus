#include "incr_array.hpp"
#include "plus_reduce_array.hpp"
#include "spmv.hpp"
#include "fib.hpp"

#include "tpalrts.hpp"

/*---------------------------------------------------------------------*/
/* Control the setting of kappa */

namespace tpalrts {


void set_kappa_20() {
  set_kappa_usec(dflt_cpu_freq_ghz, 20);
}

void set_kappa_40() {
  set_kappa_usec(dflt_cpu_freq_ghz, 40);
}

void set_kappa_100() {
  set_kappa_usec(dflt_cpu_freq_ghz, 100);
}

void set_kappa_400() {
  set_kappa_usec(dflt_cpu_freq_ghz, 400);
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
}

/*---------------------------------------------------------------------*/
/* Incr array */

namespace tpalrts {  
namespace bench_incr_array {

uint64_t nb_items = 1000 * 1000 * 1000;
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

} // end namespace
} // end namespace

extern "C" {
  void handle_incr_array_interrupt(char *buf, void *priv) {
    tpalrts::bench_incr_array::bench_incr_array_interrupt();
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
}

/*---------------------------------------------------------------------*/
/* Plus reduce array */

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
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_plus_reduce_array_software_polling() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_plus_reduce_array_serial() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_plus_reduce_array_manual() {
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new plus_reduce_array_manual<microbench_scheduler_type>(a, 0, nb_items, &result);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

} // end namespace
} // end namespace

extern "C" {
  void handle_plus_reduce_array_interrupt(char *buf, void *priv) {
    tpalrts::plus_reduce_array::bench_plus_reduce_array_interrupt();
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
}

/*---------------------------------------------------------------------*/
/* Sparse matrix dense vector product */

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
  n = 90000000;
  row_len =std::min(n, (int64_t)1000);
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
  spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);
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
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_spmv_software_polling() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_spmv_serial() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_spmv_manual() {
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new spmv_manual<microbench_scheduler_type>(val, row_ptr, col_ind, x, y, 0, nb_rows, 0, 0);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

} // end namespace
} // end namespace

extern "C" {
  void handle_spmv_interrupt(char *buf, void *priv) {
    tpalrts::bench_spmv::bench_spmv_interrupt();
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
}

/*---------------------------------------------------------------------*/
/* Fib */

namespace tpalrts {
namespace fib {

uint64_t n = 40;
uint64_t r = 0;
tpalrts::stack_type s;
  
auto bench_pre() -> void {
  
}

auto bench_body_interrupt(promotable* p) -> void {
  rollforward_table = {
  };
  s = tpalrts::snew();
  fib_heartbeat<heartbeat_mechanism_hardware_interrupt>(n, &r, p, 128, s, fib_heartbeat_entry, 0);  
}

auto bench_body_software_polling(promotable* p) -> void {
  s = tpalrts::snew();
  fib_heartbeat<heartbeat_mechanism_software_polling>(n, &r, p, 128, s, fib_heartbeat_entry, 0);
}

auto bench_body_serial(promotable* p) -> void {
  s = tpalrts::snew();
  r = fib_custom_stack_serial(n, 128, s);
}

auto bench_post() -> void {
  if (s.stack != nullptr) {
    sdelete(s);
  }
  assert(r == fib_serial(n));
}

void bench_fib_interrupt() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, bench_body_interrupt);
}

void bench_fib_software_polling() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_software_polling);
}

void bench_fib_serial() {
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_serial);
}

void bench_fib_manual() {
  mcsl::perworker::unique_id::initialize(nb_workers);
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  bench_pre();
  auto bench_body_manual = new fib_manual<microbench_scheduler_type>(n, &r);
  auto bench_pre2 = [] { };
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre2, bench_post, bench_body_manual);
}

  
} // end namespace
} // end namespace

extern "C" {
  void handle_fib_interrupt(char *buf, void *priv) {
    tpalrts::fib::bench_fib_interrupt();
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
}

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
