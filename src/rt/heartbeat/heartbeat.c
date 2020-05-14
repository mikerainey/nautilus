#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>
#include <assert.h>

#ifndef NAUT_CONFIG_NDPC_RT_DEBUG
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...) DEBUG_PRINT("heartbeat: " fmt, ##args)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("heartbeat: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("heartbeat: " fmt, ##args)

/*---------------------------------------------------------------------*/

void microbench();

static
int handle_heartbeat_fib(char *buf, void *priv) {
  microbench();
  return 0;
}

static
struct shell_cmd_impl heartbeat_fib = {
  .cmd      = "heartbeat_fib",
  .help_str = "heartbeat_fib",
  .handler  = handle_heartbeat_fib,
};

nk_register_shell_cmd(heartbeat_fib);

/*---------------------------------------------------------------------*/
/* Registry of nautilus thread-local storage for heartbeat runtime */

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

/*---------------------------------------------------------------------*/
/* Heartbeat runtime init (called at boot time by init.c) */
// for now, we won't be using this mechanism, but instead will use the shell registry, above

void nk_heartbeat_init() {

}

void nk_heartbeat_deinit() { 

}
