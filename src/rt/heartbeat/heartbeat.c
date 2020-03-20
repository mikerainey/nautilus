#include <nautilus/nautilus.h>
//#include <rt/heartbeat/heartbeat.h>
#include <nautilus/thread.h>
#include <assert.h>

#ifndef NAUT_CONFIG_NDPC_RT_DEBUG
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...) DEBUG_PRINT("heartbeat: " fmt, ##args)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("heartbeat: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("heartbeat: " fmt, ##args)

long fib(long n) {
  if (n <= 1) {
    return n;
  } else {
    return fib(n-1) + fib(n-2);
  }
}

void worker(long n) {
  //  INFO("worker\n");
  long r = fib(n);
  //  INFO("n=%ld r=%ld\n",n,r);
}

static void parallel_start_wrapper(void *in, void **out) {
  worker((long)in);
}  

void simple_fj(int nb_workers, long n) {
  for (int i = 1; i < nb_workers; i++) {
    nk_thread_start(parallel_start_wrapper,
                    (void*)n,0,0,TSTACK_DEFAULT,0,-1);
  }
  nk_join_all_children(0);
  //  INFO("completed\n");
}

void microbench();

void nk_heartbeat_init() {
  printk("heartbeat!!!!\n");
  microbench();
  //  simple_fj(4, 20);
  INFO("heartbeat exiting\n");
}

void nk_heartbeat_deinit() {
    INFO("deinited\n");
}

static
nk_tls_key_t unique_id_key;

static
void unique_id_key_free(void* arg) {
  assert(arg != NULL);
  free(arg);
}

void nk_heartbeat_init_unique_id() {
  nk_tls_key_create(&unique_id_key, unique_id_key_free);
}

void nk_heartbeat_set_unique_id(unsigned id) {
  unsigned* p = (unsigned*)malloc(sizeof(unsigned));
  *p = id;
  nk_tls_set(unique_id_key, p);
}

unsigned nk_heartbeat_read_unique_id() {
  return *((unsigned*)nk_tls_get(unique_id_key));
}
