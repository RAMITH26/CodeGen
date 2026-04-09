
#ifndef RELEASE_TEST_ASW_CFG_H
#define RELEASE_TEST_ASW_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration macros for Release_test_asw module
   Adjust these macros to match the battery pack and system design. */

/* Hardware configuration */
#define RELEASE_TEST_ASW_NUM_CELLS            (16U)    /* Number of cells in series supported (max) */

/* Cell safety thresholds (volts, degrees Celsius) */
#define RELEASE_TEST_CELL_OVERVOLTAGE_VOLT    (4.200f) /* Per-cell overvoltage threshold */
#define RELEASE_TEST_CELL_UNDERVOLTAGE_VOLT   (2.800f) /* Per-cell undervoltage threshold */
#define RELEASE_TEST_CELL_OVERTEMP_C          (65.0f)  /* Per-cell overtemperature threshold (degC) */

/* Precharge / contactor */
#define RELEASE_TEST_PRECHARGE_ENABLE_VOLT    (3.0f * 1.0f)   /* Minimum per-series-cell average to allow precharge (unused if pack voltage used) */
#define RELEASE_TEST_PRECHARGE_TIME_MS        (5000UL)        /* Precharge duration required (ms) */
#define RELEASE_TEST_PRECHARGE_MAX_CURRENT_A  (50.0f)         /* Max allowed current during precharge (A) */

/* Current limits (A) */
#define RELEASE_TEST_MAX_CHARGE_CURRENT_A     (200.0f)  /* Absolute maximum allowed charging current */
#define RELEASE_TEST_MAX_DISCHARGE_CURRENT_A  (-400.0f) /* Absolute maximum allowed discharge current (negative) */

/* Cell balancing criteria */
#define RELEASE_TEST_BALANCE_DELTA_VOLT       (0.020f)  /* Minimum delta between highest and lowest cell to consider balancing (V) */
#define RELEASE_TEST_BALANCE_ENABLE_VOLT      (3.900f)  /* Balancing enabled only when cell voltage reaches this or above (V) */
#define RELEASE_TEST_BALANCE_ENABLE_DIFF_VOLT (0.005f)  /* Per-cell threshold above average to enable balancing (V) */

/* Timeouts and retries */
#define RELEASE_TEST_SENSOR_READ_TIMEOUT_MS   (100U)    /* Max age of sensor data tolerated by ASW (ms) */

/* Compile-time sanity checks */
#if (RELEASE_TEST_ASW_NUM_CELLS == 0U)
#error "RELEASE_TEST_ASW_NUM_CELLS must be >= 1"
#endif

#ifdef __cplusplus
}
#endif

#endif /* RELEASE_TEST_ASW_CFG_H */