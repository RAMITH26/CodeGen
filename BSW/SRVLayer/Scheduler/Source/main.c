
#include "system_cfg.h"
#include "init_handlers.h"
#include "scheduler.h"
#include "diag_manager.h"
#include "task_config.h"
#include "stm32h7xx_hal.h"
#include "CodeGen_Logging.h"

/* Standard ASW includes - declarations for task entry points */
extern void Dynamic_address_assignment_Task(void);
extern void Soc_estimation_Task(void);
extern void Passive_Cell_balancing_Task(void);

/* Main entry point */
int main(void)
{
    /* System initialization */
    System_Init();

    /* Initialize logging and diagnostics */
    Diag_Init();

    /* Initialize application modules in the correct order */
    InitHandlers_InitAll();

    /* Initialize scheduler and create tasks/timers */
    Scheduler_Init();

    /* Start scheduler (this will start RTOS kernel and timers) */
    Scheduler_Start();

    /* If osKernelStart returns, fallback to infinite loop to service main-loop tasks */
    for (;;)
    {
        /* In case RTOS is not running, provide cooperative polling at SCHEDULER_TICK_MS granularity */
#if (ENABLE_DYNAMIC_ADDRESS_ASSIGNMENT != 0u)
        Dynamic_address_assignment_Task();
#endif

#if (ENABLE_PASSIVE_CELL_BALANCING != 0u)
        Passive_Cell_balancing_Task();
#endif

        Diag_Heartbeat();
        Diag_ServiceWatchdog();

        HAL_Delay(SCHEDULER_TICK_MS);
    }

    /* Should never reach here */
    return 0;
}

