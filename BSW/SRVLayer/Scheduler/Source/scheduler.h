
#ifndef SCHEDULER_H
#define SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void Scheduler_Init(void);
void Scheduler_Start(void);
void Scheduler_RegisterPeriodicTask(void (*task)(void), uint32_t period_ms);
void Scheduler_RegisterISRHandler(void (*isr)(void));

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULER_H */

