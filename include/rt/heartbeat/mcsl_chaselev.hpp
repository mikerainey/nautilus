#ifndef MCSL_CHASELEV_H_
#define MCSL_CHASELEV_H_

#include <memory>
#include <assert.h>
#include <deque>

#include "mcsl_atomic.hpp"
#include "mcsl_perworker.hpp"
#include "mcsl_random.hpp"
#include "mcsl_stats.hpp"
#include "mcsl_snzi.hpp"

typedef void* nk_thread_id_t;

extern "C"
nk_thread_id_t nk_thread_fork(void);

extern "C"
int nk_join_all_children(int (*output_consumer)(void *output));

extern "C"
void nk_sched_reap(int);

extern "C"
void nk_yield();

namespace mcsl {
  
/*---------------------------------------------------------------------*/
/* Chase-Lev Work-Stealing Deque data structure  */

template <typename T>
class chaselev_deque {
private:
    
  class circular_array {
  private:

    cache_aligned_array<std::atomic<T*>> items;

    std::unique_ptr<circular_array> previous;

  public:

    circular_array(std::size_t size) : items(size) { }

    std::size_t size() const {
      return items.size();
    }

    T* get(std::size_t i) {
      return items[i & (size() - 1)].load(std::memory_order_relaxed);
    }

    void put(std::size_t i, T* x) {
      items[i & (size() - 1)].store(x, std::memory_order_relaxed);
    }

    circular_array* grow(std::size_t top, std::size_t bottom) {
      circular_array* new_array = new circular_array(size() * 2);
      new_array->previous.reset(this);
      for (auto i = top; i != bottom; ++i) {
        new_array->put(i, get(i));
      }
      return new_array;
    }

  };

  std::atomic<circular_array*> array;
  
  std::atomic<std::size_t> top, bottom;

public:

  chaselev_deque()
    : array(new circular_array(32)), top(0), bottom(0) { }

  ~chaselev_deque() {
    auto p = array.load(std::memory_order_relaxed);
    if (p) {
      delete p;
    }
  }

  std::size_t size() {
    return (std::size_t)bottom.load() - top.load();
  }

  bool empty() {
    return size() == 0;
  }

  void push(T* x) {
    auto b = bottom.load(std::memory_order_relaxed);
    auto t = top.load(std::memory_order_acquire);
    auto a = array.load(std::memory_order_relaxed);
    if (b - t > a->size() - 1) {
      a = a->grow(t, b);
      array.store(a, std::memory_order_relaxed);
    }
    a->put(b, x);
    std::atomic_thread_fence(std::memory_order_release);
    bottom.store(b + 1, std::memory_order_relaxed);
  }

  T* pop() {
    auto b = bottom.load(std::memory_order_relaxed) - 1;
    auto a = array.load(std::memory_order_relaxed);
    bottom.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto t = top.load(std::memory_order_relaxed);
    if (t <= b) {
      T* x = a->get(b);
      if (t == b) {
        if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
          x = nullptr;
        }
        bottom.store(b + 1, std::memory_order_relaxed);
      }
      return x;
    } else {
      bottom.store(b + 1, std::memory_order_relaxed);
      return nullptr;
    }
  }

  T* steal() {
    auto t = top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto b = bottom.load(std::memory_order_acquire);
    T* x = nullptr;
    if (t < b) {
      auto a = array.load(std::memory_order_relaxed);
      x = a->get(t);
      if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        return nullptr;
      }
    }
    return x;
  }

};

/*---------------------------------------------------------------------*/
/* Work-stealing scheduler  */

using fiber_status_type = enum fiber_status_enum {
  fiber_status_continue,
  fiber_status_pause,
  fiber_status_finish,
  fiber_status_terminate
};

template <typename Scheduler_configuration,
	  template <typename> typename Fiber,
	  typename Stats>
class chase_lev_work_stealing_scheduler {
private:

  using fiber_type = Fiber<Scheduler_configuration>;

  using cl_deque_type = chaselev_deque<fiber_type>;

  using buffer_type = std::deque<fiber_type*>;

  static
  perworker::array<cl_deque_type> deques;

  static
  perworker::array<buffer_type> buffers;

  static
  random::rng_array_type random_number_generators;

  static
  std::size_t random_other_worker(size_t my_id) {
    return random::other_worker(my_id, random_number_generators);
  }

