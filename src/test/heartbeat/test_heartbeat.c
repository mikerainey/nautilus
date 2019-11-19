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

//double mcsl_cycles_test();
void test_launch_incr_array();

int test_heartbeat (void) {
  //  INFO("x=%f", mcsl_cycles_test());
  INFO("Starting simple test of heartbeat\n");
  test_launch_incr_array();
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
