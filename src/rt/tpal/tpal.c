#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>
#include <nautilus/timer.h>
#include <nautilus/nemo.h>
#include <nautilus/cmdline.h>
#include <assert.h>

#ifndef NAUT_CONFIG_TPAL_RT_DEBUG
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...) DEBUG_PRINT("tpal: " fmt, ##args)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("tpal: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("tpal: " fmt, ##args)

/*---------------------------------------------------------------------*/
/* Control the setting of kappa */

void handle_set_kappa_20(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_kappa_20 = {
  .cmd      = "set_kappa_20",
  .help_str = "",
  .handler  = handle_set_kappa_20,
};

nk_register_shell_cmd(tpal_set_kappa_20);

void handle_set_kappa_40(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_kappa_40 = {
  .cmd      = "set_kappa_40",
  .help_str = "",
  .handler  = handle_set_kappa_40,
};

nk_register_shell_cmd(tpal_set_kappa_40);

void handle_set_kappa_100(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_kappa_100 = {
  .cmd      = "set_kappa_100",
  .help_str = "",
  .handler  = handle_set_kappa_100,
};

nk_register_shell_cmd(tpal_set_kappa_100);

void handle_set_kappa_400(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_kappa_400 = {
  .cmd      = "set_kappa_400",
  .help_str = "",
  .handler  = handle_set_kappa_400,
};

nk_register_shell_cmd(tpal_set_kappa_400);

void handle_set_kappa_40000(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_kappa_40000 = {
  .cmd      = "set_kappa_40000",
  .help_str = "",
  .handler  = handle_set_kappa_40000,
};

nk_register_shell_cmd(tpal_set_kappa_40000);

/*---------------------------------------------------------------------*/
/* Control the number of work threads */

void handle_set_nb_workers_1(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_nb_workers_1 = {
  .cmd      = "set_nb_workers_1",
  .help_str = "",
  .handler  = handle_set_nb_workers_1,
};

nk_register_shell_cmd(tpal_set_nb_workers_1);

void handle_set_nb_workers_3(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_nb_workers_3 = {
  .cmd      = "set_nb_workers_3",
  .help_str = "",
  .handler  = handle_set_nb_workers_3,
};

nk_register_shell_cmd(tpal_set_nb_workers_3);

void handle_set_nb_workers_7(char *buf, void *priv);

static
struct shell_cmd_impl tpal_set_nb_workers_7 = {
  .cmd      = "set_nb_workers_7",
  .help_str = "",
  .handler  = handle_set_nb_workers_7,
};

nk_register_shell_cmd(tpal_set_nb_workers_7);


/*---------------------------------------------------------------------*/
/* Benchmark registry for incr_array */

// Interrupt

void handle_incr_array_interrupt(char *buf, void *priv);

static
struct shell_cmd_impl tpal_incr_array_interrupt = {
  .cmd      = "incr_array_interrupt",
  .help_str = "",
  .handler  = handle_incr_array_interrupt,
};

nk_register_shell_cmd(tpal_incr_array_interrupt);

// Interrupt no promotion

void handle_incr_array_interrupt_nopromote(char *buf, void *priv);

static
struct shell_cmd_impl tpal_incr_array_interrupt_nopromote = {
  .cmd      = "incr_array_interrupt_nopromote",
  .help_str = "",
  .handler  = handle_incr_array_interrupt_nopromote,
};

nk_register_shell_cmd(tpal_incr_array_interrupt_nopromote);

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

// All

void handle_incr_array(char *buf, void *priv);

static
struct shell_cmd_impl tpal_incr_array = {
  .cmd      = "incr_array",
  .help_str = "",
  .handler  = handle_incr_array,
};

nk_register_shell_cmd(tpal_incr_array);

/*---------------------------------------------------------------------*/
/* Benchmark registry for plus_reduce_array */

// Interrupt

void handle_plus_reduce_array_interrupt(char *buf, void *priv);

static
struct shell_cmd_impl tpal_plus_reduce_array_interrupt = {
  .cmd      = "plus_reduce_array_interrupt",
  .help_str = "",
  .handler  = handle_plus_reduce_array_interrupt,
};

nk_register_shell_cmd(tpal_plus_reduce_array_interrupt);

// Interrupt no promotion

void handle_plus_reduce_array_interrupt_nopromote(char *buf, void *priv);

static
struct shell_cmd_impl tpal_plus_reduce_array_interrupt_nopromote = {
  .cmd      = "plus_reduce_array_interrupt_nopromote",
  .help_str = "",
  .handler  = handle_plus_reduce_array_interrupt_nopromote,
};

nk_register_shell_cmd(tpal_plus_reduce_array_interrupt_nopromote);

// Manual

void handle_plus_reduce_array_manual(char *buf, void *priv);

static
struct shell_cmd_impl tpal_plus_reduce_array_manual = {
  .cmd      = "plus_reduce_array_manual",
  .help_str = "",
  .handler  = handle_plus_reduce_array_manual,
};

nk_register_shell_cmd(tpal_plus_reduce_array_manual);

// Software polling

void handle_plus_reduce_array_software_polling(char *buf, void *priv);

static
struct shell_cmd_impl tpal_plus_reduce_array_software_polling = {
  .cmd      = "plus_reduce_array_software_polling",
  .help_str = "",
  .handler  = handle_plus_reduce_array_software_polling,
};

nk_register_shell_cmd(tpal_plus_reduce_array_software_polling);

// Serial

void handle_plus_reduce_array_serial(char *buf, void *priv);

static
struct shell_cmd_impl tpal_plus_reduce_array_serial = {
  .cmd      = "plus_reduce_array_serial",
  .help_str = "",
  .handler  = handle_plus_reduce_array_serial,
};

nk_register_shell_cmd(tpal_plus_reduce_array_serial);

// All

void handle_plus_reduce_array(char *buf, void *priv);

static
struct shell_cmd_impl tpal_plus_reduce_array = {
  .cmd      = "plus_reduce_array",
  .help_str = "",
  .handler  = handle_plus_reduce_array,
};

nk_register_shell_cmd(tpal_plus_reduce_array);

/*---------------------------------------------------------------------*/
/* Benchmark registry for spmv */

// Interrupt

void handle_spmv_interrupt(char *buf, void *priv);

static
struct shell_cmd_impl tpal_spmv_interrupt = {
  .cmd      = "spmv_interrupt",
  .help_str = "",
  .handler  = handle_spmv_interrupt,
};

nk_register_shell_cmd(tpal_spmv_interrupt);

// Interrupt no promotion

void handle_spmv_interrupt_nopromote(char *buf, void *priv);

static
struct shell_cmd_impl tpal_spmv_interrupt_nopromote = {
  .cmd      = "spmv_interrupt_nopromote",
  .help_str = "",
  .handler  = handle_spmv_interrupt_nopromote,
};

nk_register_shell_cmd(tpal_spmv_interrupt_nopromote);

// Manual

void handle_spmv_manual(char *buf, void *priv);

static
struct shell_cmd_impl tpal_spmv_manual = {
  .cmd      = "spmv_manual",
  .help_str = "",
  .handler  = handle_spmv_manual,
};

nk_register_shell_cmd(tpal_spmv_manual);

// Software polling

void handle_spmv_software_polling(char *buf, void *priv);

static
struct shell_cmd_impl tpal_spmv_software_polling = {
  .cmd      = "spmv_software_polling",
  .help_str = "",
  .handler  = handle_spmv_software_polling,
};

nk_register_shell_cmd(tpal_spmv_software_polling);

// Serial

void handle_spmv_serial(char *buf, void *priv);

static
struct shell_cmd_impl tpal_spmv_serial = {
  .cmd      = "spmv_serial",
  .help_str = "",
  .handler  = handle_spmv_serial,
};

nk_register_shell_cmd(tpal_spmv_serial);

// All

void handle_spmv(char *buf, void *priv);

static
struct shell_cmd_impl tpal_spmv = {
  .cmd      = "spmv",
  .help_str = "",
  .handler  = handle_spmv,
};

nk_register_shell_cmd(tpal_spmv);

/*---------------------------------------------------------------------*/
/* Benchmark registry for fib */

// Interrupt

void handle_fib_interrupt(char *buf, void *priv);

static
struct shell_cmd_impl tpal_fib_interrupt = {
  .cmd      = "fib_interrupt",
  .help_str = "",
  .handler  = handle_fib_interrupt,
};

nk_register_shell_cmd(tpal_fib_interrupt);

// Interrupt no promotion

void handle_fib_interrupt_nopromote(char *buf, void *priv);

static
struct shell_cmd_impl tpal_fib_interrupt_nopromote = {
  .cmd      = "fib_interrupt_nopromote",
  .help_str = "",
  .handler  = handle_fib_interrupt_nopromote,
};

nk_register_shell_cmd(tpal_fib_interrupt_nopromote);

// Manual

void handle_fib_manual(char *buf, void *priv);

static
struct shell_cmd_impl tpal_fib_manual = {
  .cmd      = "fib_manual",
  .help_str = "",
  .handler  = handle_fib_manual,
};

nk_register_shell_cmd(tpal_fib_manual);

// Software polling

void handle_fib_software_polling(char *buf, void *priv);

static
struct shell_cmd_impl tpal_fib_software_polling = {
  .cmd      = "fib_software_polling",
  .help_str = "",
  .handler  = handle_fib_software_polling,
};

nk_register_shell_cmd(tpal_fib_software_polling);

// Serial

void handle_fib_serial(char *buf, void *priv);

static
struct shell_cmd_impl tpal_fib_serial = {
  .cmd      = "fib_serial",
  .help_str = "",
  .handler  = handle_fib_serial,
};

nk_register_shell_cmd(tpal_fib_serial);

// All

void handle_fib(char *buf, void *priv);

static
struct shell_cmd_impl tpal_fib = {
  .cmd      = "fib",
  .help_str = "",
  .handler  = handle_fib,
};

nk_register_shell_cmd(tpal_fib);

/*---------------------------------------------------------------------*/
/* Benchmark registry for knapsack */

// Interrupt

void handle_knapsack_interrupt(char *buf, void *priv);

static
struct shell_cmd_impl tpal_knapsack_interrupt = {
  .cmd      = "knapsack_interrupt",
  .help_str = "",
  .handler  = handle_knapsack_interrupt,
};

nk_register_shell_cmd(tpal_knapsack_interrupt);

// Interrupt no promotion

void handle_knapsack_interrupt_nopromote(char *buf, void *priv);

static
struct shell_cmd_impl tpal_knapsack_interrupt_nopromote = {
  .cmd      = "knapsack_interrupt_nopromote",
  .help_str = "",
  .handler  = handle_knapsack_interrupt_nopromote,
};

nk_register_shell_cmd(tpal_knapsack_interrupt_nopromote);

// Manual

void handle_knapsack_manual(char *buf, void *priv);

static
struct shell_cmd_impl tpal_knapsack_manual = {
  .cmd      = "knapsack_manual",
  .help_str = "",
  .handler  = handle_knapsack_manual,
};

nk_register_shell_cmd(tpal_knapsack_manual);

// Software polling

void handle_knapsack_software_polling(char *buf, void *priv);

static
struct shell_cmd_impl tpal_knapsack_software_polling = {
  .cmd      = "knapsack_software_polling",
  .help_str = "",
  .handler  = handle_knapsack_software_polling,
};

nk_register_shell_cmd(tpal_knapsack_software_polling);

// Serial

void handle_knapsack_serial(char *buf, void *priv);

static
struct shell_cmd_impl tpal_knapsack_serial = {
  .cmd      = "knapsack_serial",
  .help_str = "",
  .handler  = handle_knapsack_serial,
};

nk_register_shell_cmd(tpal_knapsack_serial);

// All

void handle_knapsack(char *buf, void *priv);

static
struct shell_cmd_impl tpal_knapsack = {
  .cmd      = "knapsack",
  .help_str = "",
  .handler  = handle_knapsack,
};

nk_register_shell_cmd(tpal_knapsack);

/*---------------------------------------------------------------------*/
/* Command line (grub) */

#define ARGMAX 80
#define ARG_OPEN '"'
#define ARG_CLOSE '"'

/*
 * format is -tpal bench "arg1 arg2 -f arg3"
 *                 ^
 *                 we start here
 */
static
int tpal_parse_args (char * args, int * argc, char ** argv[]) {
    char * arg_vec[ARGMAX];
    char * arg_copy  = NULL;
    char  * testname = NULL;
    char * curs      = NULL;
    char * tmp       = NULL;
    int foundargs = 0;
    int len = 0;

    memset(arg_vec, 0, sizeof(char*)*ARGMAX);

    curs = args;

    // skip white space
    while (*curs && *curs == ' ') curs++;

    tmp = curs;

    // find the end of the test name
    while (*curs != ' ') {
        curs++; len++;
    }

    testname = malloc(len+1);
    strncpy(testname, tmp, len);
    testname[len] = '\0';

    DEBUG("bench: %s\n", testname);

    arg_vec[(*argc)++] = testname;

    while (*curs && *curs != ARG_OPEN) curs++;

    if (*curs != ARG_OPEN) {
        DEBUG("No args found, skipping\n");
        goto out;
    }

    tmp = ++curs;
    len = 0;

    while (*curs && *curs != ARG_CLOSE) {
        curs++; len++;
    }

    arg_copy = malloc(len+1);
    strncpy(arg_copy, tmp, len);
    arg_copy[len] = '\0';

    tmp = strtok(arg_copy, " ");

    DEBUG("First arg: %s\n", tmp);

    if (tmp) {
        arg_vec[(*argc)++] = tmp;
    }

    while ((tmp = strtok(NULL, " "))) {
        DEBUG("Arg found: %s\n", tmp);
        arg_vec[(*argc)++] = tmp;
    }

    DEBUG("Found %d args\n", *argc);

out:
    *argv = malloc(sizeof(char*)*(*argc));
    memcpy(*argv, arg_vec, sizeof(char*)*(*argc));

    return 0;
}

void handle_cmdline(int argc, char** argv);

/*
static
int handle_tpal_from_cmdline (char* args) {
  char ** argv;
  int argc;
  
  //tpal_parse_args(args, &argc, &argv);
  
  //  handle_cmdline(argc, argv);
  return 0;
}

static struct nk_cmdline_impl tpal_cmdline_impl = {
    .name    = "tpal",
    .handler = handle_tpal_from_cmdline,
};
nk_register_cmdline_flag(tpal_cmdline_impl);
*/

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
