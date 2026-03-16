
#include "init_handlers.h"
#include "system_cfg.h"
#include "diag_manager.h"
#include "task_config.h"

/* ASW headers */
#include "CodeGen_Dynamic_address_assignment.h"
#include "BMS_SOC_Manager.h"
#include "balancing_asw.h"

/* Ensure the ASW modules expose the standardized Init/Deinit symbols.
   The integration layer expects the following symbols to be implemented by the ASW:
     - Dynamic_address_assignment_Init / Deinit
     - Soc_estimation_Init / Deinit
     - Passive_Cell_balancing_Init / Deinit
   If ASW headers use different names, those headers should provide wrappers.
*/

/* Declarations of standardized init/deinit functions implemented by ASW or by wrappers */
extern void Dynamic_address_assignment_Init(void);
extern void Dynamic_address_assignment_Deinit(void);

extern void Soc_estimation_Init(void);
extern void Soc_estimation_Deinit(void);

extern void Passive_Cell_balancing_Init(void);
extern void Passive_Cell_balancing_Deinit(void);

/* Initialization ordering:
   1) All modules with execution_phase == Init
      - Soc_estimation (Init)
      - Passive_Cell_balancing (Init)
   2) System services (e.g., CAN, GPIO) are assumed initialized by ASW or system init
   3) Modules with execution_phase == Run
      - Dynamic_address_assignment (Run)
*/

void InitHandlers_InitAll(void)
{
    /* System-level init already done in main: System_Init(), HAL, clocks */
    Diag_Init();

    /* Init phase modules */
#if (ENABLE_SOC_ESTIMATION != 0u)
    Soc_estimation_Init();
#endif

#if (ENABLE_PASSIVE_CELL_BALANCING != 0u)
    Passive_Cell_balancing_Init();
#endif

    /* Run phase modules */
#if (ENABLE_DYNAMIC_ADDRESS_ASSIGNMENT != 0u)
    Dynamic_address_assignment_Init();
#endif
}

void InitHandlers_DeinitAll(void)
{
    /* Deinit in reverse order to respect dependency teardown */
#if (ENABLE_DYNAMIC_ADDRESS_ASSIGNMENT != 0u)
    Dynamic_address_assignment_Deinit();
#endif

#if (ENABLE_PASSIVE_CELL_BALANCING != 0u)
    Passive_Cell_balancing_Deinit();
#endif

#if (ENABLE_SOC_ESTIMATION != 0u)
    Soc_estimation_Deinit();
#endif
}

