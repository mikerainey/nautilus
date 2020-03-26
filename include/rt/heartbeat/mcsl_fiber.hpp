#pragma once

#include "mcsl_chaselev.hpp"

namespace mcsl {

/*---------------------------------------------------------------------*/
/* Interface for promotable fibers */

class promotable {
public:

  virtual
  void async_finish_promote(std::function<void(promotable*)>) = 0;

  virtual
  void fork_join_promote(std::function<void(promotable*)>,
			 std::function<void(promotable*)>) = 0;

};

/*---------------------------------------------------------------------*/
/* Fibers */

template <typename Scheduler_configuration>
class fiber : public promotable {
private:

  std::atomic<std::size_t> incounter;

  fiber* outedge;

  std::function<void(promotable*)> body;
  
public:

  using stats = typename Scheduler_configuration::stats;
  
  using logging = typename Scheduler_configuration::logging;

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
  mcsl::fiber_status_type exec() {
    body(this);
    return mcsl::fiber_status_finish;
  }

  bool is_ready() {
    return incounter.load() == 0;
  }
  
  void release() {
    if (--incounter == 0) {
      mcsl::schedule<Scheduler_configuration, fiber, stats, logging>(this);
    }
  }

  fiber* capture_continuation() {
    auto oe = outedge;
    outedge = nullptr;
    return oe;
  }

  static
  void add_edge(fiber* src, fiber* dst) {
    assert(src != nullptr); assert(dst != nullptr);
    assert(src->outedge == nullptr);
    src->outedge = dst;
    dst->incounter++;
  }

  virtual
  void notify() {
    assert(is_ready());
    auto fo = outedge;
    outedge = nullptr;
    if (fo != nullptr) {
      fo->release();
    }
  }

  virtual
  void finish() {
    notify();
    delete this;
  }

  void async_finish_promote(std::function<void(promotable*)> body) {
    auto b = new fiber(body);
    add_edge(b, outedge);
    b->release();
    mcsl::commit<Scheduler_configuration, fiber, stats, logging>();
    stats::increment(stats::configuration_type::nb_promotions);
  }

  void fork_join_promote(std::function<void(promotable*)> body,
			 std::function<void(promotable*)> combine) {
    auto b = new fiber(body);
    auto c = new fiber(combine);
    c->outedge = capture_continuation();
    add_edge(this, c);
    add_edge(b, c);
    b->release();
    c->release();
    mcsl::commit<Scheduler_configuration, fiber, stats, logging>();
    stats::increment(stats::configuration_type::nb_promotions);
  }

};
  
} // end namespace
