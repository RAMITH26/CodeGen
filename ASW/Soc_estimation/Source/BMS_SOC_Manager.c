
#include "BMS_SOC_Manager.h"
#include <string.h>
#include <math.h>

/* Internal persist payload stored in NVM via BMS_StationaryStorage (must be POD) */
typedef struct
{
    float pack_soc_percent;
    uint32_t last_persist_timestamp_s;
    uint32_t reserved;
} BMS_PersistData_t;

/* Internal state */
static BMS_SOC_Manager_CfgType BSM_Cfg;
static bool BSM_Initialized = false;
static MSM_StateType BSM_State = MSM_STATE_UNINITIALIZED;

/* SOC bookkeeping */
static float BSM_pack_capacity_Ah = 0.0F;
static float BSM_pack_cumulative_Ah = 0.0F;
static float BSM_pack_soc_percent = 50.0F;
static float BSM_I_prev_A = 0.0F;
static bool BSM_soc_valid = false;
static bool BSM_input_invalid = false;
static bool BSM_cell_imbalance_warning = false;
static bool BSM_current_sensor_fault = false;

/* Persist / debounce state */
static bool BSM_persist_request = false;
static uint32_t BSM_last_persist_timestamp_s = 0U;
static uint32_t BSM_debounce_timer_ms = 0U;
static uint32_t BSM_debounce_target_ms = 0U;
static float BSM_prev_soc_percent = 50.0F;
static uint32_t BSM_publish_tick_accum_ms = 0U;
static uint32_t BSM_rate_limit_ms = 10000U; /* 10s rate limit */

/* Helper prototypes */
static bool BSM_validate_sample(const BMS_AfeSample_t * sample);
static void BSM_dispatch_state_machine(const BMS_AfeSample_t * sample);
static void BSM_init_path_handle(const BMS_AfeSample_t * sample);
static void BSM_operational_path_handle(const BMS_AfeSample_t * sample);
static void BSM_persist_path_handle(const BMS_AfeSample_t * sample);
static void BSM_fault_path_handle(const BMS_AfeSample_t * sample);
static float BSM_lookup_ocv_soc(float voltage);
static float BSM_compute_stddev(const float * arr, uint32_t n);
static MSM_StatusType BSM_nvm_read_persist(BMS_PersistData_t * out);
static MSM_StatusType BSM_nvm_write_persist(const BMS_PersistData_t * in);
static void BSM_publish_now(void);
static void BSM_start_debounce(void);
static void BSM_clear_debounce(void);
static void BSM_check_threshold_crossing_and_request_persist(void);

/* Public API implementations */

MSM_StatusType BMS_SOC_Manager_Init(const BMS_SOC_Manager_CfgType * cfg)
{
    if (BSM_Initialized != false)
    {
        return MSM_OK;
    }

    if (cfg != NULL)
    {
        /* Copy configuration */
        (void)memcpy(&BSM_Cfg, cfg, sizeof(BSM_Cfg));
    }
    else
    {
        /* Use compile-time default config from cfg header */
        (void)memcpy(&BSM_Cfg, &BMS_SOC_Manager_CfgDefault, sizeof(BSM_Cfg));
    }

    /* Basic validation */
    if ((BSM_Cfg.ocv_table_size == 0U) || (BSM_Cfg.ocv_table_size > BMS_SOC_OCV_TABLE_SIZE) ||
        (BSM_Cfg.publish_cb == NULL) || (BSM_Cfg.pack_capacity_Ah <= 0.0F))
    {
        return MSM_INVALID_PARAM;
    }

    BSM_pack_capacity_Ah = BSM_Cfg.pack_capacity_Ah;
    BSM_pack_cumulative_Ah = (BSM_pack_capacity_Ah * 0.5F); /* default 50% until initialization completes */
    BSM_pack_soc_percent = 50.0F;
    BSM_prev_soc_percent = BSM_pack_soc_percent;
    BSM_I_prev_A = 0.0F;
    BSM_soc_valid = false;
    BSM_input_invalid = false;
    BSM_cell_imbalance_warning = false;
    BSM_current_sensor_fault = false;
    BSM_persist_request = false;
    BSM_debounce_timer_ms = 0U;
    BSM_debounce_target_ms = (10U * 1000U); /* 10 s debounce */
    BSM_publish_tick_accum_ms = 0U;
    BSM_last_persist_timestamp_s = 0U;

    /* Initialize underlying storage BSW with default configuration */
    {
        /* Use default storage config from BSW; pass NULL to use BSS_ConfigDefault */
        BSS_StatusType bss_ret = BMS_StationaryStorage_Init(NULL);
        if (bss_ret != BSS_OK)
        {
            return MSM_NVM_ERROR;
        }
    }

    BSM_State = MSM_STATE_INITIALIZATION;
    BSM_Initialized = true;
    return MSM_OK;
}