  static
  fiber_type* flush() {
    auto& my_buffer = buffers.mine();
    auto& my_deque = deques.mine();
    fiber_type* current = nullptr;
    if (my_buffer.empty()) {
      return current;
    }
    current = my_buffer.back();
    my_buffer.pop_back();
    while (! my_buffer.empty()) {
      auto f = my_buffer.front();
      my_buffer.pop_front();
      my_deque.push(f);
    }
    assert(current != nullptr);
    return current;
  }

public:

  static
  void commit() {
    auto f = flush();
    if (f != nullptr) {
      deques.mine().push(f);
    }
  }

  static
  void launch(std::size_t nb_workers) {
    perworker::unique_id::initialize(nb_workers);
    perworker::unique_id::initialize_tls_worker(0);
    bool should_terminate = false;
    snzi_termination_detection_barrier<> termination_barrier;
    
    using scheduler_status_type = enum scheduler_status_enum {
      scheduler_status_active,
      scheduler_status_finish
    };

    auto acquire = [&] {
      if (nb_workers == 1) {
        termination_barrier.set_active(false);
        return scheduler_status_finish;
      }
      auto sa = Stats::on_enter_acquire();
      termination_barrier.set_active(false);
      auto my_id = perworker::unique_id::get_my_id();
      fiber_type* current = nullptr;
      while (current == nullptr) {
	nk_yield();
        auto k = random_other_worker(my_id);
        if (! deques[k].empty()) {
          termination_barrier.set_active(true);
          current = deques[k].steal();
          if (current == nullptr) {
            termination_barrier.set_active(false);
          } else {
            Stats::increment(Stats::configuration_type::nb_steals);
          }
        }
        if (termination_barrier.is_terminated() || should_terminate) {
          assert(current == nullptr);
          Stats::on_exit_acquire(sa);
          return scheduler_status_finish;
        }
      }
      assert(current != nullptr);
      buffers.mine().push_back(current);
      Stats::on_exit_acquire(sa);
      return scheduler_status_active;
    };

    auto worker_loop = [&] {
      Scheduler_configuration::initialize_worker();
      auto& my_deque = deques.mine();
      scheduler_status_type status = scheduler_status_active;
      fiber_type* current = nullptr;
      while (status == scheduler_status_active) {
        current = flush();
        while ((current != nullptr) || ! my_deque.empty()) {
          current = (current == nullptr) ? my_deque.pop() : current;
          if (current != nullptr) {
            auto s = current->run();
            if (s == fiber_status_continue) {
              buffers.mine().push_back(current);
            } else if (s == fiber_status_pause) {
              // do nothing
            } else if (s == fiber_status_finish) {
              delete current;
            } else {
              assert(s == fiber_status_terminate);
              status = scheduler_status_finish;
              should_terminate = true;
            }
            current = flush();
          }
        }
        assert((current == nullptr) && my_deque.empty());
        status = acquire();
      }
    };

    for (std::size_t i = 0; i < random_number_generators.size(); ++i) {
      random_number_generators[i] = random::seed(i);
    }

    Scheduler_configuration::initialize_signal_handler();

    termination_barrier.set_active(true);
    nk_thread_id_t t;
    for (std::size_t i = 1; i < nb_workers; i++) {
      t = nk_thread_fork();
      //assert(t != NK_BAD_THREAD_ID);
      if (t == 0) { // child thread
	perworker::unique_id::initialize_tls_worker(i);
	termination_barrier.set_active(true);
	break;
      } else {
	// parent; goes on to fork again
      }
    }
    worker_loop();
    if (nk_join_all_children(0)) {
      assert(false); // there was a problem
    }
    nk_sched_reap(1); // clean up unconditionally
  }

  static
  void schedule(fiber_type* f) {
    assert(f->is_ready());
    buffers.mine().push_back(f);
  }
  
};

template <typename Scheduler_configuration,
	template <typename> typename Fiber,
	typename Stats>
perworker::array<typename chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats>::cl_deque_type> chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats>::deques;

template <typename Scheduler_configuration,
	template <typename> typename Fiber,
	typename Stats>
perworker::array<typename chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats>::buffer_type> chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats>::buffers;

template <typename Scheduler_configuration,
	template <typename> typename Fiber,
	typename Stats>
random::rng_array_type chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats>::random_number_generators;

template <typename Scheduler_configuration,
	  template <typename> typename Fiber,
	  typename Stats>
void schedule(Fiber<Scheduler_configuration>* f) {
  chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats>::schedule(f);  
}

template <typename Scheduler_configuration,
	  template <typename> typename Fiber,
	  typename Stats>
void commit() {
  chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats>::commit();
}

} // end namespace

#endif
