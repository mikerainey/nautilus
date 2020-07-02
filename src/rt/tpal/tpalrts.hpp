#pragma once

#include "mcsl_chaselev.hpp"

#include "tpalrts_scheduler.hpp"
#include "tpalrts_fiber.hpp"

namespace tpalrts {

mcsl::clock::time_point_type start_time, finish_time;

/*---------------------------------------------------------------------*/
/* Launch */
 
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
      finish_time = mcsl::clock::now();
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
  {
    uint64_t seconds = finish_time.tv_sec - start_time.tv_sec;
    uint64_t ns = finish_time.tv_nsec - start_time.tv_nsec;
    if (start_time.tv_nsec > finish_time.tv_nsec) { // clock underflow 
      --seconds; 
      ns += 1000000000; 
    }
    printk("exectime %lu.%09ld\n", seconds, ns);
  }
  logging::output(nb_workers);
}

template <typename Scheduler, typename Worker, typename Interrupt,
          typename Bench_pre, typename Bench_post, typename Bench_body>
void launch(std::size_t nb_workers,
            const Bench_pre& bench_pre,
            const Bench_post& bench_post,
            const Bench_body& bench_body) {
  mcsl::perworker::unique_id::initialize(nb_workers);
  auto f_body = new fiber<Scheduler>(bench_body);
  launch0<Scheduler, Worker, Interrupt>(nb_workers, bench_pre, bench_post, f_body);
}
  
} // end namespace
