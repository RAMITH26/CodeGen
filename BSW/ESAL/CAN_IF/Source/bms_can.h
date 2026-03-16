
#ifndef BMS_CAN_H
#define BMS_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "bms_can_cfg.h"

/* Return codes local to BMS CAN wrapper */
typedef enum
{
    BMS_CAN_OK = 0,
    BMS_CAN_ERROR = 1,
    BMS_CAN_TIMEOUT = 2,
    BMS_CAN_BUSY = 3
} BMS_CAN_StatusTypeDef;

/* Public API prototypes (BSW/HAL boundary) */
/* Initialize FDCAN peripheral according to configuration (GPIO, clocks, message RAM, filters are configured later) */
BMS_CAN_StatusTypeDef BMS_CAN_Init(void);

/* Configure CAN filters (mask mode) from configuration macros */
BMS_CAN_StatusTypeDef BMS_CAN_ConfigureFilters(void);

/* Queue a message for transmission. Header and pData are HAL FDCAN types.
   Timeout in ms used only for software timeout before returning BMS_CAN_TIMEOUT.
*/
BMS_CAN_StatusTypeDef BMS_CAN_Transmit(FDCAN_TxHeaderTypeDef const * const pHeader,
                                      uint8_t const * const pData,
                                      uint32_t timeout_ms);

/* Receive a message from specified FIFO (0 or 1). pHeader and pData must point to valid storage.
   timeout_ms: 0 means non-blocking, otherwise block until timeout (uses HAL_GetTick()).
*/
BMS_CAN_StatusTypeDef BMS_CAN_Receive(uint8_t fifo, FDCAN_RxHeaderTypeDef * const pHeader,
                                     uint8_t * const pData, uint32_t timeout_ms);

/* ISR entry - to be called from the vector for the FDCAN instance used.
   This wrapper calls HAL_FDCAN_IRQHandler() and keeps driver isolation.
*/
void BMS_CAN_IRQHandler(void);

/* Handle persistent error recovery (bus-off handling, re-init attempts).
   Returns BMS_CAN_OK on successful recovery, error otherwise.
*/
BMS_CAN_StatusTypeDef BMS_CAN_HandleTimeout(uint32_t timeout_ms);

#endif /* BMS_CAN_H */

/* ===== End File: bms_can.h ===== */

