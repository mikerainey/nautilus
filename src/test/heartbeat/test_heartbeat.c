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


void test_launch_incr_array();

int test_heartbeat (void) {
  nk_vc_printf("test heartbeat\n");
  //  test_threads(); // uncomment this line to run tests for nautilus threads
  //simple_fj(4, 20);
  //test_launch_incr_array();
  return 0;
}

static int
handle_heartbeat_test (char * buf, void * priv)
{
  return test_heartbeat();
}

static struct shell_cmd_impl impl = {
    .cmd      = "heartbeattest",
    .help_str = "heartbeattest",
    .handler  = handle_heartbeat_test,
};
nk_register_shell_cmd(impl);

/* static struct nk_test_impl test_heartbeat_impl = { */
/*     .name         = "heartbeattest", */
/*     .handler      = handle_heartbeat_test, */
/*     .default_args = "", */
/* }; */

/* nk_register_test(test_heartbeat_impl); */
