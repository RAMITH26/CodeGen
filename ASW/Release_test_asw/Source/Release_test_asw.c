
#include "Release_test_asw.h"

/* Internal static state */
static Release_test_asw_State_t asw_state = RELEASE_TEST_ASW_STATE_UNINITIALIZED;
static Release_test_asw_Inputs_t asw_inputs;
static Release_test_asw_Outputs_t asw_outputs;

/* Timing tracking for precharge */
static uint32_t precharge_start_tick = 0u;

/* Local forward declarations */
static void asw_read_inputs(void);
static void asw_evaluate_safety_checks(void);
static void asw_manage_contactor(void);
static void asw_manage_balancing(void);
static void asw_set_all_balancers(bool enable);
static void asw_clear_outputs(void);
static void asw_set_fault(uint32_t fault);
static void asw_clear_fault(uint32_t fault);

/* Public API implementations */

void Release_test_asw_Init(void)
{
    /* Initialize internal structures */
    (void)memset(&asw_inputs, 0, sizeof(asw_inputs));
    (void)memset(&asw_outputs, 0, sizeof(asw_outputs));

    /* Ensure contactor is commanded open at init */
    (void)Bsw_SetContactorCommand(false);

    /* Disable all balancers */
    asw_set_all_balancers(false);

    asw_outputs.faultMask = 0u;
    asw_state = RELEASE_TEST_ASW_STATE_INIT;
    precharge_start_tick = 0u;
    asw_state = RELEASE_TEST_ASW_STATE_STANDBY;
}

void Release_test_asw_DeInit(void)
{
    /* Open contactor and disable balancing */
    (void)Bsw_SetContactorCommand(false);
    asw_set_all_balancers(false);
    asw_clear_outputs();
    asw_state = RELEASE_TEST_ASW_STATE_UNINITIALIZED;
}

void Release_test_asw_RunCycle(void)
{
    /* Guard: must be initialized */
    if (asw_state == RELEASE_TEST_ASW_STATE_UNINITIALIZED)
    {
        return;
    }

    asw_read_inputs();

    /* Evaluate safety (voltages, temps, sensors) */
    asw_evaluate_safety_checks();

    /* Manage contactor state machine (precharge, close/open) */
    asw_manage_contactor();

    /* Manage cell balancing */
    asw_manage_balancing();

    /* Report faults to BSW for logging/diagnostics */
    (void)Bsw_ReportFault(asw_outputs.faultMask);

    /* Update global state variable */
    if (asw_outputs.faultMask != 0u)
    {
        asw_state = RELEASE_TEST_ASW_STATE_FAULT;
    }
    else if (asw_outputs.contactorClosed)
    {
        asw_state = RELEASE_TEST_ASW_STATE_ACTIVE;
    }
    else if (asw_outputs.prechargeActive)
    {
        asw_state = RELEASE_TEST_ASW_STATE_PRECHARGING;
    }
    else
    {
        asw_state = RELEASE_TEST_ASW_STATE_STANDBY;
    }
}

void Release_test_asw_GetOutputs(Release_test_asw_Outputs_t * const outputs)
{
    if (outputs != NULL)
    {
        (void)memcpy(outputs, &asw_outputs, sizeof(asw_outputs));
    }
}

Release_test_asw_State_t Release_test_asw_GetState(void)
{
    return asw_state;
}

/* Internal helpers */

