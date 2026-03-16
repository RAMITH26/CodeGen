
#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmsis_os2.h"

/* Module enable flags (1 = enabled, 0 = disabled) */
#define ENABLE_DYNAMIC_ADDRESS_ASSIGNMENT   (1u)
#define ENABLE_SOC_ESTIMATION               (1u)
#define ENABLE_PASSIVE_CELL_BALANCING       (1u)

/* Periods in milliseconds (timer-driven execution) */
#define PERIOD_DYNAMIC_ADDRESS_ASSIGNMENT_MS   (100u)   /* 100 ms */
#define PERIOD_SOC_ESTIMATION_MS                (500u)   /* 500 ms */
#define PERIOD_PASSIVE_CELL_BALANCING_MS        (100u)   /* 100 ms */

/* Priorities (mapped to CMSIS-RTOS2 priority levels) */
#define PRIORITY_CRITICAL    (osPriorityRealtime)
#define PRIORITY_HIGH        (osPriorityHigh)
#define PRIORITY_DEFAULT     (osPriorityNormal)

/* Module priorities (use mapping above) */
#define DAA_TASK_PRIORITY    PRIORITY_CRITICAL
#define SOC_TASK_PRIORITY    PRIORITY_HIGH
#define PCB_TASK_PRIORITY    PRIORITY_CRITICAL

/* Memory usage information (as provided by the configuration wizard) */
/* Ram/Flash usage units preserved as provided (interpretation left to build system) */
#define DYNAMIC_ADDRESS_ASSIGNMENT_RAM_USAGE_BYTES   (512u)
#define DYNAMIC_ADDRESS_ASSIGNMENT_FLASH_USAGE_BYTES (512u)

#define SOC_ESTIMATION_RAM_USAGE_BYTES   (12u)
#define SOC_ESTIMATION_FLASH_USAGE_BYTES (12u)

#define PASSIVE_CELL_BALANCING_RAM_USAGE_BYTES   (2u)
#define PASSIVE_CELL_BALANCING_FLASH_USAGE_BYTES (23u)

/* RTOS stack size for RTOS tasks (in bytes). Derived from reported RAM usage.
   Use a conservative multiplier to ensure sufficient stack. */
#define SOC_ESTIMATION_STACK_SIZE_BYTES   ((SOC_ESTIMATION_RAM_USAGE_BYTES) * 256u)

/* Names for tasks (string literals for diagnostics) */
#define DYNAMIC_ADDRESS_ASSIGNMENT_TASK_NAME   "DynamicAddrAsg"
#define SOC_ESTIMATION_TASK_NAME               "SOC_Est"
#define PASSIVE_CELL_BALANCING_TASK_NAME       "PassiveBal"

/* Generic timeout for osDelay until next tick (ms) */
#define SCHEDULER_TICK_MS  (10u)

#ifdef __cplusplus
}
#endif

#endif /* TASK_CONFIG_H */

