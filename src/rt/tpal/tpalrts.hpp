#pragma once

#include "mcsl_chaselev.hpp"

#include "tpalrts_scheduler.hpp"
#include "tpalrts_fiber.hpp"

namespace tpalrts {

mcsl::clock::time_point_type start_time, finish_time;

// temporarily, assume we're benchmarking on the desktop machine, which has cpus @ 3ghz
static constexpr
double dflt_cpu_freq_ghz = 3.0;

char* sched_configuration_serial = "serial";

char* sched_configuration_software_polling = "software_polling";

char* sched_configuration_interrupt = "interrupt";

char* sched_configuration_manual = "manual";

char* sched_configuration = "<bogus>";

/*---------------------------------------------------------------------*/
/* Launch */

std::function<void()> print_header;
 
template <typename Scheduler, typename Worker, typename Interrupt,
          typename Bench_pre, typename Bench_post, typename Fiber_body>
void launch0(std::size_t nb_workers,
	     const Bench_pre& bench_pre,
	     const Bench_post& bench_post,
	     Fiber_body f_body) {
  set_kappa_usec(dflt_cpu_freq_ghz, kappa_usec);
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
      {
        uint64_t seconds = finish_time.tv_sec - start_time.tv_sec;
        uint64_t ns = finish_time.tv_nsec - start_time.tv_nsec;
        if (start_time.tv_nsec > finish_time.tv_nsec) { // clock underflow
          --seconds; 
          ns += 1000000000l; 
        }
        print_header();
        aprintf("scheduler_configuration %s\n", sched_configuration);
        aprintf("---\n");
        aprintf("nb_workers %lu\n", nb_workers);
        aprintf("kappa_usec %lu\n", kappa_usec);
        aprintf("exectime %lu.%09lu\n", seconds, ns);
      }
      stats::report(nb_workers);
      aprintf("==========\n");
      bench_post();
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
  logging::output(nb_workers);
  ping_thread_interrupt::ping_thread_status.store(ping_thread_status_active);
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
