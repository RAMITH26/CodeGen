
#include "scheduler.h"
#include "task_config.h"
#include "cmsis_os2.h"
#include "stm32h7xx_hal.h"
#include "diag_manager.h"

/* ASW task declarations (must be provided by ASW integration) */
extern void Dynamic_address_assignment_Task(void);
extern void Dynamic_address_assignment_ISR(void);

extern void Soc_estimation_Task(void);
extern void Soc_estimation_ISR(void);

extern void Passive_Cell_balancing_Task(void);
extern void Passive_Cell_balancing_ISR(void);

/* Internal handlers */
static osThreadId_t soc_thread_id = NULL;
static osTimerId_t daa_timer_id = NULL;
static osTimerId_t pcb_timer_id = NULL;

/* Timer callbacks that wrap module tasks */
static void DAA_TimerCallback(void *argument)
{
    (void)argument;
    Dynamic_address_assignment_Task();
}

static void PCB_TimerCallback(void *argument)
{
    (void)argument;
    Passive_Cell_balancing_Task();
}

/* SOC estimation RTOS thread function */
static void SocEstimation_Thread(void *argument)
{
    (void)argument;

    const uint32_t period = PERIOD_SOC_ESTIMATION_MS;
    for (;;)
    {
        Soc_estimation_Task();
        /* Delay until next period */
        osDelay(period);
    }
}

void Scheduler_Init(void)
{
    /* Initialize CMSIS-RTOS2 kernel */
    (void)osKernelInitialize();

    /* Create SOC estimation RTOS task if enabled */
#if (ENABLE_SOC_ESTIMATION != 0u)
    const osThreadAttr_t soc_attr = {
        .name = SOC_ESTIMATION_TASK_NAME,
        .stack_size = (uint32_t)SOC_ESTIMATION_STACK_SIZE_BYTES,
        .priority = SOC_TASK_PRIORITY
    };
    soc_thread_id = osThreadNew(SocEstimation_Thread, NULL, &soc_attr);
    if (soc_thread_id == NULL)
    {
        Diag_ReportFault(1u); /* Fault ID 1: SOC thread creation failed */
    }
#endif

    /* Create timers for periodic tasks (timer-driven execution) */
#if (ENABLE_DYNAMIC_ADDRESS_ASSIGNMENT != 0u)
    const osTimerAttr_t daa_timer_attr = { .name = "DAA_Timer" };
    daa_timer_id = osTimerNew(DAA_TimerCallback, osTimerPeriodic, NULL, &daa_timer_attr);
    if (daa_timer_id == NULL)
    {
        Diag_ReportFault(2u); /* Fault ID 2: DAA timer creation failed */
    }
#endif

#if (ENABLE_PASSIVE_CELL_BALANCING != 0u)
    const osTimerAttr_t pcb_timer_attr = { .name = "PCB_Timer" };
    pcb_timer_id = osTimerNew(PCB_TimerCallback, osTimerPeriodic, NULL, &pcb_timer_attr);
    if (pcb_timer_id == NULL)
    {
        Diag_ReportFault(3u); /* Fault ID 3: Passive balancing timer creation failed */
    }
#endif
}

void Scheduler_Start(void)
{
    /* Start timers and kernel */
#if (ENABLE_DYNAMIC_ADDRESS_ASSIGNMENT != 0u)
    (void)osTimerStart(daa_timer_id, PERIOD_DYNAMIC_ADDRESS_ASSIGNMENT_MS);
#endif

#if (ENABLE_PASSIVE_CELL_BALANCING != 0u)
    (void)osTimerStart(pcb_timer_id, PERIOD_PASSIVE_CELL_BALANCING_MS);
#endif

    /* Start RTOS kernel: starts the soc thread and timers */
    (void)osKernelStart();
}

/* Hook to register generic periodic tasks (not used internally, but exported) */
void Scheduler_RegisterPeriodicTask(void (*task)(void), uint32_t period_ms)
{
    /* Create a dedicated timer for the task using osTimer API */
    if (task == NULL)
    {
        return;
    }

    /* We use a simple wrapper: allocate a timer per registration. */
    /* Note: For simplicity we rely on a small local trampoline function via osTimerNew
       requiring a static function; in production code a dynamic trampoline or
       context object is preferable. This function is left unimplemented here. */
    (void)task;
    (void)period_ms;
}

/* Hook to register ISR handlers: in this architecture ISRs should be declared
   and bound in the vector table. This function provides a registration point
   but actual binding is platform specific. */
void Scheduler_RegisterISRHandler(void (*isr)(void))
{
    (void)isr;
    /* Binding is platform-specific and handled elsewhere */
}

