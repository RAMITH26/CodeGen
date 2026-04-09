
#include "diag_manager.h"
#include "task_config.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/* Simple diagnostics representation */
#define DIAG_MAX_FAULTS    (16u)

static uint32_t g_fault_bitmap[DIAG_MAX_FAULTS / 32u];
static volatile bool g_faults_present = false;

void DiagManager_Init(void)
{
    memset(g_fault_bitmap, 0x00u, sizeof(g_fault_bitmap));
    g_faults_present = false;

#if (WATCHDOG_ENABLE != 0u)
    /* If watchdog enabled, initialize and configure (hardware-specific).
       For the generated code we leave integration points here. */
    /* Example: MX_IWDG_Init(); */
#endif
}

void DiagManager_Process(void)
{
    /* Periodic processing: check system health, escalate faults if required.
       If watchdog is enabled the diag manager must service it here. */
#if (WATCHDOG_ENABLE != 0u)
    /* Service watchdog: example pseudo-call:
       IWDG_Refresh(); */
#endif
    /* No-op if no faults */
    (void)g_fault_bitmap;
}

void DiagManager_ReportFault(uint32_t fault_id)
{
    uint32_t idx;
    uint32_t bit;
    if (fault_id >= (DIAG_MAX_FAULTS)) { return; }
    idx = (fault_id / 32u);
    bit = (1u << (fault_id & 31u));
    g_fault_bitmap[idx] |= bit;
    g_faults_present = true;
}

void DiagManager_ClearFault(uint32_t fault_id)
{
    uint32_t idx;
    uint32_t bit;
    if (fault_id >= (DIAG_MAX_FAULTS)) { return; }
    idx = (fault_id / 32u);
    bit = (1u << (fault_id & 31u));
    g_fault_bitmap[idx] &= ~bit;
    /* Recompute presence flag conservatively */
    {
        uint32_t i;
        bool any = false;
        for (i = 0u; i < (DIAG_MAX_FAULTS / 32u); ++i)
        {
            if (g_fault_bitmap[i] != 0u)
            {
                any = true;
                break;
            }
        }
        g_faults_present = any;
    }
}

bool DiagManager_HasFaults(void)
{
    return g_faults_present;
}

