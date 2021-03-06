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

namespace tpalrts {

/*---------------------------------------------------------------------*/

using scheduler_configuration_type = enum scheduler_configurations_enum {
  scheduler_configuration_serial,
  scheduler_configuration_software_polling,
  scheduler_configuration_interrupt_ping_thread,
  scheduler_configuration_nopromote_interrupt, // run interrupt version w/ interrupts disabled
  scheduler_configuration_serial_interrupt_ping_thread, // run serial version w/ interrupts enabled
  scheduler_configuration_manual,
  scheduler_bogus,
  nb_scheduler_configurations
};

char* scheduler_configuration_names [] = {
  "serial",
  "software_polling",
  "interrupt_ping_thread",
  "nopromote_interrupt",
  "serial_interrupt_ping_thread",
  "manual",
  "<bogus>"
};

auto is_scheduler_configuration_parallel(scheduler_configuration_type sc) {
  return ((sc == scheduler_configuration_interrupt_ping_thread) ||
	  (sc == scheduler_configuration_software_polling) ||
	  (sc == scheduler_configuration_manual));
}

/*---------------------------------------------------------------------*/

using promotable_function_type = std::function<void(promotable*)>;

using benchmark_type = struct benchmark_struct {
  promotable_function_type pre;
  std::vector<promotable_function_type> bodies;
  promotable_function_type post;
};

auto find_string(char* s, char** ss, std::size_t n) -> std::size_t {
  std::size_t i = 0;
  for (; i < n; i++) {
    if (strcmp(s, ss[i]) == 0) {
      return i;
    }
  }
  return n;
}

auto is_benchmark_in_list(char* name, std::vector<char*>& names) -> bool {
  auto n = names.size();
  if (n == 0) {
    return false;
  }
  return find_string(name, &names[0], n) != n;
}

std::vector<char*> benchmark_onlys;
std::vector<char*> benchmark_excludes;
std::vector<benchmark_type> benchmarks;

template <typename Pre,
	  typename Body_serial,
	  typename Body_interrupt,
	  typename Post>
auto add_benchmark(char* name,
		   const Pre& pre,
		   const Body_serial& body_serial,
		   const Body_interrupt& body_interrupt,
		   const Post& post) {
  if (is_benchmark_in_list(name, benchmark_excludes)) {
    return;
  }
  if ((! benchmark_onlys.empty()) && (! is_benchmark_in_list(name, benchmark_onlys))) {
    return;
  }
  std::vector<promotable_function_type> bodies(nb_scheduler_configurations, body_interrupt);
  bodies[scheduler_configuration_serial] = promotable_function_type(body_serial);
  bodies[scheduler_configuration_interrupt_ping_thread] = promotable_function_type(body_interrupt);
  bodies[scheduler_configuration_nopromote_interrupt] = promotable_function_type(body_interrupt);
  bodies[scheduler_configuration_serial_interrupt_ping_thread] = promotable_function_type(body_serial);
  benchmark_type b = {
    .pre = promotable_function_type(pre),
    .bodies = bodies,
    .post = promotable_function_type(post) };
  benchmarks.push_back(b);
}
  
std::function<void()> print_header;

/*---------------------------------------------------------------------*/

uint64_t cpu_freq_khz = 0;
  
uint64_t start_cycle, elapsed_cycles;

template <typename Scheduler, typename Worker, typename Interrupt>
void launch(std::size_t nb_workers,
	    uint64_t kappa_usec,
	    scheduler_configuration_type scheduler_configuration) {
  mcsl::nb_workers = nb_workers;
  auto nb_cpus = nk_get_num_cpus();
  assert(nb_workers <= nb_cpus);
  assert(cpu_freq_khz != 0);
  mcsl::perworker::unique_id::initialize(nb_workers);
  assign_kappa(cpu_freq_khz, kappa_usec);
  std::vector<fiber<Scheduler>*> fibers;
  auto f_cont = new fiber<Scheduler>([=] (promotable*) { });
  auto f_term = new terminal_fiber<Scheduler>();
  fibers.push_back(f_cont);
  fibers.push_back(f_term);
  fiber<Scheduler>::add_edge(f_cont, f_term);
  for (auto b : benchmarks) {
    auto f_pre = new fiber<Scheduler>([=] (promotable* p) {
      b.pre(p);
      initialize_rollfoward_table();
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
	auto sc = scheduler_configuration_names[scheduler_configuration];
	aprintf("scheduler_configuration %s\n", sc);
	aprintf("---\n");
	aprintf("proc %lu\n", nb_workers);
	aprintf("kappa_usec %lu\n", kappa_usec);
	aprintf("kappa_cycles %lu\n", kappa_cycles);
	aprintf("cpu_freq_khz %lu\n", cpu_freq_khz);
	aprintf("execcycles %lu\n", elapsed_cycles);
	auto et = mcsl::seconds_of(cpu_freq_khz, elapsed_cycles);
	aprintf("exectime %lu.%03lu\n", et.seconds, et.milliseconds);
	aprintf("exectime_via_cycles %lu.%03lu\n", et.seconds, et.milliseconds);
      }
      stats::report(nb_workers);
      aprintf("==========\n");
      destroy_rollfoward_table();
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
  using scheduler_type =
    mcsl::chase_lev_work_stealing_scheduler<Scheduler,
					    fiber,
					    stats,
					    logging,
					    mcsl::minimal_elastic,
					    Worker,
					    Interrupt>;
  scheduler_type::launch(nb_workers);
  ping_thread_interrupt::ping_thread_status.store(ping_thread_status_active);
}

/*---------------------------------------------------------------------*/
/* Command-line parsing */
  
auto parse_comma_list(char* cl, std::vector<char*>& v) {
  v.clear();
  char* s = strtok (cl,",");
  while (s != nullptr) {
    v.push_back(s);
    s = strtok (NULL, ",");
  }
}

auto parse_comma_list(char* cl, std::vector<int>& v) {
  v.clear();
  std::vector<char*> ss;
  parse_comma_list(cl, ss);
  for (auto s : ss) {
    v.push_back(atoi(s));
  }
}
  
auto parse_comma_list(char* cl, std::vector<scheduler_configuration_type>& v) {
  v.clear();
  std::vector<char*> ss;
  parse_comma_list(cl, ss);
  for (auto s : ss) {
    auto j = find_string(s, scheduler_configuration_names, nb_scheduler_configurations);
    if (j < nb_scheduler_configurations) {
      v.push_back((scheduler_configuration_type)j);
    }
  }
}
  
template <typename F>
auto parse_kvp(std::size_t i, std::size_t n, char** ss,
	       char* k, const F& f) {
  if (strcmp(ss[i], k) == 0) {
    if ((i + 1) < n) {
      f(ss[i + 1]);
    }
  }
}

auto parse_int(char* cl, int& n) {
  std::vector<int> v;
  parse_comma_list(cl, v);
  if (v.size() != 1) {
    aprintf("ERROR: bogus command line argument for %s\n", cl);
  }
  n = v.back();
}
  
auto print_prog(const char* s) {
  aprintf("prog run\n");
  aprintf("benchmark %s\n", s);
};

void benchmark_init(int argc, char** argv) {
  auto nb_cpus = nk_get_num_cpus();
  cpu_freq_khz = mcsl::load_cpu_frequency_khz();
  mcsl::initialize_machine();
  std::vector<scheduler_configuration_type> scheduler_configurations = {
    scheduler_configuration_serial,
    scheduler_configuration_interrupt_ping_thread,
    scheduler_configuration_nopromote_interrupt, 
    scheduler_configuration_serial_interrupt_ping_thread
  };
  std::vector<int> procs = {1, nb_cpus - 1};
  std::vector<int> kappa_usecs = {20, 100};
  auto parse_command_line = [&] {
    for (int i = 0; i < argc; i++) {
      parse_kvp(i, argc, argv, "kappa_usec",
		[&] (char* v) {
		  parse_comma_list(v, kappa_usecs);
		});
      parse_kvp(i, argc, argv, "proc",
		[&] (char* v) {
		  parse_comma_list(v, procs);
		});
      parse_kvp(i, argc, argv, "scheduler_configuration",
		[&] (char* v) {
		  parse_comma_list(v, scheduler_configurations);
		});
      parse_kvp(i, argc, argv, "excludes",
		[&] (char* v) {
		  parse_comma_list(v, benchmark_excludes);
		});
      parse_kvp(i, argc, argv, "onlys",
		[&] (char* v) {
		  parse_comma_list(v, benchmark_onlys);
		});
      parse_kvp(i, argc, argv, "freq",
		[&] (char* v) {
		  int cpu_freq_khz0 = 0;
		  parse_int(v, cpu_freq_khz0);
		  if (cpu_freq_khz0 != 0) {
		    cpu_freq_khz = cpu_freq_khz0;
		  }
		});
    }
    if ((! benchmark_excludes.empty()) && (! benchmark_onlys.empty())) {
      printk("ERROR, cannot have both excludes and onlys lists nonempty!\n");
      return false;
    }
    return true;
  };
  if (! parse_command_line()) {
    return;
  }
  // register benchmarks
  add_benchmark(spmv::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("spmv");
      aprintf("inputname arrowhead\n");
    };
    spmv::bench_pre_arrowhead(p);
  },
    spmv::bench_body_serial,
    spmv::bench_body_interrupt,
    spmv::bench_post);
  add_benchmark(spmv::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("spmv");
      aprintf("inputname bigcols\n");
    };
    spmv::bench_pre_bigcols(p);
  },
    spmv::bench_body_serial,
    spmv::bench_body_interrupt,
    spmv::bench_post);
  add_benchmark(spmv::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("spmv");
      aprintf("inputname bigrows\n");
    };
    spmv::bench_pre_bigrows(p);
  },
    spmv::bench_body_serial,
    spmv::bench_body_interrupt,
    spmv::bench_post);
  add_benchmark(mergesort::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("mergesort");
      aprintf("inputname uniformdist\n");
    };
    mergesort::bench_pre_uniformdist(p);
  },
    mergesort::bench_body_serial,
    mergesort::bench_body_interrupt,
    mergesort::bench_post);
  add_benchmark(mergesort::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("mergesort");
      aprintf("inputname expdist\n");
    };
    mergesort::bench_pre_expdist(p);
  },
    mergesort::bench_body_serial,
    mergesort::bench_body_interrupt,
    mergesort::bench_post);
  add_benchmark(knapsack::name,		
  [] (promotable* p) {
    print_header = [=] {
      print_prog("knapsack");
      aprintf("inputname thirty_six_items\n");
    };
    knapsack::bench_pre(p);
  },
    knapsack::bench_body_serial,
    knapsack::bench_body_interrupt,
    knapsack::bench_post);
  add_benchmark(kmeans::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("kmeans");
      aprintf("inputname one_million_objects\n");
    };
    kmeans::bench_pre(p);
  },
    kmeans::bench_body_serial,
    kmeans::bench_body_interrupt,
    kmeans::bench_post);  
  add_benchmark(incr_array::name,
  [] (promotable* p) {
    print_header = [] {
      print_prog("incr_array");
      aprintf("inputname one_hundred_million_64bit_ints\n");
    };
    incr_array::bench_pre(p);
  },
    incr_array::bench_body_serial,
    incr_array::bench_body_interrupt,
    incr_array::bench_post);
  add_benchmark(plus_reduce_array::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("plus_reduce_array");
      aprintf("inputname one_hundred_million_64bit_ints\n");
    };
    plus_reduce_array::bench_pre(p);
  },
    plus_reduce_array::bench_body_serial,
    plus_reduce_array::bench_body_interrupt,
    plus_reduce_array::bench_post); 
  add_benchmark(mandelbrot::name,
  [] (promotable* p) {
    print_header = [=] {
      print_prog("mandelbrot");
      aprintf("inputname fourk_by_fourk\n");
    };
    mandelbrot::bench_pre(p);
  },
    mandelbrot::bench_body_serial,
    mandelbrot::bench_body_interrupt,
    mandelbrot::bench_post); 
  add_benchmark(floyd_warshall::name,
  [] (promotable* p) {
    print_header = [=] {
      floyd_warshall::vertices = 1024;
      print_prog("floyd_warshall");
      aprintf("inputname onek_vertices\n");
    };
    floyd_warshall::bench_pre(p);
  },
    floyd_warshall::bench_body_serial,
    floyd_warshall::bench_body_interrupt,
    floyd_warshall::bench_post);
  add_benchmark(floyd_warshall::name,
  [] (promotable* p) {
    print_header = [=] {
      floyd_warshall::vertices = 2048;
      print_prog("floyd_warshall");
      aprintf("inputname twok_vertices\n");
    };
    floyd_warshall::bench_pre(p);
  },
    floyd_warshall::bench_body_serial,
    floyd_warshall::bench_body_interrupt,
    floyd_warshall::bench_post);
  // launch scheduler
  aprintf("###start-experiment###\n");
  for (auto scheduler_configuration : scheduler_configurations) {
    for (auto kappa_usec : kappa_usecs) {
      for (auto nb_workers : procs) {
	if ((nb_workers > 1) && (! is_scheduler_configuration_parallel(scheduler_configuration))) {
	  continue;
	}
	switch (scheduler_configuration) {
	case scheduler_configuration_serial: {
	  using microbench_scheduler_type =
	    mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
	  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, kappa_usec, scheduler_configuration);
	  break;
	}
	case scheduler_configuration_interrupt_ping_thread:
	case scheduler_configuration_serial_interrupt_ping_thread:{
	  using microbench_scheduler_type =
	    mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic,
				    ping_thread_worker, ping_thread_interrupt>;
	  launch<microbench_scheduler_type, ping_thread_worker, ping_thread_interrupt>(nb_workers, kappa_usec, scheduler_configuration);
	  break;
	}
	case scheduler_configuration_nopromote_interrupt: {
	  using microbench_scheduler_type =
	    mcsl::minimal_scheduler<stats, logging, mcsl::minimal_elastic, tpal_worker>;
	  launch<microbench_scheduler_type, tpal_worker, mcsl::minimal_interrupt>(nb_workers, kappa_usec, scheduler_configuration);
	  break;
	}
	default: {
	  aprintf("bogus choice for scheduler\n");
	  return;
	}}
	mcsl_worker_notify_exit();
      }
    }
  }
  aprintf("###end-experiment###\n");
}

}

extern "C" {
  void tpal_benchmark_init(int argc, char** argv) {
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
  assert(p != nullptr);
  std::free(p);
}

void operator delete(void * p, std::nothrow_t const&) {
  assert(p != nullptr);
  std::free(p);
}

void* operator new(std::size_t n, std::align_val_t) {
  void * const p = std::malloc(n);
  // handle p == 0
  return p;
}

void operator delete(void* p, unsigned long, std::align_val_t) {
  assert(p != nullptr);
  std::free(p);
}
