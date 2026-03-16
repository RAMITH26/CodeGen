
#include "balancing_asw.h"

#include <string.h> /* memset */

/* Weak BMS state definition (can be overridden by application) */
volatile uint8_t BMS_State __attribute__((weak)) = 0u;

/* Exported diagnostics/status variables */
Bal_CellStatus_t Bal_Status[NUM_CELLS];
AFE_BleedState_t Bal_BleedState[NUM_CELLS];
volatile bool Balancing_Active = false;
volatile uint8_t Parity_Flag = 0u;
volatile uint16_t Last_Max_mV = 0u;
volatile uint16_t Last_Min_mV = 0u;
volatile uint16_t Last_Diff_mV = 0u;

/* Internal working buffer for voltages */
static uint16_t s_cell_voltages[NUM_CELLS];

/* Forward declarations of internal helpers */
static void compute_min_max_diff(const uint16_t voltages[], size_t count,
                                 uint16_t *min_v, uint16_t *max_v, uint16_t *diff_v);
static void branch_not_charging(const uint16_t voltages[], size_t count,
                                uint16_t min_v, uint16_t max_v, uint16_t diff_v);
static void branch_stop(const uint16_t voltages[], size_t count,
                        uint16_t min_v, uint16_t max_v, uint16_t diff_v);
static void branch_hysteresis(const uint16_t voltages[], size_t count,
                              uint16_t min_v, uint16_t max_v, uint16_t diff_v);
static void branch_active(const uint16_t voltages[], size_t count,
                          uint16_t min_v, uint16_t max_v, uint16_t diff_v);

/* Weak stub implementations for BSW functions to allow linking in absence of BSW.
   Production integration should provide real implementations (no weak attribute).
*/
BSW_Status_t AFE_ReadCellVoltages(uint16_t voltages[], size_t count) __attribute__((weak));
BSW_Status_t AFE_ReadCellVoltages(uint16_t voltages[], size_t count)
{
    /* Weak stub: indicate error to avoid unintended balancing if BSW not linked.
       Concrete platform must provide real implementation.
    */
    (void)voltages;
    (void)count;
    return BSW_STATUS_ERROR;
}

BSW_Status_t AFE_SetBleed(uint8_t cell_index, AFE_BleedState_t state) __attribute__((weak));
BSW_Status_t AFE_SetBleed(uint8_t cell_index, AFE_BleedState_t state)
{
    (void)cell_index;
    (void)state;
    return BSW_STATUS_ERROR;
}

/* Initialize balancing module */
void Bal_ASW_Init(void)
{
    size_t idx;

    /* Initialize diagnostics and bleed state */
    for (idx = 0u; idx < (size_t)NUM_CELLS; ++idx)
    {
        Bal_Status[idx].bleed_error = false;
        Bal_BleedState[idx] = AFE_BLEED_OFF;
    }

    Balancing_Active = false;
    Parity_Flag = 0u;
    Last_Max_mV = 0u;
    Last_Min_mV = 0u;
    Last_Diff_mV = 0u;
    memset(s_cell_voltages, 0, sizeof(s_cell_voltages));
}

/* Main periodic execution of balancing logic (one scheduler tick) */
void Bal_ASW_RunTick(void)
{
    uint16_t min_v;
    uint16_t max_v;
    uint16_t diff_v;
    BSW_Status_t ret;

    /* Read cell voltages */
    ret = AFE_ReadCellVoltages(s_cell_voltages, (size_t)NUM_CELLS);
    if (ret != BSW_STATUS_OK)
    {
        /* Reading voltages failed - record diagnostics and abort this cycle */
        /* Mark all cells with read error via bleed_error flag as indication */
        size_t i;
        for (i = 0u; i < (size_t)NUM_CELLS; ++i)
        {
            Bal_Status[i].bleed_error = true;
        }
        return;
    }

    /* Compute min, max and difference */
    compute_min_max_diff(s_cell_voltages, (size_t)NUM_CELLS, &min_v, &max_v, &diff_v);

    /* Dispatch according to BMS state and diff thresholds */
    if (BMS_State != (uint8_t)BMS_STATE_CHARGING)
    {
        /* NOT_CHARGING branch */
        branch_not_charging(s_cell_voltages, (size_t)NUM_CELLS, min_v, max_v, diff_v);
    }
    else if (diff_v <= (uint16_t)BAL_STOP_DIFF_MV)
    {
        /* STOPPED branch */
        branch_stop(s_cell_voltages, (size_t)NUM_CELLS, min_v, max_v, diff_v);
    }
    else if (diff_v > (uint16_t)BAL_START_DIFF_MV)
    {
        /* ACTIVE branch */
        branch_active(s_cell_voltages, (size_t)NUM_CELLS, min_v, max_v, diff_v);
    }
    else
    {
        /* HYSTERESIS branch */
        branch_hysteresis(s_cell_voltages, (size_t)NUM_CELLS, min_v, max_v, diff_v);
    }
}

