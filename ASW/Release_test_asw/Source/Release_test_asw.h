/* ===== File: Release_test_asw.h ===== */
#ifndef RELEASE_TEST_ASW_H
#define RELEASE_TEST_ASW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard headers */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32h7xx_hal.h"
#include "Release_test_asw_cfg.h"

/* Return codes for BSW API integration */
#define RELEASE_TEST_BSW_OK   (0U)
#define RELEASE_TEST_BSW_ERR  (1U)

/* Fault bitmask definitions */
#define RELEASE_TEST_FAULT_OVERVOLTAGE     (1u << 0)
#define RELEASE_TEST_FAULT_UNDERVOLTAGE    (1u << 1)
#define RELEASE_TEST_FAULT_OVERTEMP        (1u << 2)
#define RELEASE_TEST_FAULT_SENSOR_FAIL     (1u << 3)
#define RELEASE_TEST_FAULT_PRECHARGE_FAIL  (1u << 4)
#define RELEASE_TEST_FAULT_COMM_FAIL       (1u << 5)

/* ASW state enumeration */
typedef enum
{
    RELEASE_TEST_ASW_STATE_UNINITIALIZED = 0u,
    RELEASE_TEST_ASW_STATE_INIT,
    RELEASE_TEST_ASW_STATE_STANDBY,
    RELEASE_TEST_ASW_STATE_PRECHARGING,
    RELEASE_TEST_ASW_STATE_ACTIVE,
    RELEASE_TEST_ASW_STATE_FAULT
} Release_test_asw_State_t;

/* ASW inputs container */
typedef struct
{
    float packVoltage;                                          /* Pack voltage in V */
    float packCurrent;                                          /* Pack current in A (+ charge, - discharge) */
    float cellVoltages[RELEASE_TEST_ASW_NUM_CELLS];             /* Individual cell voltages in V */
    float cellTemperatures[RELEASE_TEST_ASW_NUM_CELLS];         /* Individual cell temperatures in degC */
    uint8_t cellCount;                                          /* Number of valid cells in arrays */
    uint32_t timestamp_ms;                                      /* HAL tick when sampled (ms) */
} Release_test_asw_Inputs_t;

/* ASW outputs container */
typedef struct
{
    bool contactorClosed;                                       /* True when main contactor commanded closed */
    bool prechargeActive;                                       /* True during precharge interval */
    bool balancerEnabled[RELEASE_TEST_ASW_NUM_CELLS];           /* Per-cell balancer enable */
    uint32_t faultMask;                                         /* Active fault bitmask */
} Release_test_asw_Outputs_t;

/* Public API of ASW */
void Release_test_asw_Init(void);
void Release_test_asw_DeInit(void);
void Release_test_asw_RunCycle(void);

/* Get latest outputs (copy) */
void Release_test_asw_GetOutputs(Release_test_asw_Outputs_t * const outputs);

/* Get internal state (for diagnostics) */
Release_test_asw_State_t Release_test_asw_GetState(void);

/* BSW API prototypes used by this ASW
   These are expected to be provided by BSW. Default weak implementations
   are provided in Release_test_asw.c and may be overridden by real BSW. */

/* Read pack voltage in Volts */
int32_t Bsw_GetPackVoltage(float * const voltage);

/* Read pack current in Amps */
int32_t Bsw_GetPackCurrent(float * const current);

/* Read individual cell voltages.
   cellVoltages: pointer to array sized at least numCells.
   numCells: number of cells to read (output may fill fewer).
   Returns RELEASE_TEST_BSW_OK on success. */
int32_t Bsw_GetCellVoltages(float * const cellVoltages, uint8_t * const numCells);

/* Read individual cell temperatures.
   cellTemps: pointer to array sized at least numCells.
   numCells: in: max slots available; out: number filled.
   Returns RELEASE_TEST_BSW_OK on success. */
int32_t Bsw_GetCellTemperatures(float * const cellTemps, uint8_t * const numCells);

/* Command contactor (true = close, false = open) */
int32_t Bsw_SetContactorCommand(bool close);

/* Enable/disable balancer for specific cell index (0..N-1) */
int32_t Bsw_EnableCellBalancer(uint8_t cellIndex, bool enable);

/* Report fault bitmask to higher-level diagnostics (BSW). */
int32_t Bsw_ReportFault(uint32_t faultMask);

#ifdef __cplusplus
}
#endif

#endif /* RELEASE_TEST_ASW_H */
