#pragma once

#include <atomic>

#include "mcsl_aligned.hpp"

#ifndef MCSL_MAX_NB_WORKERS_LG
#define MCSL_MAX_NB_WORKERS_LG 7
#endif

extern "C"
void nk_heartbeat_init_unique_id();

extern "C"
void nk_heartbeat_set_unique_id(unsigned id);

extern "C"
unsigned nk_heartbeat_read_unique_id();

namespace mcsl {
namespace perworker {

/*---------------------------------------------------------------------*/
/* Static threshold to upper bound the number of worker threads */

static constexpr
int default_max_nb_workers_lg = MCSL_MAX_NB_WORKERS_LG;

static constexpr
int default_max_nb_workers = 1 << default_max_nb_workers_lg;

#undef MCSL_MAX_NB_WORKERS_LG

/*---------------------------------------------------------------------*/
/* Per-worker, unique id */

class unique_id {
private:

  static
  std::size_t nb_workers;

public:

  static
  void initialize(std::size_t _nb_workers) {
    nb_workers = _nb_workers;
    nk_heartbeat_init_unique_id();
  }

  static
  void initialize_tls_worker(std::size_t id) {
    nk_heartbeat_set_unique_id((unsigned)id);
  }
  
  static
  std::size_t get_my_id() {
    return nk_heartbeat_read_unique_id();
  }

  static
  std::size_t get_nb_workers() {
    return nb_workers;
  }

};

std::size_t unique_id::nb_workers = 0;

/*---------------------------------------------------------------------*/
/* Per-worker array */

template <typename Item, std::size_t capacity=default_max_nb_workers>
class array {
private:

  cache_aligned_fixed_capacity_array<Item, capacity> items;

public:

  array() {
    for (std::size_t i = 0; i < items.size(); ++i) {
      new (&items[i]) value_type;
    }
  }

  ~array() {
    for (std::size_t i = 0; i < items.size(); ++i) {
      items[i].~value_type();
    }
  }

  using value_type = Item;
  using reference = Item&;
  using iterator = value_type*;    

  iterator begin() {
    return items.begin();
  }

  iterator end() {
    return items.end();
  }

  reference operator[](std::size_t i) {
    return items[i];
  }

  reference mine() {
    return items[unique_id::get_my_id()];
  }

  std::size_t size() const {
    return capacity;
  }
  
};
  
} // end namespace
} // end namespace