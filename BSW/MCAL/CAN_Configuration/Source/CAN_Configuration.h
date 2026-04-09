
#ifndef CAN_CONFIGURATION_H
#define CAN_CONFIGURATION_H

/* Standard headers */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* HAL headers required for FDCAN on STM32H7 series */
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_fdcan.h"

/* Module configuration */
#include "CAN_Configuration_cfg.h"

/* Public API - initialization and control */
HAL_StatusTypeDef CAN_Config_Init(void);
HAL_StatusTypeDef CAN_Config_Start(void);
HAL_StatusTypeDef CAN_Config_Stop(void);

/* Transmit a single CAN/FDCAN frame (up to 8 bytes for classic CAN or up to 64 for CAN FD).
   'length' must be <= 64. Returns HAL_OK on success, HAL_ERROR or HAL_TIMEOUT on failure. */
HAL_StatusTypeDef CAN_Config_Transmit(uint32_t id, const uint8_t *pdata, uint8_t length, bool extended);

/* Add a filter; returns HAL_OK on success */
HAL_StatusTypeDef CAN_Config_AddFilter(const CAN_FilterConfig_t *filter);

/* Retrieve internal FDCAN handle for advanced operations if required */
FDCAN_HandleTypeDef *CAN_Config_GetHandle(void);

/* RX indication callback invoked when an Rx frame is received.
   Provide application implementation if required; weak default provided in .c */
void CAN_Config_RxIndication(uint32_t id, const uint8_t *pdata, uint8_t length, bool extended);

/* Error indication callback for reporting FDCAN errors; weak default provided in .c */
void CAN_Config_ErrorNotification(uint32_t error_code);

#endif /* CAN_CONFIGURATION_H */

