
#include "scheduler.h"
#include "stm32h7xx_hal.h"
#include <string.h>

typedef struct
{
    const char * name;
    TaskEntry_t entry;
    uint32_t period_ms;
    uint8_t priority;
    bool enabled;
    uint32_t last_run_tick;
} SchedulerTask_t;

static SchedulerTask_t g_tasks[SCHEDULER_MAX_TASKS];
static uint32_t g_task_count = 0u;
static volatile uint32_t g_tick_count = 0u;
static bool g_scheduler_running = false;

/* ISR registry (not used to execute tasks in ISR, but for vector binding) */
#define MAX_ISR_REGISTRY 8u
typedef struct
{
    const char * name;
    IsrEntry_t isr;
} ISRReg_t;
static ISRReg_t g_isr_reg[MAX_ISR_REGISTRY];
static uint32_t g_isr_count = 0u;

void Scheduler_Init(void)
{
    uint32_t i;
    for (i = 0u; i < SCHEDULER_MAX_TASKS; ++i)
    {
        g_tasks[i].name = NULL;
        g_tasks[i].entry = (TaskEntry_t)0u;
        g_tasks[i].period_ms = 0u;
        g_tasks[i].priority = 0u;
        g_tasks[i].enabled = false;
        g_tasks[i].last_run_tick = 0u;
    }
    g_task_count = 0u;
    g_tick_count = 0u;
    g_scheduler_running = false;

    for (i = 0u; i < MAX_ISR_REGISTRY; ++i)
    {
        g_isr_reg[i].name = NULL;
        g_isr_reg[i].isr = (IsrEntry_t)0u;
    }
    g_isr_count = 0u;
}

void Scheduler_RegisterTask(const char * name,
                            TaskEntry_t entry,
                            uint32_t period_ms,
                            uint8_t priority,
                            bool enabled)
{
    if ((g_task_count >= SCHEDULER_MAX_TASKS) || (entry == (TaskEntry_t)0u))
    {
        return;
    }
    g_tasks[g_task_count].name = name;
    g_tasks[g_task_count].entry = entry;
    g_tasks[g_task_count].period_ms = (period_ms == 0u) ? 1u : period_ms;
    g_tasks[g_task_count].priority = priority;
    g_tasks[g_task_count].enabled = enabled;
    g_tasks[g_task_count].last_run_tick = 0u;
    ++g_task_count;
}

void Scheduler_RegisterISR(const char * name, IsrEntry_t isr)
{
    if ((g_isr_count >= MAX_ISR_REGISTRY) || (isr == (IsrEntry_t)0u))
    {
        return;
    }
    g_isr_reg[g_isr_count].name = name;
    g_isr_reg[g_isr_count].isr = isr;
    ++g_isr_count;
}

void Scheduler_Start(void)
{
    g_scheduler_running = true;
}

void Scheduler_Stop(void)
{
    g_scheduler_running = false;
}

void Scheduler_TickHandler(void)
{
    /* Called from timer ISR context; increment tick */
    ++g_tick_count;
}

static int compare_priority(const void * a, const void * b)
{
    const SchedulerTask_t * ta = (const SchedulerTask_t *)a;
    const SchedulerTask_t * tb = (const SchedulerTask_t *)b;
    /* lower numeric value = higher priority */
    if (ta->priority < tb->priority) return -1;
    if (ta->priority > tb->priority) return 1;
    return 0;
}

void Scheduler_Dispatch(void)
{
    uint32_t i;
    uint32_t current_tick;
    if (!g_scheduler_running)
    {
        return;
    }

    current_tick = g_tick_count;

    /* Simple scheduling: iterate tasks by priority order */
    /* Make a local copy and sort by priority to ensure deterministic order */
    SchedulerTask_t local_tasks[SCHEDULER_MAX_TASKS];
    uint32_t local_count = 0u;

    for (i = 0u; i < g_task_count; ++i)
    {
        local_tasks[local_count++] = g_tasks[i];
    }

    /* Simple insertion sort (small N) to maintain MISRA friendliness */
    for (i = 1u; i < local_count; ++i)
    {
        SchedulerTask_t key = local_tasks[i];
        int32_t j = (int32_t)i - 1;
        while ((j >= 0) && (local_tasks[j].priority > key.priority))
        {
            local_tasks[j + 1] = local_tasks[j];
            --j;
        }
        local_tasks[j + 1] = key;
    }

    for (i = 0u; i < local_count; ++i)
    {
        SchedulerTask_t * t = &local_tasks[i];
        uint32_t elapsed;
        if ((t->enabled == false) || (t->entry == (TaskEntry_t)0u))
        {
            continue;
        }
        /* Compute elapsed time in ms since last_run_tick */
        if (current_tick >= t->last_run_tick)
        {
            elapsed = current_tick - t->last_run_tick;
        }
        else
        {
            /* Tick wrapped - handle wrap-around safely */
            elapsed = (UINT32_MAX - t->last_run_tick) + current_tick + 1u;
        }

        if (elapsed >= t->period_ms)
        {
            /* Update the actual g_tasks[] table to reflect last_run_tick */
            /* Find real task index and update to keep state across dispatch calls */
            uint32_t k;
            for (k = 0u; k < g_task_count; ++k)
            {
                if (g_tasks[k].entry == t->entry)
                {
                    g_tasks[k].last_run_tick = current_tick;
                    break;
                }
            }
            /* Execute task in main loop context (per handler_type: Main loop) */
            t->entry();
        }
    }
}

/* Provide wrapper to allow TIM IRQ to call HAL handler */
void Scheduler_TimerIRQHandler(void)
{
    /* Increment tick (ms) */
    Scheduler_TickHandler();
}

