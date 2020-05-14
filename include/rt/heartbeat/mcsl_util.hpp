#pragma once

#include <cstdint>
#include <atomic>
#include <stdarg.h>
#include <assert.h>

extern "C"
int printk(const char* fmt, ...);

extern "C"
void** nk_get_tid();

typedef long time_t;

typedef int clockid_t;


extern "C"
int clock_gettime(clockid_t, struct timespec*);

namespace nautilus {
#include <nautilus/spinlock.h>
}

namespace mcsl {

/*---------------------------------------------------------------------*/
/* Hash function */

uint64_t hash(uint64_t x) {
  x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
  x = x ^ (x >> 31);
  return x;
}

/*---------------------------------------------------------------------*/
/* Cycle counter */

namespace cycles {

namespace {
  
static inline
uint64_t rdtsc() {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return  ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

static inline
void rdtsc_wait(uint64_t n) {
  const uint64_t start = rdtsc();
  while (rdtsc() < (start + n)) {
    __asm__("PAUSE");
  }
}
  
} // end namespace
  
static inline
uint64_t diff(uint64_t start, uint64_t finish) {
  return finish - start;
}

static inline
uint64_t now() {
  return rdtsc();
}

static inline
uint64_t since(uint64_t start) {
  return diff(start, now());
}

static inline
void spin_for(uint64_t nb_cycles) {
  rdtsc_wait(nb_cycles);
}
  
} // end namespace

/*---------------------------------------------------------------------*/
/* System clock */

namespace clock {

using time_point_type = struct timespec;
  
static inline
double diff(time_point_type start, time_point_type finish) {
  long seconds = finish.tv_sec - start.tv_sec; 
  long ns = finish.tv_nsec - start.tv_nsec; 
  if (start.tv_nsec > finish.tv_nsec) { // clock underflow 
    --seconds; 
    ns += 1000000000; 
  }
  return (double)seconds + (double)ns/(double)1000000000;
}

static inline
time_point_type now() {
  struct timespec te;
  clock_gettime(CLOCK_MONOTONIC, &te);
  return te;
}

static inline
double since(time_point_type start) {
  return diff(start, now());
}
  
} // end namespace
  
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

/*---------------------------------------------------------------------*/
/* Atomic printing routines */
  
nautilus::spinlock_t print_lock;
  
void init_print_lock() {
  nautilus::spinlock_init(&print_lock);
}

void acquire_print_lock() {
  nautilus::spin_lock(&print_lock);
}

void release_print_lock() {
  nautilus::spin_unlock(&print_lock);
}

void die(const char *fmt, ...) {
  va_list	ap;
  va_start (ap, fmt);
  acquire_print_lock(); {
    printk(fmt, ap);
  }
  release_print_lock();
  va_end(ap);
  assert(false);
}

void afprintf(FILE* stream, const char *fmt, ...) {
  die("afprintf: todo\n");
  va_list	ap;
  va_start (ap, fmt);
  acquire_print_lock(); {
    // later: output to file
  }
  release_print_lock();
  va_end(ap);
}

void aprintf(const char *fmt, ...) {
  va_list	ap;
  va_start (ap, fmt);
  acquire_print_lock(); {
    printk(fmt, ap);
  }
  release_print_lock();
  va_end(ap);
}

} // end namespace
