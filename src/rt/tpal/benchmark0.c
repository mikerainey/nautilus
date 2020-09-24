#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>
#include <nautilus/timer.h>
#include <nautilus/nemo.h>
#include <nautilus/cmdline.h>
#include <test/test.h>
#include <assert.h>

#ifndef NAUT_CONFIG_TPAL_RT_DEBUG
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...) DEBUG_PRINT("tpal: " fmt, ##args)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("tpal: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("tpal: " fmt, ##args)

/*---------------------------------------------------------------------*/
/* Command line (grub) */

static
int handle_tpal_benchmark (int argc, char** argv) {
  tpal_bechmark_init(argc, argv);  
  return 0;
}

static struct nk_test_impl test_impl = {
    .name         = "tpal_benchmark",
    .handler      = handle_tpal_benchmark,
    .default_args = "-benchmark incr_array -kappa_usec 100",
};

nk_register_test(test_impl);

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
  if (p == NULL) {
    ERROR("nk_mcsl_set_unique_id\n");
  }
  *p = id;
  nk_tls_set(unique_id_key, p);
}

unsigned nk_mcsl_read_unique_id() {
  unsigned* p = (unsigned*)nk_tls_get(unique_id_key);
  if (p == NULL) {
    ERROR("nk_mcsl_read_unique_id\n");
  }
  return *p;
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
