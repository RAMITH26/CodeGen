
#ifndef SCHEDULER_H
#define SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "task_config.h"

/* Task entry prototype required by generator */
typedef void (*TaskEntry_t)(void);
typedef void (*IsrEntry_t)(void);

/* Scheduler API */
void Scheduler_Init(void);
void Scheduler_Start(void);
void Scheduler_Stop(void);
void Scheduler_RegisterTask(const char * name,
                            TaskEntry_t entry,
                            uint32_t period_ms,
                            uint8_t priority,
                            bool enabled);
void Scheduler_TickHandler(void); /* called from TIM ISR */
void Scheduler_Dispatch(void);    /* called from main loop */

/* Helper to bind ISR handlers (for completeness) */
void Scheduler_RegisterISR(const char * name, IsrEntry_t isr);

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULER_H */