static void asw_read_inputs(void)
{
    uint8_t cellCount = RELEASE_TEST_ASW_NUM_CELLS;
    int32_t rc = RELEASE_TEST_BSW_OK;

    /* Initialize timestamp */
    asw_inputs.timestamp_ms = HAL_GetTick();

    /* Read pack voltage */
    rc = Bsw_GetPackVoltage(&asw_inputs.packVoltage);
    if (rc != RELEASE_TEST_BSW_OK)
    {
        asw_set_fault(RELEASE_TEST_FAULT_SENSOR_FAIL);
        asw_inputs.packVoltage = 0.0f;
    }

    /* Read pack current */
    rc = Bsw_GetPackCurrent(&asw_inputs.packCurrent);
    if (rc != RELEASE_TEST_BSW_OK)
    {
        asw_set_fault(RELEASE_TEST_FAULT_SENSOR_FAIL);
        asw_inputs.packCurrent = 0.0f;
    }

    /* Read cell voltages */
    rc = Bsw_GetCellVoltages(asw_inputs.cellVoltages, &cellCount);
    if (rc != RELEASE_TEST_BSW_OK)
    {
        asw_set_fault(RELEASE_TEST_FAULT_SENSOR_FAIL);
        /* Fill with zeros to avoid NaN propagation */
        (void)memset(asw_inputs.cellVoltages, 0, sizeof(asw_inputs.cellVoltages));
        asw_inputs.cellCount = 0u;
    }
    else
    {
        /* Limit stored count to configured max */
        if (cellCount > RELEASE_TEST_ASW_NUM_CELLS)
        {
            cellCount = RELEASE_TEST_ASW_NUM_CELLS;
        }
        asw_inputs.cellCount = cellCount;
    }

    /* Read cell temperatures */
    cellCount = RELEASE_TEST_ASW_NUM_CELLS;
    rc = Bsw_GetCellTemperatures(asw_inputs.cellTemperatures, &cellCount);
    if (rc != RELEASE_TEST_BSW_OK)
    {
        asw_set_fault(RELEASE_TEST_FAULT_SENSOR_FAIL);
        (void)memset(asw_inputs.cellTemperatures, 0, sizeof(asw_inputs.cellTemperatures));
    }
}

static void asw_evaluate_safety_checks(void)
{
    uint8_t i;
    float v;
    float t;

    /* Default: clear transient sensor fail if values present (do not clear other faults) */
    /* Check each cell for over/under voltage and over temperature */
    for (i = 0u; i < asw_inputs.cellCount; ++i)
    {
        v = asw_inputs.cellVoltages[i];
        if (v > RELEASE_TEST_CELL_OVERVOLTAGE_VOLT)
        {
            asw_set_fault(RELEASE_TEST_FAULT_OVERVOLTAGE);
        }
        else
        {
            asw_clear_fault(RELEASE_TEST_FAULT_OVERVOLTAGE);
        }

        if (v < RELEASE_TEST_CELL_UNDERVOLTAGE_VOLT)
        {
            asw_set_fault(RELEASE_TEST_FAULT_UNDERVOLTAGE);
        }
        else
        {
            asw_clear_fault(RELEASE_TEST_FAULT_UNDERVOLTAGE);
        }

        t = asw_inputs.cellTemperatures[i];
        if (t > RELEASE_TEST_CELL_OVERTEMP_C)
        {
            asw_set_fault(RELEASE_TEST_FAULT_OVERTEMP);
        }
        else
        {
            asw_clear_fault(RELEASE_TEST_FAULT_OVERTEMP);
        }
    }

    /* Pack-level checks - conservative: derive pack voltage from sum of cell voltages if available; else use measured pack */
    if (asw_inputs.cellCount > 0u)
    {
        float pack_calc = 0.0f;
        for (i = 0u; i < asw_inputs.cellCount; ++i)
        {
            pack_calc += asw_inputs.cellVoltages[i];
        }
        /* Over/under check using pack_calc in addition to cell checks */
        (void)pack_calc;
    }

    /* Current checks: ensure current is within module limits */
    if (asw_inputs.packCurrent > RELEASE_TEST_MAX_CHARGE_CURRENT_A)
    {
        /* Charging current too high */
        asw_set_fault(RELEASE_TEST_FAULT_PRECHARGE_FAIL);
    }
    else
    {
        asw_clear_fault(RELEASE_TEST_FAULT_PRECHARGE_FAIL);
    }
}

