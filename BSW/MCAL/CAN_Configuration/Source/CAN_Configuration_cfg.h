
#ifndef CAN_CONFIGURATION_CFG_H
#define CAN_CONFIGURATION_CFG_H

/* Standard headers */
#include <stdint.h>
#include <stdbool.h>

/* BSP / HAL selection header guard note:
   The implementation files include the STM32 HAL headers and adapt
   to available macros; adjust definitions here per board specifics. */

/* ---------------------------------------------------------------------------
   CAN hardware instance selection and basic timing parameters
   Adjust these macros to match your network bit-timing requirements.
   The values below are conservative defaults for many FDCAN setups and
   must be tuned for production (nominal/data prescalers & segments).
   --------------------------------------------------------------------------- */
#ifndef CAN_CFG_FDCAN_INSTANCE
#define CAN_CFG_FDCAN_INSTANCE    FDCAN1U /* Use FDCAN1U as token, implementation maps to actual instance */
#endif

/* Nominal bit timing (classical CAN bit-rate configuration, values are example) */
#ifndef CAN_CFG_NOMINAL_PRESCALER
#define CAN_CFG_NOMINAL_PRESCALER 1U
#endif
#ifndef CAN_CFG_NOMINAL_SJW
#define CAN_CFG_NOMINAL_SJW       1U
#endif
#ifndef CAN_CFG_NOMINAL_TS1
#define CAN_CFG_NOMINAL_TS1       63U
#endif
#ifndef CAN_CFG_NOMINAL_TS2
#define CAN_CFG_NOMINAL_TS2       16U
#endif

/* Data phase bit timing for CAN FD (if used) */
#ifndef CAN_CFG_DATA_PRESCALER
#define CAN_CFG_DATA_PRESCALER    1U
#endif
#ifndef CAN_CFG_DATA_SJW
#define CAN_CFG_DATA_SJW          1U
#endif
#ifndef CAN_CFG_DATA_TS1
#define CAN_CFG_DATA_TS1          63U
#endif
#ifndef CAN_CFG_DATA_TS2
#define CAN_CFG_DATA_TS2          16U
#endif

/* Filter and FIFO configuration */
#ifndef CAN_CFG_STD_FILTER_COUNT
#define CAN_CFG_STD_FILTER_COUNT  8U
#endif
#ifndef CAN_CFG_EXT_FILTER_COUNT
#define CAN_CFG_EXT_FILTER_COUNT  8U
#endif

#ifndef CAN_CFG_RX_FIFO0_SIZE
#define CAN_CFG_RX_FIFO0_SIZE     64U
#endif
#ifndef CAN_CFG_TX_FIFO_QUEUE_SIZE
#define CAN_CFG_TX_FIFO_QUEUE_SIZE 32U
#endif

/* NVIC priorities and timeouts */
#ifndef CAN_CFG_IRQ_PRIORITY
#define CAN_CFG_IRQ_PRIORITY      6U
#endif
#ifndef CAN_CFG_IRQ_SUBPRIORITY
#define CAN_CFG_IRQ_SUBPRIORITY   0U
#endif

#ifndef CAN_CFG_TX_TIMEOUT_MS
#define CAN_CFG_TX_TIMEOUT_MS     100U
#endif
#ifndef CAN_CFG_INIT_TIMEOUT_MS
#define CAN_CFG_INIT_TIMEOUT_MS   200U
#endif

/* Return/Status types mapping (keeps code portable) */
#include "stm32h7xx_hal.h"

/* Filter descriptor used by BSW filter setup APIs */
typedef struct
{
    uint32_t id;        /* Filter id (standard 11-bit or extended 29-bit) */
    uint32_t mask;      /* Filter mask */
    bool     extended;  /* true -> extended ID, false -> standard ID */
    uint32_t fifo;      /* 0 for Rx FIFO 0, 1 for Rx FIFO 1 (if supported) */
    bool     enabled;   /* true to enable this filter */
} CAN_FilterConfig_t;

#endif /* CAN_CONFIGURATION_CFG_H */

