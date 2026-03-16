
#ifndef SYSTEM_CFG_H
#define SYSTEM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* System clock configuration (in MHz as provided by wizard) */
#define SYSTEM_CLOCK_MHZ   (8u)

/* Watchdog configuration */
#define WATCHDOG_ENABLE    (0u) /* 0 = Disabled, 1 = Enabled */
#define WATCHDOG_TIMEOUT_MS  (0u)
#define WATCHDOG_RESET_MODE  (0u)

void System_Init(void);
void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_CFG_H */