MSM_StatusType BMS_SOC_Manager_ProcessSample(const BMS_AfeSample_t * sample)
{
    if ((BSM_Initialized == false) || (sample == NULL))
    {
        return MSM_INVALID_PARAM;
    }

    /* Validate sample structure values (range checks) */
    BSM_input_invalid = !BSM_validate_sample(sample);

    /* Dispatch runtime state selection and handling */
    BSM_dispatch_state_machine(sample);

    return MSM_OK;
}

void BMS_SOC_Manager_Tick(uint32_t elapsed_ms)
{
    if (BSM_Initialized == false)
    {
        return;
    }

    /* Handle debounce timer for persist requests */
    if (BSM_debounce_timer_ms > 0U)
    {
        if (elapsed_ms >= BSM_debounce_timer_ms)
        {
            BSM_debounce_timer_ms = 0U;
        }
        else
        {
            BSM_debounce_timer_ms -= elapsed_ms;
        }
    }

    /* If persist requested and debounce satisfied or shutdown persistent requested, attempt NVM write */
    if (BSM_persist_request != false)
    {
        if (BSM_debounce_timer_ms == 0U)
        {
            /* Rate limit check */
            uint32_t now_s = 0U;
            /* For embedded system without RTC, we use HAL tick as seconds baseline */
            now_s = (uint32_t)(HAL_GetTick() / 1000U);
            if ((now_s - BSM_last_persist_timestamp_s) >= (BSM_rate_limit_ms / 1000U))
            {
                /* Prepare persist payload */
                BMS_PersistData_t payload;
                payload.pack_soc_percent = BSM_pack_soc_percent;
                payload.last_persist_timestamp_s = now_s;
                payload.reserved = 0U;

                if (BSM_nvm_write_persist(&payload) == MSM_OK)
                {
                    BSM_last_persist_timestamp_s = now_s;
                    BSM_persist_request = false;
                    BSM_clear_debounce();
                }
            }
            else
            {
                /* Defer due to rate limit: keep persist_request true */
            }
        }
    }

    /* Publish periodic tick checks (every 100 ms) */
    BSM_publish_tick_accum_ms += elapsed_ms;
    if (BSM_publish_tick_accum_ms >= BMS_SOC_PUBLISH_PERIOD_MS)
    {
        BSM_publish_tick_accum_ms = 0U;
        BSM_publish_now();
    }
}

MSM_StatusType BMS_SOC_Manager_RequestShutdownPersist(void)
{
    if (BSM_Initialized == false)
    {
        return MSM_INVALID_PARAM;
    }

    /* Immediate synchronous write: compose payload and call BSW save directly */
    BMS_PersistData_t payload;
    payload.pack_soc_percent = BSM_pack_soc_percent;
    payload.last_persist_timestamp_s = (uint32_t)(HAL_GetTick() / 1000U);
    payload.reserved = 0U;

    return BSM_nvm_write_persist(&payload);
}

MSM_StatusType BMS_SOC_Manager_GetDiagnostics(BMS_SOC_Diagnostics_t * out_diag)
{
    if ((BSM_Initialized == false) || (out_diag == NULL))
    {
        return MSM_INVALID_PARAM;
    }

    out_diag->input_invalid = BSM_input_invalid;
    out_diag->soc_valid = BSM_soc_valid;
    out_diag->cell_imbalance_warning = BSM_cell_imbalance_warning;
    out_diag->current_sensor_fault = BSM_current_sensor_fault;
    out_diag->pack_soc_percent = BSM_pack_soc_percent;
    out_diag->ah_remaining = (BSM_pack_capacity_Ah * BSM_pack_soc_percent) / 100.0F;
    return MSM_OK;
}

MSM_StatusType BMS_SOC_Manager_FactoryResetPersisted(void)
{
    if (BSM_Initialized == false)
    {
        return MSM_INVALID_PARAM;
    }

    BSS_StatusType st = BMS_StationaryStorage_FactoryReset();
    return (st == BSS_OK) ? MSM_OK : MSM_NVM_ERROR;
}

