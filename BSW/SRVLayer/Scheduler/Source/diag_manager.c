
#include "diag_manager.h"
#include "CodeGen_Logging.h"
#include "task_config.h"
#include "system_cfg.h"
#include "stm32h7xx_hal.h"

/* Diagnostics manager handles fault reporting, logging and watchdog servicing.
   The hardware watchdog is not enabled per project configuration. The
   service function is a noop when watchdog is disabled. */

static volatile uint32_t diag_fault_bitmap = 0u;

void Diag_Init(void)
{
    /* Initialize logging interface if present */
    (void)CodeGen_Logging_Init(); /* CodeGen_Logging.h expected to expose this */
    diag_fault_bitmap = 0u;

#if (WATCHDOG_ENABLE != 0u)
    /* Initialize watchdog peripheral (IWDG) if enabled (not enabled in config) */
    /* Implementation would go here */
#endif
}

void Diag_ReportFault(uint32_t fault_id)
{
    diag_fault_bitmap |= (1u << (fault_id & 31u));
    /* Log event */
    (void)CodeGen_Logging_LogEvent("DIAG", "FAULT", fault_id);
}

void Diag_ClearFault(uint32_t fault_id)
{
    diag_fault_bitmap &= ~(1u << (fault_id & 31u));
    (void)CodeGen_Logging_LogEvent("DIAG", "CLEAR", fault_id);
}

void Diag_Heartbeat(void)
{
    /* Periodic heartbeat for diagnostics */
    (void)CodeGen_Logging_LogEvent("DIAG", "HEART", 0u);
}

void Diag_ServiceWatchdog(void)
{
#if (WATCHDOG_ENABLE != 0u)
    /* Pet the watchdog here. Implementation depends on chosen watchdog (IWDG/WDT). */
    /* HAL_IWDG_Refresh(&hiwdg); */
#else
    /* No-op when watchdog disabled */
    (void)0u;
#endif
}

