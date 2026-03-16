
#ifndef BALANCING_ASW_H
#define BALANCING_ASW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard includes */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Configuration header */
#include "balancing_asw_cfg.h"

/* BSW (AFE) status type */
typedef enum
{
    BSW_STATUS_OK = 0,
    BSW_STATUS_ERROR = 1
} BSW_Status_t;

/* AFE bleed state */
typedef enum
{
    AFE_BLEED_OFF = 0u,
    AFE_BLEED_ON  = 1u
} AFE_BleedState_t;

/* Balancing diagnostics per cell */
typedef struct
{
    bool bleed_error; /* true if last bleed operation failed for this cell */
} Bal_CellStatus_t;

/* External declarations of BSW APIs.
   These functions are expected to be provided by lower layers (BSW).
   Weak definitions are available in balancing_asw.c for integration/testing.
*/
extern BSW_Status_t AFE_ReadCellVoltages(uint16_t voltages[], size_t count);
extern BSW_Status_t AFE_SetBleed(uint8_t cell_index, AFE_BleedState_t state);

/* External BMS state - charging state must be provided by system.
   The BMS charging state value for "charging" is defined as BMS_STATE_CHARGING.
*/
extern volatile uint8_t BMS_State;

/* ASW API */

/* Initialize balancing ASW. Must be called once before RunTick. */
void Bal_ASW_Init(void);

/* Periodic tick handler - executes the balancing flow (one cycle). */
void Bal_ASW_RunTick(void);

/* Accessors for diagnostics/status */
extern Bal_CellStatus_t Bal_Status[NUM_CELLS];
extern AFE_BleedState_t Bal_BleedState[NUM_CELLS];
extern volatile bool Balancing_Active;
extern volatile uint8_t Parity_Flag; /* 0 or 1 */
extern volatile uint16_t Last_Max_mV;
extern volatile uint16_t Last_Min_mV;
extern volatile uint16_t Last_Diff_mV;

#ifdef __cplusplus
}
#endif

#endif /* BALANCING_ASW_H */

