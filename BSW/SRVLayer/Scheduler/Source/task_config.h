
#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* System clock (Hz) as provided by project configuration */
#define SYSTEM_CLOCK_HZ            (1000000u)  /* 1 MHz */

/* Watchdog configuration (from system config) */
#define WATCHDOG_ENABLE            (0u)        /* 0 = Disabled, 1 = Enabled */
#define WATCHDOG_RESET_MODE_SW     (1u)
#define WATCHDOG_TIMEOUT_US        (1u)        /* 1 microsecond (unused when disabled) */

/* Task enable flags */
#define TASK_ENABLE_APP_M1         (1u)
#define TASK_ENABLE_DEMO           (1u)
#define TASK_ENABLE_TEST           (1u)
#define TASK_ENABLE_RELEASE_TEST   (1u)

/* Task priorities (lower numeric -> higher priority in this simple scheduler) */
#define TASK_PRIO_CRITICAL         (1u)
#define TASK_PRIO_HIGH             (2u)

/* Task metadata: periods expressed in milliseconds (timer-driven) */
/* NOTE: Period values chosen to match BMS responsiveness requirements:
   Critical modules run at 10 ms, high-priority demo runs at 50 ms. */
#define TASK_PERIOD_MS_APP_M1          (10u)
#define TASK_PRIORITY_APP_M1           (TASK_PRIO_CRITICAL)

#define TASK_PERIOD_MS_DEMO            (50u)
#define TASK_PRIORITY_DEMO             (TASK_PRIO_HIGH)

#define TASK_PERIOD_MS_TEST            (10u)
#define TASK_PRIORITY_TEST             (TASK_PRIO_CRITICAL)

#define TASK_PERIOD_MS_RELEASE_TEST    (10u)
#define TASK_PRIORITY_RELEASE_TEST     (TASK_PRIO_CRITICAL)

/* Stack sizes (bytes) when RTOS threads are used; for cooperative scheduler not used,
   but present for build-time reflection. */
#define TASK_STACK_SIZE_APP_M1         (1024u)    /* 1 KB */
#define TASK_STACK_SIZE_DEMO           (512u)     /* 512 B */
#define TASK_STACK_SIZE_TEST           (512u)
#define TASK_STACK_SIZE_RELEASE_TEST   (512u)

/* Memory usage reporting (from ASW config) */
#define TASK_RAM_USAGE_APP_M1          (1024u * 1024u)    /* 1 MB */
#define TASK_FLASH_USAGE_APP_M1        (1024u * 1024u)    /* 1 MB */

#define TASK_RAM_USAGE_DEMO            (512u * 1024u)     /* 512 kB */
#define TASK_FLASH_USAGE_DEMO          (512u * 1024u)     /* 512 kB */

#define TASK_RAM_USAGE_TEST            (222u * 1024u)     /* 222 kB */
#define TASK_FLASH_USAGE_TEST          (222u * 1024u)     /* 222 kB */

#define TASK_RAM_USAGE_RELEASE_TEST    (111u * 1024u)     /* 111 kB */
#define TASK_FLASH_USAGE_RELEASE_TEST  (222u * 1024u)     /* 222 kB */

/* Scheduler configuration */
#define SCHEDULER_TICK_HZ              (1000u)    /* 1 kHz tick -> 1 ms resolution */
#define SCHEDULER_MAX_TASKS            (8u)

#ifdef __cplusplus
}
#endif

#endif /* TASK_CONFIG_H */

