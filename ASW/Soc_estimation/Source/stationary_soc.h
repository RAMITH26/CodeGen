
#ifndef STATIONARY_SOC_H
#define STATIONARY_SOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "can_if.h"
#include "stationary_soc_cfg.h"

/* Public runtime states */
typedef enum
{
    ST_SOC_STATE_UNINIT = 0,
    ST_SOC_STATE_INIT,
    ST_SOC_STATE_NORMAL,
    ST_SOC_STATE_SHUTDOWN,
    ST_SOC_STATE_IDLE,
    ST_SOC_STATE_ERROR
} StationarySoc_State_t;

/* AFE sample structure */
typedef struct
{
    float    pack_current;                       /* Amperes (positive = discharge) */
    float    cell_voltages[ST_SOC_NUM_CELLS];    /* Volts */
    float    temperatures[ST_SOC_NUM_TEMPS];     /* Degrees C */
    uint32_t timestamp_ms;                       /* HAL_GetTick() timestamp */
} StationarySoc_AfeSample_t;

/* Diagnostics info */
typedef struct
{
    bool     input_invalid;
    bool     soc_valid;
    bool     ocv_correction_applied;
    bool     cell_imbalance_warning;
    bool     nvm_write_event;
    uint32_t last_nvm_write_time_ms;
    uint32_t last_input_invalid_time_ms;
} StationarySoc_Diag_t;

/* Public API */

/* Initialize the Stationary SOC module.
   Call once after clocks and HAL are initialized. */
void StationarySoc_Init(void);

/* Main periodic function - must be called at a steady rate equal to
   ST_SOC_SAMPLE_PERIOD_S (in seconds) or a multiple thereof that matches
   sample timing. This function reads AFE samples (via AFE_ReadSamples())
   and runs the state machine. */
void StationarySoc_MainFunction(void);

/* Request orderly shutdown (will cause SOC persist and stop updates) */
void StationarySoc_RequestShutdown(void);

/* Get last computed pack SOC (0..100.0) */
float StationarySoc_GetPackSoc(void);

/* Get remaining Ah as computed (pack_capacity * pack_soc / 100) */
float StationarySoc_GetAhRemaining(void);

/* Get current module state */
void StationarySoc_GetState(StationarySoc_State_t *state);

/* Get diagnostics snapshot */
void StationarySoc_GetDiag(StationarySoc_Diag_t *outDiag);

/* Register optional publish notification callback (called from main context
   after a successful publish). Pass NULL to disable. */
void StationarySoc_RegisterPublishNotify(void (*notify)(void));

/* Hardware abstraction hooks that integrator may implement/override:
   - AFE_ReadSamples() should read sensors and fill sample; return true on success.
     Provide a weak default implementation that returns false (not implemented).
   - NVM_ReadSoc() should attempt to read persisted SOC into out_soc and return true if valid.
   - NVM_WriteSoc() should attempt to persist the supplied SOC and return true on success.

   The default weak implementations are provided in stationary_soc.c and may be
   overridden by application code by providing strong implementations with the
   same prototypes. */
bool AFE_ReadSamples(StationarySoc_AfeSample_t *outSample) __attribute__((weak));
bool NVM_ReadSoc(float *out_soc) __attribute__((weak));
bool NVM_WriteSoc(float soc) __attribute__((weak));

#ifdef __cplusplus
}
#endif

#endif /* STATIONARY_SOC_H */

