#include <limits.h>
#include <functional>
#include <vector>
#include <cstring>

#include "mcsl_chaselev.hpp"
#include "mcsl_machine.hpp"

#include "tpalrts_scheduler.hpp"
#include "tpalrts_fiber.hpp"

#include "incr_array.hpp"
#include "plus_reduce_array.hpp"
#include "floyd_warshall.hpp"
#include "mandelbrot.hpp"
#include "kmeans.hpp"
#include "spmv.hpp"
#include "knapsack.hpp"
#include "mergesort.hpp"

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

namespace tpalrts {

uint64_t cpu_freq_khz = 0;

using scheduler_configuration_type = enum scheduler_configurations_enum {
  scheduler_configuration_serial,
  scheduler_configuration_software_polling,
  scheduler_configuration_interrupt_ping_thread,
  scheduler_configuration_nopromote_interrupt_ping_thread,
  scheduler_configuration_manual,
  nb_scheduler_configurations
};

char* scheduler_configuration_names [] = {
  "serial",
  "software_polling",
  "interrupt_ping_thread",
  "nopromote_interrupt_ping_thread",
  "manual",
  "<bogus>"
};

char* scheduler_configuration_name = scheduler_configuration_names[nb_scheduler_configurations];

scheduler_configuration_type scheduler_configuration = nb_scheduler_configurations;

auto find_string(char* s, char** ss, std::size_t n) -> std::size_t {
  std::size_t i = 0;
  for (; i < n; i++) {
    if (strcmp(s, ss[i]) == 0) {
      return i;
    }
  }
  return n;
};

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

using promotable_function_type = std::function<void(promotable*)>;
 
using benchmark_type = struct benchmark_struct {
  promotable_function_type pre;
  std::vector<promotable_function_type> bodies;
  promotable_function_type post;
};

std::vector<benchmark_type> benchmarks;

template <typename Pre,
	  typename Body_serial,
	  typename Body_interrupt,
	  typename Post>
auto add_benchmark(const Pre& pre,
		   const Body_serial& body_serial,
		   const Body_interrupt& body_interrupt,
		   const Post& post) {
  std::vector<promotable_function_type> bodies(nb_scheduler_configurations, body_interrupt);
  bodies[scheduler_configuration_serial] = promotable_function_type(body_serial);
  bodies[scheduler_configuration_interrupt_ping_thread] = promotable_function_type(body_interrupt);
  benchmark_type b = {
    .pre = promotable_function_type(pre),
    .bodies = bodies,
    .post = promotable_function_type(post) };
  benchmarks.push_back(b);
};

template <typename Scheduler, typename Worker, typename Interrupt>
void launch(std::size_t nb_workers,
	    std::vector<benchmark_type> benchmarks) {
  cpu_freq_khz = nk_detect_cpu_freq((uint32_t)0);
  mcsl::nb_workers = nb_workers;
  mcsl::perworker::unique_id::initialize(nb_workers);
  auto nb_cpus = nk_get_num_cpus();
  assert(nb_workers <= nb_cpus);
  mcsl::initialize_machine();
  assign_kappa(cpu_freq_khz, kappa_usec);
  std::vector<fiber<Scheduler>*> fibers;
  auto f_cont = new fiber<Scheduler>([=] (promotable*) {});
  auto f_term = new terminal_fiber<Scheduler>();
  fibers.push_back(f_cont);
  fibers.push_back(f_term);
  fiber<Scheduler>::add_edge(f_cont, f_term);
  aprintf("==========\n");
  for (auto b : benchmarks) {
    auto f_pre = new fiber<Scheduler>([=] (promotable* p) {
      b.pre(p);
      stats::start_collecting();
      start_cycle = mcsl::cycles::now();
    });
    auto body = b.bodies[scheduler_configuration];
    auto f_body = new fiber<Scheduler>(body);
    auto f_post = new fiber<Scheduler>([=] (promotable* p) {
      elapsed_cycles = mcsl::cycles::since(start_cycle);
      {
	print_header();
	aprintf("nb_cpus %d\n", nb_cpus);
	aprintf("opsys nautilus\n");
	aprintf("scheduler_configuration %s\n", scheduler_configuration_name);
	aprintf("---\n");
	aprintf("proc %lu\n", nb_workers);
	aprintf("kappa_usec %lu\n", kappa_usec);
	aprintf("kappa_cycles %lu\n", kappa_cycles);
	aprintf("cpu_freq_khz %lu\n", cpu_freq_khz);
	aprintf("execcycles %lu\n", elapsed_cycles);
	auto et = mcsl::seconds_of(mcsl::load_cpu_frequency_khz(), elapsed_cycles);
	aprintf("exectime %lu.%03lu\n", et.seconds, et.milliseconds);
	aprintf("exectime_via_cycles %lu.%03lu\n", et.seconds, et.milliseconds);
      }
      stats::report(nb_workers);
      aprintf("==========\n");
      b.post(p);
    });
    fiber<Scheduler>::add_edge(f_pre, f_body);
    fiber<Scheduler>::add_edge(f_body, f_post);
    fiber<Scheduler>::add_edge(f_post, f_cont);
    fibers.push_back(f_pre);
    fibers.push_back(f_body);
    fibers.push_back(f_post);
    f_cont = f_pre;
  }
  for (auto f : fibers) {
    f->release();
  }
  using scheduler_type = mcsl::chase_lev_work_stealing_scheduler<Scheduler, fiber, stats, logging, mcsl::minimal_elastic, Worker, Interrupt>;
  scheduler_type::launch(nb_workers);
  ping_thread_interrupt::ping_thread_status.store(ping_thread_status_active);
} 

void benchmark_init(int argc, char** argv) {
  uint64_t kappa_usec = tpalrts::dflt_kappa_usec;
  uint64_t nb_workers = 1;
  // parse command line
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "kappa_usec") == 0) {
      if ((i + 1) < argc) {
	kappa_usec = atoi(argv[i + 1]);
      }
    } else if (strcmp(argv[i], "proc") == 0) {
      if ((i + 1) < argc) {
	nb_workers = atoi(argv[i + 1]);
      }
    } else if (strcmp(argv[i], "scheduler_configuration") == 0) {
      if ((i + 1) < argc) {
	auto s = argv[i + 1];
	auto n = sizeof(scheduler_configuration_names);
	assert(nb_scheduler_configurations == n);
	auto j = find_string(s, scheduler_configuration_names, n);
	if (j < n) {
	  scheduler_configuration = (scheduler_configuration_type)j;
	  scheduler_configuration_name = scheduler_configuration_names[j];
	}
      }      
    }
  }
  // register benchmarks
  add_benchmark([] (promotable* p) {
    print_header = [=] {
      print_prog("mergesort");
      aprintf("n %lu\n", mergesort::n);
    };
    mergesort::bench_pre(p);
  },
    mergesort::bench_body_serial,
    mergesort::bench_body_interrupt,
    mergesort::bench_post);
  add_benchmark([] (promotable* p) {
    print_header = [=] {
      print_prog("knapsack");
      aprintf("n %lu\n", knapsack_n);
    };
    knapsack::bench_pre(p);
  },
    knapsack::bench_body_serial,
    knapsack::bench_body_interrupt,
    knapsack::bench_post);
  add_benchmark([] (promotable* p) {
    print_header = [=] {
      print_prog("spmv");
      aprintf("n %lu\n", spmv::n);
    };
    spmv::bench_pre(p);
  },
    spmv::bench_body_serial,
    spmv::bench_body_interrupt,
    spmv::bench_post);
  add_benchmark([] (promotable* p) {
    print_header = [=] {
      print_prog("kmeans");
      aprintf("n %lu\n", kmeans::numObjects);
    };
    kmeans::bench_pre(p);
  },
    kmeans::bench_body_serial,
    kmeans::bench_body_interrupt,
    kmeans::bench_post);  
  add_benchmark([] (promotable* p) {
    print_header = [] {
      print_prog("incr_array");
      aprintf("n %lu\n", incr_array::nb_items);
    };
    incr_array::bench_pre(p);
  },
    incr_array::bench_body_serial,
    incr_array::bench_body_interrupt,
    incr_array::bench_post);
  add_benchmark([] (promotable* p) {
    print_header = [=] {
      print_prog("plus_reduce_array");
      aprintf("n %lu\n", plus_reduce_array::nb_items);
    };
    plus_reduce_array::bench_pre(p);
  },
    plus_reduce_array::bench_body_serial,
    plus_reduce_array::bench_body_interrupt,
    plus_reduce_array::bench_post); 
  add_benchmark([] (promotable* p) {
    print_header = [=] {
      print_prog("mandelbrot");
      aprintf("n %lu\n", mandelbrot::nb_items);
    };
    mandelbrot::bench_pre(p);
  },
    mandelbrot::bench_body_serial,
    mandelbrot::bench_body_interrupt,
    mandelbrot::bench_post); 
  add_benchmark([] (promotable* p) {
    print_header = [=] {
      print_prog("floyd_warshall");
      aprintf("n %lu\n", floyd_warshall::vertices);
    };
    floyd_warshall::bench_pre(p);
  },
    floyd_warshall::bench_body_serial,
    floyd_warshall::bench_body_interrupt,
    floyd_warshall::bench_post);
  // launch scheduler
  switch (scheduler_configuration) {
  case scheduler_configuration_serial: {
    using microbench_scheduler_type =
      mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
    launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, benchmarks);
    break;
  }
  case scheduler_configuration_interrupt_ping_thread: {
    using microbench_scheduler_type =
      mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic,
			      ping_thread_worker, ping_thread_interrupt>;
    launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, benchmarks);
    break;
  }
  default: {
    aprintf("bogus choice for scheduler\n");
    return;
  }
  }
}

}

extern "C" {
  void tpal_bechmark_init(int argc, char** argv) {
    tpalrts::benchmark_init(argc, argv);
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
