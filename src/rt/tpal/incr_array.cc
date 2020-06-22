#include "incr_array.hpp"

#include "mcsl_chaselev.hpp"

#include "tpalrts_scheduler.hpp"
#include "tpalrts_fiber.hpp"

namespace tpalrts {

mcsl::clock::time_point_type start_time;
double elapsed_time;

template <typename Scheduler, typename Worker, typename Interrupt,
          typename Bench_pre, typename Bench_post, typename Fiber_body>
void launch0(std::size_t nb_workers,
	     const Bench_pre& bench_pre,
	     const Bench_post& bench_post,
	     Fiber_body f_body) {
  mcsl::init_print_lock();
  bench_pre();
  logging::initialize();
  {
    auto f_pre = new fiber<Scheduler>([=] (promotable*) {
      stats::start_collecting();
      logging::log_event(mcsl::enter_algo);
      start_time = mcsl::clock::now();                                        
    });
    auto f_cont = new fiber<Scheduler>([=] (promotable*) {
      elapsed_time = mcsl::clock::since(start_time);
      logging::log_event(mcsl::exit_algo);
      stats::report(nb_workers);
    });
    auto f_term = new terminal_fiber<Scheduler>();
    fiber<Scheduler>::add_edge(f_pre, f_body);
    fiber<Scheduler>::add_edge(f_body, f_cont);
    fiber<Scheduler>::add_edge(f_cont, f_term);
    f_pre->release();
    f_body->release();
    f_cont->release();
    f_term->release();
  }
  using scheduler_type = mcsl::chase_lev_work_stealing_scheduler<Scheduler, fiber, stats, logging, mcsl::minimal_elastic, Worker, Interrupt>;
  scheduler_type::launch(nb_workers);
  bench_post();
  printk("exectime %.3f\n", elapsed_time);
  logging::output(nb_workers);
}
  
} // end namespace

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

/*---------------------------------------------------------------------*/

namespace tpalrts {

void bench_incr_array() {
  std::size_t nb_workers = 3;
  mcsl::perworker::unique_id::initialize(nb_workers);
  rollforward_table = {
    mk_rollforward_entry(incr_array_interrupt_l0, incr_array_interrupt_rf_l0),
    mk_rollforward_entry(incr_array_interrupt_l1, incr_array_interrupt_rf_l1),
    mk_rollforward_entry(incr_array_interrupt_l2, incr_array_interrupt_rf_l2),
    mk_rollforward_entry(incr_array_interrupt_l3, incr_array_interrupt_rf_l3),
  };
  uint64_t nb_items = 40000000;
  int64_t* a = (int64_t*)malloc(sizeof(int64_t)*nb_items);
  auto bench_pre = [=] {
    for (int64_t i = 0; i < nb_items; i++) {
      a[i] = 0;
    }
  };
  auto bench_body_interrupt = [=] (promotable* p) {
    incr_array_interrupt(a, 0, nb_items, p);
  };
  auto bench_body_software_polling = [=] (promotable* p) {
    incr_array_software_polling(a, 0, nb_items, p);
  };
  auto bench_body_serial = [=] (promotable* p) {
    incr_array_serial(a, 0, nb_items);
  };
  auto bench_post = [=]   {
    int64_t m = 0;
    for (int64_t i = 0; i < nb_items; i++) {
      m += a[i];
    }
    assert(m == nb_items);
    free(a);
  };
  /*
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
  auto bench_body_manual = new incr_array_manual<microbench_scheduler_type>(a, 0, nb_items);
  launch0<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, bench_pre, bench_post, bench_body_manual);
  */
  using microbench_scheduler_type = mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, ping_thread_worker, ping_thread_interrupt>;
  auto f_body = new fiber<microbench_scheduler_type>([=] (promotable* p) {
    bench_body_interrupt(p);
  });
  launch0<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, bench_pre, bench_post, f_body);
}

}

extern "C" {
  void bench_incr_array() {
    tpalrts::bench_incr_array();
  }
}