/* Internal helpers */

static bool BSM_validate_sample(const BMS_AfeSample_t * sample)
{
    uint32_t i;
    bool valid = true;

    if (sample->cell_count == 0U)
    {
        return false;
    }

    /* Validate sample interval against expected sample period with Â±10% tolerance */
    if (sample->sample_interval_ms == 0U)
    {
        valid = false;
    }
    else
    {
        uint32_t expected_ms = BMS_SOC_EXPECTED_SAMPLE_PERIOD_MS;
        uint32_t lower = (uint32_t)((100U - 10U) * expected_ms / 100U);
        uint32_t upper = (uint32_t)((100U + 10U) * expected_ms / 100U);
        if ((sample->sample_interval_ms < lower) || (sample->sample_interval_ms > upper))
        {
            valid = false;
        }
    }

    /* Validate each reported cell voltage is within allowed range */
    for (i = 0U; i < sample->cell_count; ++i)
    {
        float v = sample->cell_voltages_V[i];
        if ((v < BMS_CELL_VOLTAGE_MIN_V) || (v > BMS_CELL_VOLTAGE_MAX_V))
        {
            valid = false;
            break;
        }
    }

    return valid;
}

static void BSM_dispatch_state_machine(const BMS_AfeSample_t * sample)
{
    /* Decide runtime state selection based on current state and conditions */
    switch (BSM_State)
    {
        case MSM_STATE_INITIALIZATION:
            BSM_init_path_handle(sample);
            break;

        case MSM_STATE_OPERATIONAL:
            BSM_operational_path_handle(sample);
            break;

        case MSM_STATE_NVM_PERSIST:
            BSM_persist_path_handle(sample);
            break;

        case MSM_STATE_FAULT:
            BSM_fault_path_handle(sample);
            break;

        default:
            /* Unknown/uninitialized: transition to initialization path */
            BSM_State = MSM_STATE_INITIALIZATION;
            BSM_init_path_handle(sample);
            break;
    }
}

/* Initialization path handling */
static void BSM_init_path_handle(const BMS_AfeSample_t * sample)
{
    BMS_PersistData_t persisted;
    MSM_StatusType nvm_ret;

    /* Attempt to read persisted SOC from NVM */
    nvm_ret = BSM_nvm_read_persist(&persisted);

    /* Check OCV correction condition */
    if ((fabsf(sample->pack_current_A) < BMS_OCV_CURRENT_THRESHOLD_A) &&
        (sample->rest_duration_s >= BMS_OCV_REST_DURATION_S))
    {
        /* Compute per-cell OCV-based SOC and apply */
        uint32_t i;
        float per_cell_soc[BMS_SOC_MAX_CELLS];
        uint32_t cnt = (uint32_t)sample->cell_count;
        for (i = 0U; i < cnt; ++i)
        {
            per_cell_soc[i] = BSM_lookup_ocv_soc(sample->cell_voltages_V[i]);
        }

        /* Average */
        float sum = 0.0F;
        for (i = 0U; i < cnt; ++i)
        {
            sum += per_cell_soc[i];
        }
        float avg = (cnt > 0U) ? (sum / (float)cnt) : 50.0F;

        /* Apply */
        BSM_pack_soc_percent = avg;
        if (BSM_pack_soc_percent < 0.0F)
        {
            BSM_pack_soc_percent = 0.0F;
        }
        else if (BSM_pack_soc_percent > 100.0F)
        {
            BSM_pack_soc_percent = 100.0F;
        }

        BSM_pack_cumulative_Ah = (BSM_pack_capacity_Ah * BSM_pack_soc_percent) / 100.0F;

        BSM_soc_valid = true;
        BSM_I_prev_A = sample->pack_current_A;

        /* Stddev warning */
        {
            float stddev = BSM_compute_stddev(per_cell_soc, cnt);
            if (stddev > BMS_OCV_STDDEV_IMBALANCE_PCT)
            {
                BSM_cell_imbalance_warning = true;
            }
            else
            {
                BSM_cell_imbalance_warning = false;
            }
        }

        /* Completed initialization */
        BSM_State = MSM_STATE_OPERATIONAL;
        return;
    }

    /* OCV condition not met: if NVM has valid SOC, load persisted value */
    if (nvm_ret == MSM_OK)
    {
        BSM_pack_soc_percent = persisted.pack_soc_percent;
        if (BSM_pack_soc_percent < 0.0F)
        {
            BSM_pack_soc_percent = 0.0F;
        }
        else if (BSM_pack_soc_percent > 100.0F)
        {
            BSM_pack_soc_percent = 100.0F;
        }

        BSM_pack_cumulative_Ah = (BSM_pack_capacity_Ah * BSM_pack_soc_percent) / 100.0F;
        BSM_soc_valid = true;
        BSM_I_prev_A = sample->pack_current_A;
        BSM_State = MSM_STATE_OPERATIONAL;
        return;
    }

    /* No persisted SOC: default initialize */
    BSM_pack_soc_percent = 50.0F;
    BSM_pack_cumulative_Ah = (BSM_pack_capacity_Ah * 0.5F);
    BSM_soc_valid = false; /* as flowchart */
    BSM_I_prev_A = sample->pack_current_A;
    BSM_State = MSM_STATE_OPERATIONAL;
}