/* Helper: compute min, max and diff */
static void compute_min_max_diff(const uint16_t voltages[], size_t count,
                                 uint16_t *min_v, uint16_t *max_v, uint16_t *diff_v)
{
    size_t i;
    uint16_t local_min;
    uint16_t local_max;

    if ((voltages == NULL) || (count == 0u) || (min_v == NULL) || (max_v == NULL) || (diff_v == NULL))
    {
        /* Invalid parameters: set zeros */
        if (min_v != NULL) { *min_v = 0u; }
        if (max_v != NULL) { *max_v = 0u; }
        if (diff_v != NULL) { *diff_v = 0u; }
        return;
    }

    local_min = voltages[0];
    local_max = voltages[0];

    for (i = 1u; i < count; ++i)
    {
        if (voltages[i] < local_min)
        {
            local_min = voltages[i];
        }
        if (voltages[i] > local_max)
        {
            local_max = voltages[i];
        }
    }

    *min_v = local_min;
    *max_v = local_max;
    if (local_max >= local_min)
    {
        *diff_v = (uint16_t)(local_max - local_min);
    }
    else
    {
        /* Defensive: should not happen with unsigned, but keep safe */
        *diff_v = 0u;
    }
}

/* NOT_CHARGING branch implementation:
   Turn off any active bleeds and update diagnostics/status
*/
static void branch_not_charging(const uint16_t voltages[], size_t count,
                                uint16_t min_v, uint16_t max_v, uint16_t diff_v)
{
    size_t i;
    BSW_Status_t ret;

    (void)voltages;
    (void)min_v;
    (void)max_v;
    (void)diff_v;

    for (i = 0u; i < count; ++i)
    {
        if (Bal_BleedState[i] == AFE_BLEED_ON)
        {
            ret = AFE_SetBleed((uint8_t)i, AFE_BLEED_OFF);
            if (ret == BSW_STATUS_OK)
            {
                Bal_BleedState[i] = AFE_BLEED_OFF;
                Bal_Status[i].bleed_error = false;
            }
            else
            {
                Bal_Status[i].bleed_error = true;
            }
        }
    }

    /* Update status and diagnostics */
    Balancing_Active = false;
    Last_Min_mV = min_v;
    Last_Max_mV = max_v;
    Last_Diff_mV = diff_v;
    /* Parity flag unchanged per flowchart */
}

/* STOPPED branch: similar to NOT_CHARGING - ensure bleeds are off */
static void branch_stop(const uint16_t voltages[], size_t count,
                        uint16_t min_v, uint16_t max_v, uint16_t diff_v)
{
    size_t i;
    BSW_Status_t ret;

    (void)voltages;

    for (i = 0u; i < count; ++i)
    {
        if (Bal_BleedState[i] == AFE_BLEED_ON)
        {
            ret = AFE_SetBleed((uint8_t)i, AFE_BLEED_OFF);
            if (ret == BSW_STATUS_OK)
            {
                Bal_BleedState[i] = AFE_BLEED_OFF;
                Bal_Status[i].bleed_error = false;
            }
            else
            {
                Bal_Status[i].bleed_error = true;
            }
        }
    }

    Balancing_Active = false;
    Last_Min_mV = min_v;
    Last_Max_mV = max_v;
    Last_Diff_mV = diff_v;
}

