#include <rt/heartbeat/mcsl_fiber.hpp>

namespace mcsl {

/*---------------------------------------------------------------------*/
/* Trivial termination detector, which does nothing */
  
class noop_termination_detection_barrier {
public:

  bool set_active(bool active) {
    return false;
  }

  bool is_terminated() {
    return false;
  }
  
};

/*---------------------------------------------------------------------*/
/* Stats */

#define SPAWNBENCH_STATS 1
  
class stats_configuration {
public:

#ifdef SPAWNBENCH_STATS
  static constexpr
  bool enabled = true;
#else
  static constexpr
  bool enabled = false;
#endif

  using counter_id_type = enum counter_id_enum {
    nb_promotions,
    nb_steals,
    nb_counters
  };

  static
  const char* name_of_counter(counter_id_type id) {
    if (id == nb_promotions) return "nb_promotions";
    if (id == nb_steals) return "nb_steals";
    return "<unknown event>";
  }
  
};

using stats = mcsl::stats_base<stats_configuration>;

/*---------------------------------------------------------------------*/
/* Logging */

#ifdef SPAWNBENCH_LOGGING
using logging = logging_base<true>;
#else
using logging = logging_base<false>;
#endif

/*---------------------------------------------------------------------*/
/* Basic scheduler configuration */

class basic_scheduler_configuration {
public:
  
  static
  void initialize_worker() {
    
  }  

  static
  void initialize_signal_handler() {

  }

  template <template <typename> typename Fiber>
  static
  void schedule(Fiber<basic_scheduler_configuration>* f) {
    mcsl::schedule<basic_scheduler_configuration, Fiber, stats, logging>(f);
  }

  template <template <typename> typename Fiber>
  static
  Fiber<basic_scheduler_configuration>* take() {
    return mcsl::take<basic_scheduler_configuration, Fiber, stats, logging>();
  }

  using termination_detection_barrier_type = noop_termination_detection_barrier;

  using stats = mcsl::stats;
  using logging = mcsl::logging;

};

/* A fiber that, when executed, initiates the teardown of the 
 * scheduler. 
 */
  
template <typename Scheduler_configuration>
class terminal_fiber : public fiber<Scheduler_configuration> {
public:
  
  terminal_fiber()
    : fiber<Scheduler_configuration>([] (promotable*) { return mcsl::fiber_status_finish; }) { }
  
  mcsl::fiber_status_type exec() {
    return mcsl::fiber_status_terminate;
  }
  
};

/*---------------------------------------------------------------------*/
/* Scheduler launch */

int64_t fib_T = 15;

int64_t fib_seq(int64_t n) {
  if (n <= 1) {
    return n;
  } else {
    return fib_seq(n-1) + fib_seq(n-2);
  }
}

template <typename Scheduler_configuration>
class fib_fiber : public mcsl::fiber<Scheduler_configuration> {
public:

  using trampoline_type = enum { entry, exit };

  trampoline_type trampoline = entry;

  int64_t n; int64_t* dst;
  int64_t d1, d2;

  fib_fiber(int64_t n, int64_t* dst)
    : mcsl::fiber<Scheduler_configuration>([] (promotable*) { }), n(n), dst(dst) { }

  mcsl::fiber_status_type exec() {
    switch (trampoline) {
    case entry: {
      if (n <= fib_T) {
        *dst = fib_seq(n);
        break;
      }
      auto f1 = new fib_fiber(n-1, &d1);
      auto f2 = new fib_fiber(n-2, &d2);
      mcsl::fiber<Scheduler_configuration>::add_edge(f1, this);
      mcsl::fiber<Scheduler_configuration>::add_edge(f2, this);
      f1->release();
      f2->release();
      mcsl::stats::increment(mcsl::stats_configuration::nb_promotions);
      mcsl::stats::increment(mcsl::stats_configuration::nb_promotions);
      trampoline = exit;
      return mcsl::fiber_status_pause;	  
    }
    case exit: {
      *dst = d1 + d2;
      break;
    }
    }
    return mcsl::fiber_status_finish;
  }

};

int64_t res;

mcsl::clock::time_point_type start_time;
double elapsed_time;

static
void report_exectime() {
  printk("exectime %.3f\n", elapsed_time);
}
  
template <typename Scheduler_configuration>
void launch(std::size_t nb_workers) {
  perworker::unique_id::initialize(nb_workers);
  perworker::unique_id::initialize_tls_worker(0);
  logging::initialize();
  {
    auto f_body = new fib_fiber<Scheduler_configuration>(33, &res);
    auto f_cont = new fiber<Scheduler_configuration>([=] (promotable*)  {
      elapsed_time = mcsl::clock::since(start_time);
    });
    auto f_term = new terminal_fiber<Scheduler_configuration>();
    fiber<Scheduler_configuration>::add_edge(f_body, f_cont);
    fiber<Scheduler_configuration>::add_edge(f_cont, f_term);
    start_time = mcsl::clock::now();
    f_body->release();
    f_cont->release();
    f_term->release();
  }
  using scheduler_type = mcsl::chase_lev_work_stealing_scheduler<Scheduler_configuration, fiber, stats, logging>;
  stats::on_enter_launch();
  scheduler_type::launch(nb_workers);
  stats::on_exit_launch();
  //bench_post();
  //report_exectime();
  printk("result %ld\n",res);
  stats::report();
  logging::output();
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

extern "C" {
  void microbench() {
    std::size_t nb_workers = 4;
    mcsl::launch<mcsl::basic_scheduler_configuration>(nb_workers);
  }
}
