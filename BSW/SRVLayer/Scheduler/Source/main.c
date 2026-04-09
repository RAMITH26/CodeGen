
#include "system_cfg.h"
#include "init_handlers.h"
#include "scheduler.h"
#include "diag_manager.h"
#include "task_config.h"
#include "stm32h7xx_hal.h"

/* Forward declarations of ASW task/ISR functions (strict naming) */
extern void App_M1_Task(void);
extern void Demo_Task(void);
extern void Test_Task(void);
extern void Release_test_asw_Task(void);

extern void App_M1_ISR(void);
extern void Demo_ISR(void);
extern void Test_ISR(void);
extern void Release_test_asw_ISR(void);

/* Local helper to register tasks based on task_config.h */
static void Register_Tasks(void)
{
    /* Register tasks - they will be executed in main loop by scheduler */
#if TASK_ENABLE_APP_M1
    Scheduler_RegisterTask("App_M1", App_M1_Task, TASK_PERIOD_MS_APP_M1, TASK_PRIORITY_APP_M1, true);
    Scheduler_RegisterISR("App_M1_ISR", App_M1_ISR);
#endif
#if TASK_ENABLE_DEMO
    Scheduler_RegisterTask("Demo", Demo_Task, TASK_PERIOD_MS_DEMO, TASK_PRIORITY_DEMO, true);
    Scheduler_RegisterISR("Demo_ISR", Demo_ISR);
#endif
#if TASK_ENABLE_TEST
    Scheduler_RegisterTask("Test", Test_Task, TASK_PERIOD_MS_TEST, TASK_PRIORITY_TEST, true);
    Scheduler_RegisterISR("Test_ISR", Test_ISR);
#endif
#if TASK_ENABLE_RELEASE_TEST
    Scheduler_RegisterTask("Release_test_asw", Release_test_asw_Task, TASK_PERIOD_MS_RELEASE_TEST, TASK_PRIORITY_RELEASE_TEST, true);
    Scheduler_RegisterISR("Release_test_asw_ISR", Release_test_asw_ISR);
#endif
}

int main(void)
{
    /* Initialize hardware abstraction layer and system peripherals */
    System_Init();

    /* Initialize diagnostics manager */
    DiagManager_Init();

    /* Scheduler init */
    Scheduler_Init();

    /* Register ASW init handlers and perform initialization sequence */
    InitHandlers_RegisterAll();
    InitHandlers_InitAll();

    /* Register tasks into scheduler */
    Register_Tasks();

    /* Start scheduler */
    Scheduler_Start();

    /* Main loop: dispatch scheduler and process diagnostics */
    for (;;)
    {
        /* Dispatch periodic tasks (executed in main thread per configuration) */
        Scheduler_Dispatch();

        /* Process diagnostics and fault handling */
        DiagManager_Process();

        /* Light weight idle: could enter low-power until next interrupt */
        __WFI();
    }

    /* never reached */
    return 0;
}

