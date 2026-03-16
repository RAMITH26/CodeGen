
#ifndef BMS_CAN_CFG_H
#define BMS_CAN_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32h7xx_hal.h"

/* -------------------------
   Project / clock constants
   ------------------------- */
#define BMS_SYSTEM_CLOCK_FREQ_HZ              64000000UL  /* Required FDCAN kernel clock */

/* -------------------------
   Hardware mapping (package)
   ------------------------- */
/* FDCAN1 on STM32H743ZI UFBGA176:
   PA11 -> FDCAN1_RX (AF9)
   PA12 -> FDCAN1_TX (AF9)
*/
#define BMS_CAN_INSTANCE                      FDCAN1
#define BMS_CAN_RX_GPIO_PORT                  GPIOA
#define BMS_CAN_RX_GPIO_PIN                   GPIO_PIN_11
#define BMS_CAN_TX_GPIO_PORT                  GPIOA
#define BMS_CAN_TX_GPIO_PIN                   GPIO_PIN_12
#define BMS_CAN_GPIO_AF                       (9U)            /* AF9 for FDCAN1 on PA11/PA12 */

/* -------------------------
   Nominal bit-timing (64 MHz -> 500 kbps)
   Rationale: sample point = 87.5% (TS1=6, TS2=1), Prescaler = 16
   ------------------------- */
#define BMS_CAN_NOMINAL_PRESCALER             16U
#define BMS_CAN_NOMINAL_SJW                   1U
#define BMS_CAN_NOMINAL_TS1                   6U
#define BMS_CAN_NOMINAL_TS2                   1U

/* Data phase timing (not used for Classical CAN, kept equal to nominal if HAL requires) */
#define BMS_CAN_DATA_PRESCALER                BMS_CAN_NOMINAL_PRESCALER
#define BMS_CAN_DATA_SJW                      BMS_CAN_NOMINAL_SJW
#define BMS_CAN_DATA_TS1                      BMS_CAN_NOMINAL_TS1
#define BMS_CAN_DATA_TS2                      BMS_CAN_NOMINAL_TS2

/* -------------------------
   Message RAM / buffer sizing
   ------------------------- */
#define BMS_CAN_MESSAGE_RAM_OFFSET            0U  /* bytes; ensure alignment per RM */
#define BMS_CAN_STD_FILTERS_NBR               1U
#define BMS_CAN_EXT_FILTERS_NBR               1U

#define BMS_CAN_RXF0_ELEMENTS                 16U
#define BMS_CAN_RXF0_ELEMENT_SIZE_BYTES       16U

#define BMS_CAN_RXF1_ELEMENTS                 16U
#define BMS_CAN_RXF1_ELEMENT_SIZE_BYTES       16U

#define BMS_CAN_RX_BUFFERS_NBR                16U
#define BMS_CAN_RX_BUFFER_SIZE_BYTES          8U

#define BMS_CAN_TX_EVENTS_NBR                 8U
#define BMS_CAN_TX_BUFFERS_NBR                8U
#define BMS_CAN_TX_FIFOQ_ELEMENTS             8U
#define BMS_CAN_TX_ELEMENT_SIZE_BYTES         8U

/* -------------------------
   Filter configuration
   ------------------------- */
#define BMS_CAN_FILTER_BANK                   0U
/* Filter ID / mask macros: tool will populate these from project macros at generation time.
   Defaults provided here for build-time safety; generator must override with actual project macros.
*/
#ifndef BMS_CAN_FILTER_ID_HIGH
#define BMS_CAN_FILTER_ID_HIGH                0x0000U
#endif
#ifndef BMS_CAN_FILTER_ID_LOW
#define BMS_CAN_FILTER_ID_LOW                 0x0000U
#endif
#ifndef BMS_CAN_FILTER_MASK_HIGH
#define BMS_CAN_FILTER_MASK_HIGH              0x0000U
#endif
#ifndef BMS_CAN_FILTER_MASK_LOW
#define BMS_CAN_FILTER_MASK_LOW               0x0000U
#endif

/* Choose ID type: set to 1 for Extended IDs, 0 for Standard IDs.
   Current project design: Extended IDs -> 1
*/
#define BMS_CAN_FILTER_ID_TYPE_EXTENDED       1U

/* Route matched messages to RX FIFO1 */
#define BMS_CAN_FILTER_ROUTE_TO_RXFIFO1       1U

/* -------------------------
   NVIC / interrupts
   ------------------------- */
#define BMS_CAN_IRQ                           FDCAN1_IT0_IRQn
#define BMS_CAN_IRQ_PRIORITY                  6U  /* 0 = highest, larger = lower priority */

/* -------------------------
   Timeouts & retries
   ------------------------- */
#define BMS_RESPONSE_TIMEOUT_MS               200U
#define BMS_MESSAGE_VALIDITY_TIMEOUT_MS       1000U
#define BMS_COMMUNICATION_MAX_RETRIES         5U

/* -------------------------
   Public configuration object
   ------------------------- */
typedef struct
{
    uint32_t fcan_clock_hz;
    FDCAN_GlobalTypeDef *reserved; /* placeholder for future use; kept for alignment */
} BMS_CAN_ConfigConstType;

extern const BMS_CAN_ConfigConstType BMS_CAN_ConfigConst;

#ifdef __cplusplus
}
#endif

#endif /* BMS_CAN_CFG_H */

/* ===== End File: bms_can_cfg.h ===== */

