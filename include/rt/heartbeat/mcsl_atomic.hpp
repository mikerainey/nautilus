#pragma once

#include <atomic>
#include <stdarg.h>

#include "mcsl_cycles.hpp"

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

