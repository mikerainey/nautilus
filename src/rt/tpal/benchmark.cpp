#include <limits.h>
#include <functional>
#include <vector>
#include <cstring>

#include "benchmark_nautilus.hpp"

//#include "incr_array.hpp"

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

void benchmark_init(int argc, char** argv) {
  uint64_t kappa_usec = tpalrts::dflt_kappa_usec;
  printk("argc=%d\n",argc);
  for (int i = 0; i < argc; i++) {
    if (argv[i] == "-kappa_usec") {
      printk("hi\n");
      if ((i + 1) < argc) {
	kappa_usec = atoi(argv[i + 1]);
      }
    }
  }
  printk("ka=%lu\n",kappa_usec);
}

extern "C" {
  void tpal_bechmark_init(int argc, char** argv) {
    benchmark_init(argc, argv);
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
