#include <rt/heartbeat/mcsl_atomic.hpp>
#include <rt/heartbeat/mcsl_perworker.hpp>
#include <rt/heartbeat/mcsl_stats.hpp>

#include <rt/heartbeat/mcsl_chaselev.hpp>

mcsl::perworker::array<int> nbs;

extern "C" {
double mcsl_cycles_test() {
  //  mcsl::atomic::aprintf("hi\n");
  mcsl::chaselev_deque<void*> deque;
  nbs.mine() = 123;
  auto s = mcsl::cycles::now();
  mcsl::cycles::spin_for(100000);
  return mcsl::cycles::since(s);  
}
}