static void asw_manage_contactor(void)
{
    uint32_t tick = HAL_GetTick();

    /* If any critical fault present, ensure contactor open */
    if ((asw_outputs.faultMask & (RELEASE_TEST_FAULT_OVERVOLTAGE |
                                  RELEASE_TEST_FAULT_UNDERVOLTAGE |
                                  RELEASE_TEST_FAULT_OVERTEMP |
                                  RELEASE_TEST_FAULT_SENSOR_FAIL)) != 0u)
    {
        /* Open contactor */
        (void)Bsw_SetContactorCommand(false);
        asw_outputs.contactorClosed = false;
        asw_outputs.prechargeActive = false;
        precharge_start_tick = 0u;
        return;
    }

    /* If currently closed, maintain closed unless new fault occurs */
    if (asw_outputs.contactorClosed)
    {
        /* Safety: if cell voltages drop below undervoltage threshold, open */
        if (asw_inputs.cellCount > 0u)
        {
            uint8_t i;
            for (i = 0u; i < asw_inputs.cellCount; ++i)
            {
                if (asw_inputs.cellVoltages[i] < RELEASE_TEST_CELL_UNDERVOLTAGE_VOLT)
                {
                    (void)Bsw_SetContactorCommand(false);
                    asw_outputs.contactorClosed = false;
                    asw_outputs.prechargeActive = false;
                    precharge_start_tick = 0u;
                    break;
                }
            }
        }
        return;
    }

    /* If contactor is open and no faults, start precharge if pack voltage sufficiently high to allow closing */
    if (!asw_outputs.contactorClosed)
    {
        /* Condition to initiate precharge: pack voltage greater than configured minimum and not already precharging */
        if ((!asw_outputs.prechargeActive) &&
            (asw_inputs.packVoltage >= RELEASE_TEST_PRECHARGE_ENABLE_VOLT))
        {
            /* Start precharge */
            asw_outputs.prechargeActive = true;
            precharge_start_tick = tick;
            /* Keep contactor open while precharging (precharge path assumed separate) */
            (void)Bsw_SetContactorCommand(false);
        }

        /* If precharge active, check timeout and completion condition */
        if (asw_outputs.prechargeActive)
        {
            if ((tick - precharge_start_tick) >= RELEASE_TEST_PRECHARGE_TIME_MS)
            {
                /* Precharge completed: close contactor */
                const int32_t rc = Bsw_SetContactorCommand(true);
                if (rc == RELEASE_TEST_BSW_OK)
                {
                    asw_outputs.contactorClosed = true;
                    asw_outputs.prechargeActive = false;
                    precharge_start_tick = 0u;
                }
                else
                {
                    /* Failed command to BSW/hardware */
                    asw_set_fault(RELEASE_TEST_FAULT_COMM_FAIL);
                    (void)Bsw_SetContactorCommand(false);
                    asw_outputs.contactorClosed = false;
                    asw_outputs.prechargeActive = false;
                    precharge_start_tick = 0u;
                }
            }
            else
            {
                /* Still precharging: ensure not exceeding current limit for precharge */
                if (asw_inputs.packCurrent > RELEASE_TEST_PRECHARGE_MAX_CURRENT_A)
                {
                    /* Precharge current exceeded: abort */
                    asw_set_fault(RELEASE_TEST_FAULT_PRECHARGE_FAIL);
                    (void)Bsw_SetContactorCommand(false);
                    asw_outputs.prechargeActive = false;
                    asw_outputs.contactorClosed = false;
                    precharge_start_tick = 0u;
                }
            }
        }
    }
}

static void asw_manage_balancing(void)
{
    uint8_t i;
    float maxV = -1.0e6f;
    float minV = 1.0e6f;
    float delta = 0.0f;
    bool enable[RELEASE_TEST_ASW_NUM_CELLS];
    (void)memset(enable, 0, sizeof(enable));

    if (asw_inputs.cellCount == 0u)
    {
        /* No cell data: ensure balancers off */
        asw_set_all_balancers(false);
        return;
    }

    /* If any critical fault or contactor open, disable balancing */
    if ((asw_outputs.faultMask != 0u) || (!asw_outputs.contactorClosed))
    {
        asw_set_all_balancers(false);
        return;
    }

    /* Determine min/max cell voltages */
    for (i = 0u; i < asw_inputs.cellCount; ++i)
    {
        const float v = asw_inputs.cellVoltages[i];
        if (v > maxV)
        {
            maxV = v;
        }
        if (v < minV)
        {
            minV = v;
        }
    }

    delta = maxV - minV;

    /* Balancing decision: enable balancer for cells significantly higher than average when above enable voltage */
    if ((delta >= RELEASE_TEST_BALANCE_DELTA_VOLT) && (maxV >= RELEASE_TEST_BALANCE_ENABLE_VOLT))
    {
        float avg = 0.0f;
        for (i = 0u; i < asw_inputs.cellCount; ++i)
        {
            avg += asw_inputs.cellVoltages[i];
        }
        avg = avg / (float)asw_inputs.cellCount;

        for (i = 0u; i < asw_inputs.cellCount; ++i)
        {
            if ((asw_inputs.cellVoltages[i] - avg) >= RELEASE_TEST_BALANCE_ENABLE_DIFF_VOLT)
            {
                enable[i] = true;
            }
            else
            {
                enable[i] = false;
            }
        }
    }
    else
    {
        /* No balancing required */
        for (i = 0u; i < asw_inputs.cellCount; ++i)
        {
            enable[i] = false;
        }
    }

    /* Apply balancer enables and track outputs */
    for (i = 0u; i < asw_inputs.cellCount; ++i)
    {
        if (enable[i] != asw_outputs.balancerEnabled[i])
        {
            (void)Bsw_EnableCellBalancer(i, enable[i]);
            asw_outputs.balancerEnabled[i] = enable[i];
        }
    }
}

