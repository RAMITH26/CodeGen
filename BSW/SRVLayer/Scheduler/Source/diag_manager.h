
#ifndef DIAG_MANAGER_H
#define DIAG_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Initialize diagnostics manager */
void DiagManager_Init(void);

/* Periodic diagnostics processing - run from main loop */
void DiagManager_Process(void);

/* Report a fault to diagnostics manager */
void DiagManager_ReportFault(uint32_t fault_id);

/* Clear a fault */
void DiagManager_ClearFault(uint32_t fault_id);

/* Retrieve global fault state */
bool DiagManager_HasFaults(void);

#ifdef __cplusplus
}
#endif

#endif /* DIAG_MANAGER_H */

