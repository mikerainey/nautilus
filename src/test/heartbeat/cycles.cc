#include <rt/heartbeat/mcsl_chaselev.hpp>

mcsl::perworker::array<int> nbs;
#include <deque>
#include <functional>

namespace std {
    void __throw_bad_function_call() {}
}

extern "C" {
double mcsl_cycles_test() {
  //  mcsl::atomic::aprintf("hi\n");
  //  mcsl::chaselev_deque<void*> deque;
  //  deque.push(nullptr);
  std::deque<int> dq;
  dq.push_back(333);
  std::function<int(int)> f = [] (int x) { return x + 1; };
  //  nbs.mine() = 123;
  auto s = mcsl::cycles::now();
  mcsl::cycles::spin_for(100000);
  return mcsl::cycles::since(s) + (double)f((int)s);  
}
}
