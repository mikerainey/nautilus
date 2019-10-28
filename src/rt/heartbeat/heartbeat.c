#include <nautilus/nautilus.h>
#include <rt/heartbeat/heartbeat.h>

#ifndef NAUT_CONFIG_NDPC_RT_DEBUG
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...) DEBUG_PRINT("heartbeat: " fmt, ##args)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("heartbeat: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("heartbeat: " fmt, ##args)

void nk_heartbeat_init() {
      printk("heartbeat!!!!\n");
    INFO("inited\n");
}

void nk_heartbeat_deinit() {
    INFO("deinited\n");
}
