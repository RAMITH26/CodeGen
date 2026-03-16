
#ifndef DIAG_MANAGER_H
#define DIAG_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void Diag_Init(void);
void Diag_ReportFault(uint32_t fault_id);
void Diag_ClearFault(uint32_t fault_id);
void Diag_Heartbeat(void);
void Diag_ServiceWatchdog(void);

#ifdef __cplusplus
}
#endif

#endif /* DIAG_MANAGER_H */

