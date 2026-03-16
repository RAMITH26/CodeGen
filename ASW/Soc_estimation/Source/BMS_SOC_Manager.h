
#ifndef BMS_SOC_MANAGER_H
#define BMS_SOC_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard headers */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* HAL */
#include "stm32h7xx_hal.h"

/* BSW storage API */
#include "BMS_StationaryStorage.h"
#include "BMS_SOC_Manager_cfg.h"

/* Public status codes */
typedef enum
{
    MSM_OK = 0,
    MSM_ERROR = 1,
    MSM_TIMEOUT = 2,
    MSM_INVALID_PARAM = 3,
    MSM_NO_PERSISTED_SOC = 4,
    MSM_NVM_ERROR = 5
} MSM_StatusType;

/* Runtime state machine */
typedef enum
{
    MSM_STATE_UNINITIALIZED = 0,
    MSM_STATE_INITIALIZATION,
    MSM_STATE_OPERATIONAL,
    MSM_STATE_NVM_PERSIST,
    MSM_STATE_FAULT
} MSM_StateType;

/* AFE sample structure passed to ProcessSample */
typedef struct
{
    float pack_current_A;                       /* Instantaneous pack current (A), positive = discharge */
    float cell_voltages_V[BMS_SOC_MAX_CELLS];   /* Per-cell voltages; unused cells set to 0.0f */
    uint8_t cell_count;                         /* Number of valid cells in cell_voltages_V */
    uint32_t timestamp_s;                       /* Timestamp seconds since epoch or boot */
    uint32_t sample_interval_ms;                /* Time since previous AFE sample in milliseconds */
    uint32_t rest_duration_s;                   /* Rest time measured (for OCV condition) in seconds */
} BMS_AfeSample_t;

/* Diagnostics structure exported to user callbacks */
typedef struct
{
    bool input_invalid;
    bool soc_valid;
    bool cell_imbalance_warning;
    bool current_sensor_fault;
    float pack_soc_percent;      /* 0.0 .. 100.0 */
    float ah_remaining;          /* AH remaining */
} BMS_SOC_Diagnostics_t;

/* Publish callback prototype used by ASW to emit SOC and diagnostics */
typedef void (*BMS_SOC_PublishCb_t)(const BMS_SOC_Diagnostics_t * diag);

/* Configuration type for SOC manager (application-level) */
typedef struct
{
    float pack_capacity_Ah;
    float ocv_table_voltages_V[BMS_SOC_OCV_TABLE_SIZE];
    float ocv_table_soc_pct[BMS_SOC_OCV_TABLE_SIZE];
    uint32_t ocv_table_size;
    BMS_SOC_PublishCb_t publish_cb;
} BMS_SOC_Manager_CfgType;

/* Initialize SOC manager. Must be called once at boot before other calls.
   cfg may be NULL to use default configuration defined in BMS_SOC_Manager_cfg.h */
MSM_StatusType BMS_SOC_Manager_Init(const BMS_SOC_Manager_CfgType * cfg);

/* Process an AFE immediate sample. This implements validation, initialization
   path decisions, operational trapezoidal integration, threshold detection,
   and persistent write request generation. */
MSM_StatusType BMS_SOC_Manager_ProcessSample(const BMS_AfeSample_t * sample);

/* Tick handler called periodically from main loop or an OS timer.
   elapsed_ms: milliseconds elapsed since last Tick call. Handles debouncing,
   rate-limits and NVM writes. */
void BMS_SOC_Manager_Tick(uint32_t elapsed_ms);

/* Request immediate persist (shutdown path). This will instruct the state
   machine to perform an immediate (synchronous) NVM write on next Tick call. */
MSM_StatusType BMS_SOC_Manager_RequestShutdownPersist(void);

/* Query latest diagnostics snapshot (non-blocking) */
MSM_StatusType BMS_SOC_Manager_GetDiagnostics(BMS_SOC_Diagnostics_t * out_diag);

/* Force factory reset of persisted storage (for diagnostics). */
MSM_StatusType BMS_SOC_Manager_FactoryResetPersisted(void);

#ifdef __cplusplus
}
#endif

#endif /* BMS_SOC_MANAGER_H */

