#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>
#include <nautilus/timer.h>
#include <nautilus/nemo.h>
#include <nautilus/cmdline.h>
#include <nautilus/spinlock.h>
#include <nautilus/condvar.h>
#include <test/test.h>
#include <assert.h>
#include <nautilus/rwlock.h>
#include <nautilus/barrier.h>

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

static int
handle_from_cmdline (char * args) {
  return 0;
}

static struct nk_cmdline_impl test_sc_impl = {
    .name    = "scheduler_configuration",
    .handler = handle_from_cmdline,
};
nk_register_cmdline_flag(test_sc_impl);

static struct nk_cmdline_impl test_ku_impl = {
    .name    = "kappa_usec",
    .handler = handle_from_cmdline,
};
nk_register_cmdline_flag(test_ku_impl);

static struct nk_cmdline_impl test_proc_impl = {
    .name    = "proc",
    .handler = handle_from_cmdline,
};
nk_register_cmdline_flag(test_proc_impl);


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

/*---------------------------------------------------------------------*/
/* Support for making the TPAL scheduler reentrant */

struct mcsl_barrier_s {
  int nb_registered;
  int nb_total;
  NK_LOCK_T lock;
  nk_condvar_t condvar;
  struct mcsl_barrier_s* next;
};

typedef struct mcsl_barrier_s mcsl_barrier_t;

void mcsl_barrier_init(mcsl_barrier_t* b, int n, mcsl_barrier_t* next) {
  b->nb_registered = 0;
  b->nb_total = n;
  NK_LOCK_INIT(&(b->lock));
  nk_condvar_init(&(b->condvar));
  b->next = next;
}

void mcsl_barrier_destroy(mcsl_barrier_t* b) {
  NK_LOCK_DEINIT(&(b->lock));
  nk_condvar_destroy(&(b->condvar));
  assert(b->nb_registered == b->nb_total);
  b->nb_registered = 0;
}

void mcsl_barrier_notify(mcsl_barrier_t* b) {
  int last = 0;
  NK_LOCK(&(b->lock));
  int n = ++b->nb_registered;
  if (n == b->nb_total) {
    nk_condvar_signal(&(b->condvar));
    last = 1;
  }
  NK_UNLOCK(&(b->lock));
  if (last && (b->next != NULL)) {
    mcsl_barrier_notify(b->next);
  }
}

void mcsl_barrier_wait(mcsl_barrier_t* b) {
  NK_LOCK(&(b->lock));
  nk_condvar_wait(&(b->condvar), &(b->lock));
  NK_UNLOCK(&(b->lock));
}

mcsl_barrier_t next;
mcsl_barrier_t group;

void mcsl_phase_begin(int nb_workers) {
  mcsl_barrier_init(&next, 1, NULL);
  mcsl_barrier_init(&group, nb_workers, &next);
}

void mcsl_worker0_wait() {
  mcsl_barrier_wait(&next);
}

void mcsl_phase_end() {
  mcsl_barrier_destroy(&group);
  mcsl_barrier_destroy(&next);
}

void mcsl_worker_notify_exit() {
  mcsl_barrier_notify(&group);
}
