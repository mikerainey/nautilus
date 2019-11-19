#ifndef MCSL_STATS_H_
#define MCSL_STATS_H_

#include "mcsl_perworker.hpp"

extern "C"
uint64_t nk_sched_get_realtime();

static inline
double getclock() {
  uint64_t ns = nk_sched_get_realtime();
  return (double)ns/1e9;
}

namespace mcsl {

template <typename Configuration>
class stats_base {
public:

  using counter_id_type = typename Configuration::counter_id_type;
  
  using time_point_type = double;

  using configuration_type = Configuration;
  
private:

  using private_counters = struct {
    long counters[Configuration::nb_counters];
  };

  static
  perworker::array<private_counters> all_counters;

  static
  time_point_type enter_launch_time;
  
  static
  double launch_duration;
  
  static
  perworker::array<double> all_total_idle_time;
  
  static
  double since(time_point_type start) {
    return getclock() - start;
  }

public:

  static inline
  void increment(counter_id_type id) {
    if (! Configuration::enabled) {
      return;
    }
    all_counters.mine().counters[id]++;
  }

  static
  void on_enter_launch() {
    enter_launch_time = getclock();
  }
  
  static
  void on_exit_launch() {
    launch_duration = since(enter_launch_time);
  }

  static
  time_point_type on_enter_acquire() {
    if (! Configuration::enabled) {
      return time_point_type();
    }
    return getclock();
  }
  
  static
  void on_exit_acquire(time_point_type enter_acquire_time) {
    if (! Configuration::enabled) {
      return;
    }
    all_total_idle_time.mine() += since(enter_acquire_time);
  }

  static
  void report() {
    if (! Configuration::enabled) {
      return;
    }
    auto nb_workers = perworker::unique_id::get_nb_workers();
    for (int counter_id = 0; counter_id < Configuration::nb_counters; counter_id++) {
      long counter_value = 0;
      for (std::size_t i = 0; i < nb_workers; ++i) {
        counter_value += all_counters[i].counters[counter_id];
      }
      const char* counter_name = Configuration::name_of_counter((counter_id_type)counter_id);
      if (counter_name == "nb_promotions") {
	HEARTBEAT_DEBUG("nb_promotions %d\n", counter_value);
      } else if (counter_name == "nb_steals") {
	HEARTBEAT_DEBUG("nb_steals %d\n", counter_value);
      }
      //      std::cout << counter_name << " " << counter_value << std::endl;
    }
    HEARTBEAT_DEBUG("launch_duration %f\n", launch_duration);
    //    std::cout << "launch_duration " << launch_duration << std::endl;
    double cumulated_time = launch_duration * nb_workers;
    double total_idle_time = 0.0;
    for (std::size_t i = 0; i < nb_workers; ++i) {
      total_idle_time += all_total_idle_time[i];
    }
    double relative_idle = total_idle_time / cumulated_time;
    double utilization = 1.0 - relative_idle;
    HEARTBEAT_DEBUG("total_idle_time %f\n", total_idle_time);
    HEARTBEAT_DEBUG("utilization %f\n", utilization);
    //    std::cout << "total_idle_time " << total_idle_time << std::endl;
    //    std::cout << "utilization " << utilization << std::endl;
  }

};

template <typename Configuration>
perworker::array<typename stats_base<Configuration>::private_counters> stats_base<Configuration>::all_counters;

template <typename Configuration>
typename stats_base<Configuration>::time_point_type stats_base<Configuration>::enter_launch_time;

template <typename Configuration>
double stats_base<Configuration>::launch_duration;

template <typename Configuration>
perworker::array<double> stats_base<Configuration>::all_total_idle_time;

} // end namespace

#endif
