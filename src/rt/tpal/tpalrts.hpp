#pragma once

#include "mcsl_chaselev.hpp"
#include "mcsl_machine.hpp"

#include "tpalrts_scheduler.hpp"
#include "tpalrts_fiber.hpp"

namespace tpalrts {

uint64_t cpu_freq_khz = 0;

char* sched_configuration_serial = "serial";

char* sched_configuration_software_polling = "software_polling";

char* sched_configuration_interrupt = "interrupt_ping_thread";

char* sched_configuration_nopromote_interrupt = "nopromote_interrupt_ping_thread";

char* sched_configuration_manual = "manual";

char* sched_configuration = "<bogus>";

static inline
void nautilus_assign_kappa(uint64_t kappa_usec) {
  if (cpu_freq_khz == 0) {
    cpu_freq_khz = mcsl::load_cpu_frequency_khz();
  }
  assert(cpu_freq_khz != 0);
  assign_kappa(cpu_freq_khz, kappa_usec);
}

/*---------------------------------------------------------------------*/
/* Launch */

std::function<void()> print_header;
uint64_t start_cycle, elapsed_cycles;
 
template <typename Scheduler, typename Worker, typename Interrupt,
          typename Bench_pre, typename Bench_post, typename Fiber_body>
void launch0(std::size_t nb_workers,
	     const Bench_pre& bench_pre,
	     const Bench_post& bench_post,
	     Fiber_body f_body) {
  auto nb_cpus = nk_get_num_cpus();
  assert(nb_workers <= nb_cpus);
  mcsl::initialize_machine();
  assign_kappa(cpu_freq_khz, kappa_usec);
  bench_pre();
  logging::initialize();
  {
    auto f_pre = new fiber<Scheduler>([=] (promotable*) {
      stats::start_collecting();
      logging::log_event(mcsl::enter_algo);
      start_cycle = mcsl::cycles::now();
    });
    auto f_cont = new fiber<Scheduler>([=] (promotable*) {
      elapsed_cycles = mcsl::cycles::since(start_cycle);
      logging::log_event(mcsl::exit_algo);
      {
        print_header();
        aprintf("nb_cpus %d\n", nb_cpus);
        aprintf("scheduler_configuration %s\n", sched_configuration);
        if (sched_configuration == sched_configuration_software_polling) {
          aprintf("software_polling_K %lu\n", dflt_software_polling_K);
        }
        aprintf("---\n");
        aprintf("proc %lu\n", nb_workers);
        aprintf("kappa_usec %lu\n", kappa_usec);
        aprintf("kappa_cycles %lu\n", kappa_cycles);
        aprintf("cpu_freq_khz %lu\n", cpu_freq_khz);
        aprintf("execcycles %lu\n", elapsed_cycles);
        {
          auto et2 = mcsl::seconds_of(mcsl::load_cpu_frequency_khz(), elapsed_cycles);
          aprintf("exectime %lu.%03lu\n", et2.seconds, et2.milliseconds);
        }
      }
      stats::report(nb_workers);
      aprintf("==========\n");
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
  bench_post();
  ping_thread_interrupt::ping_thread_status.store(ping_thread_status_active);
}

template <typename Scheduler, typename Worker, typename Interrupt,
          typename Bench_pre, typename Bench_post, typename Bench_body>
void launch(std::size_t nb_workers,
            const Bench_pre& bench_pre,
            const Bench_post& bench_post,
            const Bench_body& bench_body) {
  cpu_freq_khz = nk_detect_cpu_freq((uint32_t)0);
  mcsl::nb_workers = nb_workers;
  mcsl::perworker::unique_id::initialize(nb_workers);
  auto f_body = new fiber<Scheduler>(bench_body);
  launch0<Scheduler, Worker, Interrupt>(nb_workers, bench_pre, bench_post, f_body);
}
  
} // end namespace