/* Operational path handling */
static void BSM_operational_path_handle(const BMS_AfeSample_t * sample)
{
    /* If input invalid -> suspend updates and publish diagnostics */
    if (BSM_input_invalid != false)
    {
        BSM_soc_valid = false;
        BSM_publish_now();
        return;
    }

    /* Trapezoidal integration */
    {
        float I_curr = sample->pack_current_A;
        float sample_dt_s = ((float)sample->sample_interval_ms) / 1000.0F;
        float I_avg = 0.5F * (BSM_I_prev_A + I_curr);
        float delta_Ah = (I_avg * sample_dt_s) / 3600.0F; /* A*s / 3600 = Ah */
        BSM_pack_cumulative_Ah += delta_Ah;
        if (BSM_pack_cumulative_Ah < 0.0F)
        {
            BSM_pack_cumulative_Ah = 0.0F;
        }
        /* Compute SOC and clamp */
        BSM_pack_soc_percent = (100.0F * BSM_pack_cumulative_Ah) / BSM_pack_capacity_Ah;
        if (BSM_pack_soc_percent < 0.0F)
        {
            BSM_pack_soc_percent = 0.0F;
        }
        else if (BSM_pack_soc_percent > 100.0F)
        {
            BSM_pack_soc_percent = 100.0F;
        }
        BSM_I_prev_A = I_curr;
        BSM_soc_valid = true;
    }

    /* Current sensor fault detection: simple heuristic - large change between consecutive samples */
    {
        float deltaI = fabsf(BSM_I_prev_A - sample->pack_current_A);
        if (deltaI > BMS_CURRENT_SENSOR_DELTA_THRESHOLD_A)
        {
            BSM_current_sensor_fault = true;
            BSM_soc_valid = false;
            BSM_publish_now();
            return;
        }
        else
        {
            BSM_current_sensor_fault = false;
        }
    }

    /* Detect threshold crossings and start debounce */
    BSM_check_threshold_crossing_and_request_persist();

    /* Publish tick handled by Tick(); but we still may publish diagnostics here on demand */
}

/* NVM persist path handling (not used as dedicated state, writes are performed from Tick) */
static void BSM_persist_path_handle(const BMS_AfeSample_t * sample)
{
    (void)sample;
    /* Persist handled asynchronously in Tick; nothing here */
}

/* Fault path handling */
static void BSM_fault_path_handle(const BMS_AfeSample_t * sample)
{
    (void)sample;
    BSM_input_invalid = true;
    BSM_soc_valid = false;
    BSM_publish_now();
}

/* Lookup OCV->SoC by linear interpolation.
   Table arrays are expected to be of ocv_table_size and monotonic (increasing voltage). */
