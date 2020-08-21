/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <khale@cs.iit.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <khale@cs.iit.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/nemo.h>
#include <nautilus/cpu.h>
#include <nautilus/percpu.h>
#include <nautilus/irq.h>
#include <nautilus/mm.h>
#include <nautilus/naut_assert.h>
#include <nautilus/naut_string.h>
#include <nautilus/shell.h>
#include <nautilus/cmdline.h>
#include <nautilus/vc.h>
#include <test/test.h>

#include <dev/apic.h>

#define NEMO_DEBUG(fmt, args...) DEBUG_PRINT("NEMO: " fmt, ##args)
#define NEMO_INFO(fmt, args...)  nk_vc_printf("NEMO: " fmt, ##args)
#define NEMO_ERR(fmt, args...)   ERROR_PRINT("NEMO: " fmt, ##args)
#define NEMO_WARN(fmt, args...)  WARN_PRINT("NEMO: " fmt, ##args)


static nemo_event_t * nemo_action_table[NEMO_MAX_EVENTS] __align(64);
static nemo_event_id_t nemo_lookup_table[NAUT_CONFIG_MAX_CPUS] __align(64);


static inline int
event_is_valid (nemo_event_id_t id)
{
	if (id < NEMO_MAX_EVENTS && 
		nemo_action_table[id]) {
		return 1;
	}

	return 0;
}


static int
nemo_ipi_event_recv (excp_entry_t * excp, excp_vec_t v, void *state)
{
	nemo_event_id_t eid = nemo_lookup_table[my_cpu_id()];

	ASSERT(eid < NEMO_MAX_EVENTS);

	nemo_event_t * event = nemo_action_table[eid];

	ASSERT(event);
	ASSERT(event->action);

        //	NEMO_DEBUG("%lu Recv'd notification for task id=%u func=%p priv=%p\n", nk_sched_get_realtime(), eid, (void*)event->action, event->priv_data);
        // we believe event->action contains a bogus value when run via the testing framework
        // peter believes the bogus value is caused by the test framework running before nemo configuration
        // ask kyle for debugging output that his student implemented
	event->action(excp, event->priv_data);

	IRQ_HANDLER_END();

	return 0;
}


void
nk_nemo_event_notify (nemo_event_id_t eid, int cpu)
{
	ASSERT(event_is_valid(eid));
	ASSERT(cpu < NAUT_CONFIG_MAX_CPUS && cpu >= 0);

	unsigned remote_apic   = nk_get_nautilus_info()->sys.cpus[cpu]->lapic_id;
	struct apic_dev * apic = per_cpu_get(apic);

	apic_ipi(apic, remote_apic, NEMO_INT_VEC);
}


void
nk_nemo_event_broadcast (nemo_event_id_t eid)
{
	ASSERT(event_is_valid(eid));
	struct apic_dev * apic = per_cpu_get(apic);
	apic_bcast_ipi(apic, NEMO_INT_VEC);
}


static nemo_event_id_t
allocate_event_id (void) 
{
	unsigned i;
	for (i = 0; i < NEMO_MAX_EVENTS; i++) {
		if (!nemo_action_table[i]) {
			NEMO_DEBUG("Allocating event in slot %u\n", i);
			return i;
		}
	}

	return -1;
}


nemo_event_id_t
nk_nemo_register_event_action (nemo_action_t func, void * priv_data)
{

	if (!func) {
		NEMO_ERR("Invalid function provided to nemo_regiser_task\n");
		return -1;
	}


	nemo_event_t * new_event = malloc(sizeof(nemo_event_t));
	if (!new_event) {
		NEMO_ERR("Could not malloc new task\n");
		return -1;
	}
	memset(new_event, 0, sizeof(nemo_event_t));

	new_event->action    = func;
	new_event->priv_data = priv_data;

	nemo_event_id_t id   = allocate_event_id();

	if (id < 0) {
		NEMO_ERR("Could not allocate new event in event action table\n");
		goto out;
	}

	new_event->id = id;

	nemo_action_table[id] = new_event;

	NEMO_DEBUG("Registering new event %d func=%p data=%p\n", new_event->id, new_event->action, new_event->priv_data);

	return id;

out:
	free(new_event);
	return -1;
}


void
nk_nemo_unregister_event_action (nemo_event_id_t id)
{
	ASSERT(id < NEMO_MAX_EVENTS && id >= 0);

	NEMO_DEBUG("Unregister event %d\n", id);

	if (!nemo_action_table[id]) {
		return;
	}

	free(nemo_action_table[id]);
	nemo_action_table[id] = NULL;
}


void
nk_nemo_event_await (void)
{
	halt();
}


int
nk_nemo_init (void)
{
	if (register_int_handler(NEMO_INT_VEC, nemo_ipi_event_recv, NULL) != 0) {
		NEMO_ERR("Could not register Nemo interrupt handler\n");
		return -1;
	}
	
	return 0;
}


static void
nemo_test_handler (excp_entry_t * e, void * priv)
{
    struct nk_virtual_console * vc = (struct nk_virtual_console*)priv;
	nk_vc_printf_specific(vc, "TEST invoked from core %u\n", my_cpu_id());
}


static int
test_nemo (char * buf, void * priv)
{
	NEMO_INFO("Testing Nemo Event Subsystem\n");

    struct nk_virtual_console * vc = get_cur_thread()->vc;

	nemo_event_id_t id = nk_nemo_register_event_action(nemo_test_handler, vc);
    cpu_id_t remote_core = 1;

	if (id < 0) {
		NEMO_ERR("Could not register dummy nemo task\n");
		return -1;
	}

	//NEMO_INFO("Notifying event on core %d\n", remote_core);
	nk_vc_printf_specific(vc, "Notifying event on core %d\n", remote_core);
	nk_nemo_event_notify(id, remote_core);
    return 0;
}

static struct shell_cmd_impl nemo_impl = {
    .cmd      = "nemotest",
    .help_str = "nemotest",
    .handler  = test_nemo,
};
nk_register_shell_cmd(nemo_impl);

