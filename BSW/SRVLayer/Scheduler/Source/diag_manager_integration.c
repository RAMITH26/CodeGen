
#include "diag_manager.h"
#include "task_config.h"

/* This file provides integration hooks that might be used by ASW/BSW
   to report or query the diagnostic manager. */

/* Placeholder fault IDs for standard errors */
#define DIAG_FAULT_ID_WATCHDOG      (1u)
#define DIAG_FAULT_ID_TIM_INTERRUPT (2u)

void Watchdog_Service_Point(void)
{
#if (WATCHDOG_ENABLE != 0u)
    /* This function would be called periodically to service IWDG. */
    /* Example: HAL_IWDG_Refresh(&hiwdg); */
#else
    /* Watchdog disabled - nothing to do */
    (void)DiagManager_HasFaults;
#endif
}

