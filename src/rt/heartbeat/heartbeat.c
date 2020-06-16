#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>
#include <nautilus/timer.h>
#include <nautilus/nemo.h>
#include <assert.h>

void microbench();

#ifndef NAUT_CONFIG_NDPC_RT_DEBUG
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...) DEBUG_PRINT("heartbeat: " fmt, ##args)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("heartbeat: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("heartbeat: " fmt, ##args)

/*---------------------------------------------------------------------*/

struct heartbeat_state_s {
  nk_thread_t * heartbeat_enabled_threads[NAUT_CONFIG_MAX_CPUS]; // array
  struct nk_virtual_console * vc;
};
static
struct heartbeat_state_s heartbeat_state;

nk_timer_t* timer;
volatile
int nb = 2;

void test() {
  INFO("!!test\n");
}

void nemo_test_handler (excp_entry_t * e, void * priv) {
  INFO("test handler\n");

  if (get_cur_thread() == heartbeat_state.heartbeat_enabled_threads[my_cpu_id()]) {
    INFO("on the right thread\n");
    struct nk_regs * r = (struct nk_regs*)((char*)e - 128);
    nk_print_regs(r);
    INFO("rip=%p\n",r->rip);
    r->rip = test;
  } else {
    INFO("not on the right thread\n");
  }
}

void nk_thread_init_fn(void *in, void **out) {
  heartbeat_state.heartbeat_enabled_threads[my_cpu_id()] = get_cur_thread();
  while (nb > 0) {
  }
}

#define TIMER_MS 500UL
#define TIMER_NS (1000000UL*TIMER_MS)

nemo_event_id_t id;
int remote_core = 1;

void timer_callback(void *s) {
  INFO("timer callback\n");

  nk_timer_reset(timer, TIMER_NS);
  nk_timer_start(timer);
  nb--;

  nk_nemo_event_notify(id, remote_core);

  if (nb <= 0) {
    INFO("cancel\n");
    nk_timer_cancel(timer);
  }
}

static
int handle_heartbeat_fib(char *buf, void *priv) {
  heartbeat_state.vc = get_cur_thread()->vc;
  id = nk_nemo_register_event_action(nemo_test_handler, NULL);

  timer = nk_timer_create("mytimer");
  nk_timer_set(timer,TIMER_NS,NK_TIMER_CALLBACK,timer_callback,(void*)get_cur_thread(),0);
  nk_timer_start(timer);
  nk_thread_start(nk_thread_init_fn, (void*)0,0,0,TSTACK_DEFAULT,0,remote_core);
  
  while (nb > 0) {
    //    INFO("wait %d\n",nb);    
  }
  INFO("exit\n");
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
