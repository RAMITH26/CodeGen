
#ifndef BALANCING_ASW_CFG_H
#define BALANCING_ASW_CFG_H

/* Configuration macros for balancing ASW */

/* Number of cells in the pack */
#ifndef NUM_CELLS
#define NUM_CELLS (32u)
#endif

/* Selection margin in millivolts */
#ifndef SEL_MARGIN_MV
#define SEL_MARGIN_MV (15u)
#endif

/* Balancing thresholds in millivolts */
#ifndef BAL_STOP_DIFF_MV
#define BAL_STOP_DIFF_MV (10u)
#endif

#ifndef BAL_START_DIFF_MV
#define BAL_START_DIFF_MV (30u)
#endif

/* BMS charging state value - value indicating charging state in external BMS_State variable */
#ifndef BMS_STATE_CHARGING
#define BMS_STATE_CHARGING (3u)
#endif

/* Compile-time checks */
#if (NUM_CELLS == 0u)
#error "NUM_CELLS must be greater than zero"
#endif

#endif /* BALANCING_ASW_CFG_H */