static float BSM_lookup_ocv_soc(float voltage)
{
    uint32_t i;
    uint32_t n = BSM_Cfg.ocv_table_size;

    if (n == 0U)
    {
        return 50.0F;
    }

    /* Clamp outside range */
    if (voltage <= BSM_Cfg.ocv_table_voltages_V[0])
    {
        return BSM_Cfg.ocv_table_soc_pct[0];
    }
    if (voltage >= BSM_Cfg.ocv_table_voltages_V[n - 1U])
    {
        return BSM_Cfg.ocv_table_soc_pct[n - 1U];
    }

    /* Find interval */
    for (i = 1U; i < n; ++i)
    {
        float v0 = BSM_Cfg.ocv_table_voltages_V[i - 1U];
        float v1 = BSM_Cfg.ocv_table_voltages_V[i];
        if ((voltage >= v0) && (voltage <= v1))
        {
            float s0 = BSM_Cfg.ocv_table_soc_pct[i - 1U];
            float s1 = BSM_Cfg.ocv_table_soc_pct[i];
            float t = (voltage - v0) / (v1 - v0);
            return (s0 + t * (s1 - s0));
        }
    }

    /* Fallback */
    return 50.0F;
}

/* Compute standard deviation (percentage points) of array */
static float BSM_compute_stddev(const float * arr, uint32_t n)
{
    uint32_t i;
    if ((arr == NULL) || (n == 0U))
    {
        return 0.0F;
    }

    float sum = 0.0F;
    for (i = 0U; i < n; ++i)
    {
        sum += arr[i];
    }
    float mean = sum / (float)n;
    float var = 0.0F;
    for (i = 0U; i < n; ++i)
    {
        float d = arr[i] - mean;
        var += d * d;
    }
    var = var / (float)n;
    return sqrtf(var);
}

/* Read persisted data via BSW LoadConfig (NVM Read wrapper) */
static MSM_StatusType BSM_nvm_read_persist(BMS_PersistData_t * out)
{
    if (out == NULL)
    {
        return MSM_INVALID_PARAM;
    }

    uint32_t outLen = 0U;
    BSS_StatusType st = BMS_StationaryStorage_LoadConfig((void *)out, &outLen);
    if (st == BSS_OK)
    {
        /* Basic sanity check of payload size */
        if (outLen < sizeof(BMS_PersistData_t))
        {
            return MSM_NO_PERSISTED_SOC;
        }
        return MSM_OK;
    }

    if (st == BSS_NO_VALID_STORAGE)
    {
        return MSM_NO_PERSISTED_SOC;
    }

    return MSM_NVM_ERROR;
}

/* Write persisted data via BSW SaveConfig (NVM Write wrapper) */
static MSM_StatusType BSM_nvm_write_persist(const BMS_PersistData_t * in)
{
    if (in == NULL)
    {
        return MSM_INVALID_PARAM;
    }

    BSS_StatusType st = BMS_StationaryStorage_SaveConfig((const void *)in, sizeof(BMS_PersistData_t));
    return (st == BSS_OK) ? MSM_OK : MSM_NVM_ERROR;
}

/* Publish SOC and diagnostics via configured callback */
static void BSM_publish_now(void)
{
    if (BSM_Cfg.publish_cb == NULL)
    {
        return;
    }

    BMS_SOC_Diagnostics_t diag;
    (void)BMS_SOC_Manager_GetDiagnostics(&diag);
    BSM_Cfg.publish_cb(&diag);
}

/* Debounce helpers */
static void BSM_start_debounce(void)
{
    BSM_debounce_timer_ms = BSM_debounce_target_ms;
}

static void BSM_clear_debounce(void)
{
    BSM_debounce_timer_ms = 0U;
}

/* Detect threshold crossing set {0,25,50,75,100}% */
static void BSM_check_threshold_crossing_and_request_persist(void)
{
    const float thresholds[] = { 0.0F, 25.0F, 50.0F, 75.0F, 100.0F };
    const uint32_t num_thresh = (sizeof(thresholds) / sizeof(thresholds[0]));
    uint32_t i;

    for (i = 0U; i < num_thresh; ++i)
    {
        float thr = thresholds[i];
        /* Crossed upward or downward since last sample */
        if (((BSM_prev_soc_percent < thr) && (BSM_pack_soc_percent >= thr)) ||
            ((BSM_prev_soc_percent > thr) && (BSM_pack_soc_percent <= thr)))
        {
            /* Start debounce if not already active */
            if (BSM_debounce_timer_ms == 0U)
            {
                BSM_start_debounce();
            }
            /* When debounce completes, set persist_request */
            if (BSM_debounce_timer_ms == 0U)
            {
                /* Debounce satisfied; set persist request and record last persist timestamp (rate-limited in Tick) */
                BSM_persist_request = true;
            }
            break; /* one threshold at a time */
        }
    }

    BSM_prev_soc_percent = BSM_pack_soc_percent;
}

/* End of file */

