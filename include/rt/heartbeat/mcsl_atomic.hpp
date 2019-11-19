#ifndef MCSL_ATOMIC_H_
#define MCSL_ATOMIC_H_

#include <atomic>
#include <stdarg.h>

#include "mcsl_cycles.hpp"

extern "C" int printk(const char* fmt, ...);
extern "C" void** nk_get_tid();
#define HEARTBEAT_DEBUG(fmt, args...) printk("(%x) HEARTBEAT %s:%s:%d: " fmt, nk_get_tid(),  __FILE__, __func__, __LINE__, ##args)

namespace mcsl {
namespace atomic {

/*---------------------------------------------------------------------*/
/* Atomic compare-and-exchange operation, with backoff */

template <class T>
bool compare_exchange(std::atomic<T>& cell, T& expected, T desired) {
  static constexpr
  int backoff_nb_cycles = 1l << 12;
  if (cell.compare_exchange_strong(expected, desired)) {
    return true;
  }
  cycles::spin_for(backoff_nb_cycles);
  return false;
}

} // end namespace
} // end namespace

#endif
