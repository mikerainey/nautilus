#include <functional>

#include <rt/heartbeat/mcsl.hpp>

/*---------------------------------------------------------------------*/
/* Scheduler configuration */

uint64_t kappa_usec;

uint64_t kappa_cycles;

using heartbeat_mechanism_type = enum heartbeat_mechanism_type {
  heartbeat_mechanism_software_polling,
  heartbeat_mechanism_interrupt,
  heartbeat_mechanism_interrupt_pthread,
  heartbeat_mechanism_interrupt_papi,
  heartbeat_mechanism_noop,
};

/*---------------------------------------------------------------------*/
/* Fiber (aka green thread) */

class promotable {
public:

  virtual
  void promote(std::function<void(promotable*)>) = 0;
  
};

template <typename Scheduler_configuration>
class fiber : public promotable {
private:

  std::atomic<std::size_t> incounter;

  fiber* outedge;

  std::function<void(promotable*)> body;
  
public:

  fiber(std::function<void(promotable*)> body)
    : body(body), incounter(1), outedge(nullptr) { }

  ~fiber() {
    assert(is_ready());
    auto f = outedge;
    outedge = nullptr;
    if (f != nullptr) {
      f->release();
    }
  }

  virtual
  mcsl::fiber_status_type run() {
    body(this);
    return mcsl::fiber_status_finish;
  }

  bool is_ready() {
    return incounter.load() == 0;
  }

  void release() {
    if (--incounter == 0) {
      mcsl::schedule<Scheduler_configuration, fiber, stats>(this);
    }
  }

  static
  void add_edge(fiber* src, fiber* dst) {
    assert(src->outedge == nullptr);
    src->outedge = dst;
    dst->incounter++;
  }

  void promote(std::function<void(promotable*)> body) {
    auto b = new fiber(body);
    add_edge(b, outedge);
    b->release();
    mcsl::commit<Scheduler_configuration, fiber, stats>();
    stats::increment(stats_configuration::nb_promotions);
  }

};

/*---------------------------------------------------------------------*/
/* Scheduler launch */

double end_time, start_time;

static
void report_exectime() {
  printf("exectime %.3f\n", end_time - start_time);
}

template <typename Scheduler_configuration,
          typename Bench_pre, typename Bench_post, typename Fiber_body>
void launch(std::size_t nb_workers,
	     const Bench_pre& bench_pre,
	     const Bench_post& bench_post,
	     Fiber_body f_body) {
  bench_pre();
  {
    auto f_cont = new fiber<Scheduler_configuration>([=] (promotable*) {
      end_time = mcsl::getclock();
    });
    fiber<Scheduler_configuration>::add_edge(f_body, f_cont);
    start_time = mcsl::getclock();
    f_cont->release();
    f_body->release();
  }
  using scheduler_type = mcsl::chase_lev_work_stealing_scheduler<Scheduler_configuration, fiber, stats>;
  stats::on_enter_launch();
  scheduler_type::launch(nb_workers);
  stats::on_exit_launch();
  bench_post();
  report_exectime();
  stats::report();
}

/*---------------------------------------------------------------------*/
/* Increment-array microbenchmark */

static
int64_t* incr_array_serial(int64_t* a, int64_t lo, int64_t hi) {
  for (int64_t i = lo; i != hi; i++) {
    a[i]++;
  }
  return a;
}

static
int64_t incr_array_manual_T = 8096;

template <typename Scheduler_configuration>
class incr_array_manual : public fiber<Scheduler_configuration> {
public:

  using status_type = enum { entry, exit };

  status_type status = entry;
  
  int64_t* a; int64_t lo; int64_t hi;

  incr_array_manual(int64_t* a, int64_t lo, int64_t hi)
    : fiber<Scheduler_configuration>([] (promotable*) { return mcsl::fiber_status_finish; }),
      a(a), lo(lo), hi(hi)
  { }

    mcsl::fiber_status_type run() {
      switch (status) {
      case entry: {
	if (hi-lo <= incr_array_manual_T) {
	  incr_array_serial(a, lo, hi);
	} else {
	  int64_t mid = (lo+hi)/2;
	  auto f1 = new incr_array_manual(a, lo, mid);
	  auto f2 = new incr_array_manual(a, mid, hi);
	  fiber<Scheduler_configuration>::add_edge(f1, this);
	  fiber<Scheduler_configuration>::add_edge(f2, this);
	  f1->release();
	  f2->release();
	  stats::increment(stats_configuration::nb_promotions);
	  stats::increment(stats_configuration::nb_promotions);
	  status = exit;
	  return mcsl::fiber_status_pause;	  
	}
	break;
      }
      case exit: {
	break;
      }
      }
      return mcsl::fiber_status_finish;
    }

};

/* ------------------------------------------------------- */
/* Scheduler configuration #0: don't deliver interrupts,
 * thereby precluding any parallelism
 */

class noop_scheduler_configuration {
public:
  
  static constexpr
  heartbeat_mechanism_type heartbeat_mechanism = heartbeat_mechanism_noop;

  static
  void initialize_worker() {
  }  

  static
  void initialize_signal_handler() {
  }

};

void launch_incr_array(uint64_t nb_items, uint64_t nb_workers) {
  int64_t* a = (int64_t*)malloc(sizeof(int64_t)*nb_items);
  auto bench_pre = [=] {
    for (int64_t i = 0; i < nb_items; i++) {
      a[i] = 0;
    }
  };
  auto bench_post = [=]   {
    int64_t m = 0;
    for (int64_t i = 0; i < nb_items; i++) {
      m += a[i];
    }
    assert(m == nb_items);
    free(a);
  };
  auto f = new incr_array_manual<software_polling_scheduler_configuration>(a, 0, nb_items);
  launch<software_polling_scheduler_configuration, decltype(bench_pre), decltype(bench_post), decltype(f)>(nb_workers, bench_pre, bench_post, f);

}