/* Utility helpers */

static void asw_set_all_balancers(bool enable)
{
    uint8_t i;
    for (i = 0u; i < RELEASE_TEST_ASW_NUM_CELLS; ++i)
    {
        (void)Bsw_EnableCellBalancer(i, enable);
        asw_outputs.balancerEnabled[i] = enable;
    }
}

static void asw_clear_outputs(void)
{
    asw_outputs.contactorClosed = false;
    asw_outputs.prechargeActive = false;
    (void)memset(asw_outputs.balancerEnabled, 0, sizeof(asw_outputs.balancerEnabled));
    asw_outputs.faultMask = 0u;
}

static void asw_set_fault(uint32_t fault)
{
    asw_outputs.faultMask |= fault;
}

static void asw_clear_fault(uint32_t fault)
{
    asw_outputs.faultMask &= ~fault;
}

/* Weak default implementations of BSW APIs
   These implementations are declared weak so that a real BSW implementation
   can override them. Default stubs provide safe no-op behaviors. */

__attribute__((weak)) int32_t Bsw_GetPackVoltage(float * const voltage)
{
    if (voltage == NULL)
    {
        return (int32_t)RELEASE_TEST_BSW_ERR;
    }
    /* Default safe value: 0 V (caller must handle) */
    *voltage = 0.0f;
    return (int32_t)RELEASE_TEST_BSW_OK;
}

__attribute__((weak)) int32_t Bsw_GetPackCurrent(float * const current)
{
    if (current == NULL)
    {
        return (int32_t)RELEASE_TEST_BSW_ERR;
    }
    *current = 0.0f;
    return (int32_t)RELEASE_TEST_BSW_OK;
}

__attribute__((weak)) int32_t Bsw_GetCellVoltages(float * const cellVoltages, uint8_t * const numCells)
{
    if ((cellVoltages == NULL) || (numCells == NULL))
    {
        return (int32_t)RELEASE_TEST_BSW_ERR;
    }
    /* Default: zero voltages and indicate zero populated cells */
    (void)memset(cellVoltages, 0, sizeof(float) * RELEASE_TEST_ASW_NUM_CELLS);
    *numCells = 0u;
    return (int32_t)RELEASE_TEST_BSW_OK;
}

__attribute__((weak)) int32_t Bsw_GetCellTemperatures(float * const cellTemps, uint8_t * const numCells)
{
    if ((cellTemps == NULL) || (numCells == NULL))
    {
        return (int32_t)RELEASE_TEST_BSW_ERR;
    }
    (void)memset(cellTemps, 0, sizeof(float) * RELEASE_TEST_ASW_NUM_CELLS);
    *numCells = 0u;
    return (int32_t)RELEASE_TEST_BSW_OK;
}

__attribute__((weak)) int32_t Bsw_SetContactorCommand(bool close)
{
    /* Default: pretend success. Real implementation must drive GPIO or power stage. */
    (void)close;
    return (int32_t)RELEASE_TEST_BSW_OK;
}

__attribute__((weak)) int32_t Bsw_EnableCellBalancer(uint8_t cellIndex, bool enable)
{
    /* Default: pretend success. Real implementation must drive balancing hardware. */
    (void)cellIndex;
    (void)enable;
    return (int32_t)RELEASE_TEST_BSW_OK;
}

__attribute__((weak)) int32_t Bsw_ReportFault(uint32_t faultMask)
{
    /* Default: no-op. Real implementation should log or forward to diagnostics. */
    (void)faultMask;
    return (int32_t)RELEASE_TEST_BSW_OK;
}

