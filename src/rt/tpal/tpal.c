#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>
#include <nautilus/timer.h>
#include <nautilus/nemo.h>
#include <assert.h>

#ifndef NAUT_CONFIG_TPAL_RT_DEBUG
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...) DEBUG_PRINT("tpal: " fmt, ##args)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("tpal: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("tpal: " fmt, ##args)

// Interrupt

void handle_incr_array_interrupt(char *buf, void *priv);

static
struct shell_cmd_impl tpal_incr_array_interrupt = {
  .cmd      = "incr_array_interrupt",
  .help_str = "",
  .handler  = handle_incr_array_interrupt,
};

nk_register_shell_cmd(tpal_incr_array_interrupt);

// Manual

void handle_incr_array_manual(char *buf, void *priv);

static
struct shell_cmd_impl tpal_incr_array_manual = {
  .cmd      = "incr_array_manual",
  .help_str = "",
  .handler  = handle_incr_array_manual,
};

nk_register_shell_cmd(tpal_incr_array_manual);

// Software polling

void handle_incr_array_software_polling(char *buf, void *priv);

static
struct shell_cmd_impl tpal_incr_array_software_polling = {
  .cmd      = "incr_array_software_polling",
  .help_str = "",
  .handler  = handle_incr_array_software_polling,
};

nk_register_shell_cmd(tpal_incr_array_software_polling);

// Serial

void handle_incr_array_serial(char *buf, void *priv);

static
struct shell_cmd_impl tpal_incr_array_serial = {
  .cmd      = "incr_array_serial",
  .help_str = "",
  .handler  = handle_incr_array_serial,
};

nk_register_shell_cmd(tpal_incr_array_serial);

/*---------------------------------------------------------------------*/
/* Registry of nautilus thread-local storage for the mcsl runtime */

static
nk_tls_key_t unique_id_key;

static
void unique_id_key_free(void* arg) {
  assert(arg != NULL);
  free(arg);
}

void nk_mcsl_init_unique_id() {
  nk_tls_key_create(&unique_id_key, unique_id_key_free);
}

void nk_mcsl_set_unique_id(unsigned id) {
  unsigned* p = (unsigned*)malloc(sizeof(unsigned));
  *p = id;
  nk_tls_set(unique_id_key, p);
}

unsigned nk_mcsl_read_unique_id() {
  return *((unsigned*)nk_tls_get(unique_id_key));
}

/*---------------------------------------------------------------------*/
/* TPAL runtime init (called at boot time by init.c) */
// for now, we won't be using this mechanism, but instead will use the shell registry, above

int naut_my_cpu_id() {
  return my_cpu_id();
}

nk_thread_t* naut_get_cur_thread() {
  return get_cur_thread();
}

void nk_tpal_init() {

}

void nk_tpal_deinit() { 

}
