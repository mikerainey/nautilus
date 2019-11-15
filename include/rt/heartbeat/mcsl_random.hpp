#ifndef MCSL_RANDOM_H_
#define MCSL_RANDOM_H_

#include <random>
#include <assert.h>

#include "mcsl_perworker.hpp"

namespace mcsl {
namespace random {

// from numerical recipes
inline
uint64_t hash64(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

using rng_type = uint64_t;

using rng_array_type = perworker::array<rng_type>;

rng_type seed(int i) {
  return hash64(i);
}

std::size_t other_worker(std::size_t my_id, rng_array_type& rngs) {
  auto nb_workers = perworker::unique_id::get_nb_workers();
  assert(nb_workers != 1);
  auto v = rngs[my_id];
  auto i = (size_t)(v % (nb_workers-1));
  if (i >= my_id) {
    i++;
  }
  rngs[my_id] = hash64(v);
  assert(i >= 0);
  assert(i < nb_workers);
  assert(i != my_id);
  return i;  
}
  
} // end namespace
} // end namespace

#endif
