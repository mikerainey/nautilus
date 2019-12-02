#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/shutdown.h>
#include <nautilus/cmdline.h>
#include <rt/heartbeat/heartbeat.h>
#include <test/test.h>

#ifndef NAUT_CONFIG_DEBUG_TESTS
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define DEBUG(fmt, args...) DEBUG_PRINT("TEST: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("TEST: " fmt, ##args)
#define WARN(fmt, args...)  WARN_PRINT("TEST: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("TEST: " fmt, ##args)

long fib(long n) {
  if (n < 0) {
    return n;
  } else {
    return fib(n-1) + fib(n-2);
  }
}

void worker(long n) {
  long r = fib(n);
  //INFO("r=%d\n",r);
}

void simple_fj(int nb_workers, long n) {
  nk_thread_id_t t;
  for (int i = 1; i < nb_workers; i++) {
    t = nk_thread_fork();    
    if (t == 0) { // child thread
      worker(n);
      nk_thread_exit(0);
      return;
    } else {
      // parent; goes on to fork again
    }
  }
  worker(n);
  nk_join_all_children(0);
  INFO("completed\n");
}

void test_launch_incr_array();

int test_heartbeat (void) {
  INFO("Starting simple test of heartbeat\n");
  //  test_threads(); // uncomment this line to run tests for nautilus threads
  simple_fj(4, 20);
  //test_launch_incr_array();
  return 0;
}

static int
handle_heartbeat_test (char * buf, void * priv)
{
  return test_heartbeat();
}

static struct nk_test_impl test_heartbeat_impl = {
    .name         = "heartbeattest",
    .handler      = handle_heartbeat_test,
    .default_args = "",
};

nk_register_test(test_heartbeat_impl);