/* HYSTERESIS branch: act only if balancing_active == true */
static void branch_hysteresis(const uint16_t voltages[], size_t count,
                              uint16_t min_v, uint16_t max_v, uint16_t diff_v)
{
    size_t i;
    uint16_t select_threshold;
    size_t on_count;
    BSW_Status_t ret;

    if (Balancing_Active == false)
    {
        /* Idle: no action, maintain parity_flag unchanged */
        /* Update diagnostics for inactive state if required */
        Last_Min_mV = min_v;
        Last_Max_mV = max_v;
        Last_Diff_mV = diff_v;
        return;
    }

    /* Compute select threshold safely */
    if (max_v > (uint16_t)SEL_MARGIN_MV)
    {
        select_threshold = (uint16_t)(max_v - (uint16_t)SEL_MARGIN_MV);
    }
    else
    {
        select_threshold = 0u;
    }

    on_count = 0u;

    for (i = 0u; i < count; ++i)
    {
        bool is_candidate = (voltages[i] >= select_threshold);
        bool parity_match = (uint8_t)(i & 1u) == Parity_Flag;
        AFE_BleedState_t desired_state = AFE_BLEED_OFF;

        if (is_candidate != false)
        {
            if (parity_match != false)
            {
                desired_state = AFE_BLEED_ON;
            }
            else
            {
                desired_state = AFE_BLEED_OFF;
            }
        }
        else
        {
            desired_state = AFE_BLEED_OFF;
        }

        /* Only call BSW if desired state differs from current */
        if (Bal_BleedState[i] != desired_state)
        {
            ret = AFE_SetBleed((uint8_t)i, desired_state);
            if (ret == BSW_STATUS_OK)
            {
                Bal_BleedState[i] = desired_state;
                Bal_Status[i].bleed_error = false;
            }
            else
            {
                /* Record error but continue loop */
                Bal_Status[i].bleed_error = true;
            }
        }

        if (Bal_BleedState[i] == AFE_BLEED_ON)
        {
            ++on_count;
        }
    }

    /* Post-loop updates */
    Balancing_Active = true;
    Last_Min_mV = min_v;
    Last_Max_mV = max_v;
    Last_Diff_mV = diff_v;
    /* Diagnostics: parity_flag and on_count are available if needed */
    (void)on_count;

    /* Toggle parity for next cycle */
    Parity_Flag = (uint8_t)(1u - Parity_Flag);
}

/* ACTIVE branch: select and apply bleeds aggressively */
static void branch_active(const uint16_t voltages[], size_t count,
                          uint16_t min_v, uint16_t max_v, uint16_t diff_v)
{
    size_t i;
    uint16_t select_threshold;
    size_t on_count;
    BSW_Status_t ret;

    /* Compute select threshold safely */
    if (max_v > (uint16_t)SEL_MARGIN_MV)
    {
        select_threshold = (uint16_t)(max_v - (uint16_t)SEL_MARGIN_MV);
    }
    else
    {
        select_threshold = 0u;
    }

    on_count = 0u;

    for (i = 0u; i < count; ++i)
    {
        bool is_candidate = (voltages[i] >= select_threshold);
        bool parity_match = (uint8_t)(i & 1u) == Parity_Flag;
        AFE_BleedState_t desired_state = AFE_BLEED_OFF;

        if (is_candidate != false)
        {
            if (parity_match != false)
            {
                desired_state = AFE_BLEED_ON;
            }
            else
            {
                desired_state = AFE_BLEED_OFF;
            }
        }
        else
        {
            desired_state = AFE_BLEED_OFF;
        }

        /* Only call BSW if desired state differs from current */
        if (Bal_BleedState[i] != desired_state)
        {
            ret = AFE_SetBleed((uint8_t)i, desired_state);
            if (ret == BSW_STATUS_OK)
            {
                Bal_BleedState[i] = desired_state;
                Bal_Status[i].bleed_error = false;
            }
            else
            {
                Bal_Status[i].bleed_error = true;
            }
        }

        if (Bal_BleedState[i] == AFE_BLEED_ON)
        {
            ++on_count;
        }
    }

    /* Post-loop updates */
    Balancing_Active = true;
    Last_Min_mV = min_v;
    Last_Max_mV = max_v;
    Last_Diff_mV = diff_v;
    (void)on_count;

    /* Toggle parity for next cycle */
    Parity_Flag = (uint8_t)(1u - Parity_Flag);
}

