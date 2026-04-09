
#ifndef SYSTEM_CFG_H
#define SYSTEM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "task_config.h"

/* System configuration functions */
void System_Init(void);
void SystemClock_Config(void);
void MX_TIM2_Init(void);
void MX_NVIC_Config(void);

/* TIM handle exported for scheduler use */
extern TIM_HandleTypeDef htim2;

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_CFG_H */

