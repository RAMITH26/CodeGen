
#ifndef BMS_SOC_MANAGER_CFG_H
#define BMS_SOC_MANAGER_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"

/* Configuration macros */
#define BMS_SOC_MAX_CELLS               (32U)
#define BMS_SOC_OCV_TABLE_SIZE          (16U)
#define BMS_SOC_PUBLISH_PERIOD_MS       (100U)    /* Publish every 100 ms */
#define BMS_SOC_EXPECTED_SAMPLE_PERIOD_MS (100U)   /* Expected AFE sample period (ms) */
#define BMS_CELL_VOLTAGE_MIN_V          (2.0F)
#define BMS_CELL_VOLTAGE_MAX_V          (4.25F)
#define BMS_OCV_CURRENT_THRESHOLD_A     (1.0F)    /* |I| < 1.0 A for OCV condition */
#define BMS_OCV_REST_DURATION_S         (1800U)   /* 30 minutes */
#define BMS_OCV_STDDEV_IMBALANCE_PCT    (5.0F)    /* 5% stddev threshold */
#define BMS_CURRENT_SENSOR_DELTA_THRESHOLD_A (50.0F) /* heuristic delta for stuck/noisy sensor */

/* Expose constants for C-file */
extern const float BMS_SOC_Manager_CfgDefault_ocv_voltages[BMS_SOC_OCV_TABLE_SIZE];
extern const float BMS_SOC_Manager_CfgDefault_ocv_soc[BMS_SOC_OCV_TABLE_SIZE];

/* Default configuration instance */
typedef struct BMS_SOC_Manager_CfgStaticTag
{
    float pack_capacity_Ah;
    float ocv_table_voltages_V[BMS_SOC_OCV_TABLE_SIZE];
    float ocv_table_soc_pct[BMS_SOC_OCV_TABLE_SIZE];
    uint32_t ocv_table_size;
    void (*publish_cb)(const void *); /* weak placeholder, overridden by runtime config */
} BMS_SOC_Manager_CfgStaticType;

/* Default configuration object used when init cfg is NULL.
   The publish_cb must be set by the integrator after linking, if NULL the module will operate
   but will not publish diagnostics. */
extern const BMS_SOC_Manager_CfgType BMS_SOC_Manager_CfgDefault;

/* Default tables (generic Li-ion curve approximation) */
const float BMS_SOC_Manager_CfgDefault_ocv_voltages[BMS_SOC_OCV_TABLE_SIZE] =
{
    2.800F, 3.000F, 3.200F, 3.400F, 3.500F, 3.600F, 3.650F, 3.700F,
    3.750F, 3.800F, 3.850F, 3.900F, 3.950F, 4.000F, 4.100F, 4.200F
};

const float BMS_SOC_Manager_CfgDefault_ocv_soc[BMS_SOC_OCV_TABLE_SIZE] =
{
    0.0F, 5.0F, 10.0F, 20.0F, 30.0F, 40.0F, 50.0F, 60.0F,
    70.0F, 80.0F, 85.0F, 90.0F, 95.0F, 97.0F, 99.0F, 100.0F
};

/* Default config instance */
const BMS_SOC_Manager_CfgType BMS_SOC_Manager_CfgDefault =
{
    .pack_capacity_Ah = 100.0F,
    .ocv_table_voltages_V =
    {
        2.800F, 3.000F, 3.200F, 3.400F, 3.500F, 3.600F, 3.650F, 3.700F,
        3.750F, 3.800F, 3.850F, 3.900F, 3.950F, 4.000F, 4.100F, 4.200F
    },
    .ocv_table_soc_pct =
    {
        0.0F, 5.0F, 10.0F, 20.0F, 30.0F, 40.0F, 50.0F, 60.0F,
        70.0F, 80.0F, 85.0F, 90.0F, 95.0F, 97.0F, 99.0F, 100.0F
    },
    .ocv_table_size = BMS_SOC_OCV_TABLE_SIZE,
    .publish_cb = (BMS_SOC_PublishCb_t)NULL
};

#ifdef __cplusplus
}
#endif

#endif /* BMS_SOC_MANAGER_CFG_H */