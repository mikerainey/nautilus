#pragma once

#include <memory>
#include <assert.h>
#include <deque>
#include <functional>

#include "mcsl_stats.hpp"
#include "mcsl_logging.hpp"

/*---------------------------------------------------------------------*/
/* Nautilus compatibility */

typedef uint64_t nk_stack_size_t;
typedef void* nk_thread_id_t;
typedef void (*nk_thread_fun_t)(void * input, void ** output);

extern "C"
int
nk_thread_start (nk_thread_fun_t fun, 
                 void * input,
                 void ** output,
                 uint8_t is_detached,
                 nk_stack_size_t stack_size,
                 nk_thread_id_t * tid,
                 int bound_cpu); // -1 => not bound

extern "C"
void nk_join_all_children(int);

#define TSTACK_DEFAULT 0  // will be 4K

static
void nk_thread_init_fn(void *in, void **out) {
  std::function<void()>* fp = (std::function<void()>*)in;
  (*fp)();
  delete fp;
}

void thread(const std::function<void()>& f) {
  auto fp = new std::function<void()>(f);
  nk_thread_start(nk_thread_init_fn, (void*)fp,0,0,TSTACK_DEFAULT,0,-1);
}

void join_all() {
  nk_join_all_children(0);
}

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

using random_number_seed_type = uint64_t;
  
template <typename Scheduler_configuration,
	  template <typename> typename Fiber,
	  typename Stats, typename Logging>
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
  perworker::array<random_number_seed_type> random_number_generators;

  static
  std::size_t random_other_worker(size_t nb_workers, size_t my_id) {
    assert(nb_workers != 1);
    auto& rn = random_number_generators.mine();
    auto id = (std::size_t)(rn % (nb_workers - 1));
    if (id >= my_id) {
      id++;
    }
    rn = hash(rn);
    return id;
  }

  static
  fiber_type* flush() {
    auto& my_buffer = buffers.mine();
    auto& my_deque = deques.mine();
    fiber_type* current = nullptr;
    if (my_buffer.empty()) {
      return nullptr;
    }
    current = my_buffer.back();
    my_buffer.pop_back();
    while (! my_buffer.empty()) {
      auto f = my_buffer.front();
      my_buffer.pop_front();
      my_deque.push(f);
    }
    assert(my_buffer.empty());
    return current;
  }

  using termination_detection_barrier_type = typename Scheduler_configuration::termination_detection_barrier_type;
  
public:

  static
  void launch(std::size_t nb_workers) {
    bool should_terminate = false;
    termination_detection_barrier_type termination_barrier;
    
    using scheduler_status_type = enum scheduler_status_enum {
      scheduler_status_active,
      scheduler_status_finish
    };

    auto acquire = [&] {
      if (nb_workers == 1) {
        termination_barrier.set_active(false);
        return scheduler_status_finish;
      }
      Logging::log_event(enter_wait);
      auto sa = Stats::on_enter_acquire();
      termination_barrier.set_active(false);
      auto my_id = perworker::unique_id::get_my_id();
      fiber_type* current = nullptr;
      while (current == nullptr) {
        auto k = random_other_worker(nb_workers, my_id);
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
          Logging::log_event(exit_wait);
          return scheduler_status_finish;
        }
      }
      assert(current != nullptr);
      buffers.mine().push_back(current);
      Stats::on_exit_acquire(sa);
      Logging::log_event(exit_wait);
      return scheduler_status_active;
    };

    auto worker_loop = [&] (std::size_t i) {
      Scheduler_configuration::initialize_worker();
      auto& my_deque = deques.mine();
      scheduler_status_type status = scheduler_status_active;
      fiber_type* current = nullptr;
      while (status == scheduler_status_active) {
        current = flush();
        while ((current != nullptr) || ! my_deque.empty()) {
          current = (current == nullptr) ? my_deque.pop() : current;
          if (current != nullptr) {
            auto s = current->exec();
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
      { /*
        std::unique_lock<std::mutex> lk(exit_lock);
        auto nb = ++nb_workers_exited;
        if (perworker::unique_id::get_my_id() == 0) {
          exit_condition_variable.wait(lk, [&] { return nb_workers_exited == nb_workers; });
        } else if (nb == nb_workers) {
          exit_condition_variable.notify_one();
          } */
      }
    };

    for (std::size_t i = 0; i < random_number_generators.size(); ++i) {
      random_number_generators[i] = hash(i);
    }

    Scheduler_configuration::initialize_signal_handler();

    termination_barrier.set_active(true);
    for (std::size_t i = 1; i < nb_workers; i++) {
      thread([&] {
        perworker::unique_id::initialize_tls_worker(i);
        termination_barrier.set_active(true);
        worker_loop(i);
      });
      //pthreads[i] = t.native_handle();
      //t.detach();
    }
    //pthreads[0] = pthread_self();
    Logging::log_event(enter_algo);
    worker_loop(0);
    Logging::log_event(exit_algo);
  }

  static
  fiber_type* take() {
    auto& my_buffer = buffers.mine();
    auto& my_deque = deques.mine();
    fiber_type* current = nullptr;
    assert(my_buffer.empty());
    current = my_deque.pop();
    if (current != nullptr) {
      my_buffer.push_back(current);
    }
    return current;
  }
  
  static
  void schedule(fiber_type* f) {
    assert(f->is_ready());
    buffers.mine().push_back(f);
  }

  static
  void commit() {
    auto f = flush();
    if (f != nullptr) {
      deques.mine().push(f);
    }
  }

};

template <typename Scheduler_configuration,
          template <typename> typename Fiber,
          typename Stats, typename Logging>
perworker::array<typename chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::cl_deque_type> chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::deques;

template <typename Scheduler_configuration,
          template <typename> typename Fiber,
          typename Stats, typename Logging>
perworker::array<typename chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::buffer_type> chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::buffers;

template <typename Scheduler_configuration,
          template <typename> typename Fiber,
          typename Stats, typename Logging>
perworker::array<random_number_seed_type> chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::random_number_generators;

template <typename Scheduler_configuration,
	  template <typename> typename Fiber,
	  typename Stats, typename Logging>
Fiber<Scheduler_configuration>* take() {
  return chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::take();  
}

template <typename Scheduler_configuration,
	  template <typename> typename Fiber,
	  typename Stats, typename Logging>
void schedule(Fiber<Scheduler_configuration>* f) {
  chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::schedule(f);  
}

template <typename Scheduler_configuration,
	  template <typename> typename Fiber,
	  typename Stats, typename Logging>
void commit() {
  chase_lev_work_stealing_scheduler<Scheduler_configuration,Fiber,Stats,Logging>::commit();
}
  
} // end namespace
