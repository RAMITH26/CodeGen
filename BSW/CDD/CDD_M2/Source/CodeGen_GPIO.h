/**
 * @file CodeGen_GPIO.h
 * @brief GPIO configuration for CAN communication in Battery Management System (BSW)
 *
 * This header exposes a stable, backward-compatible API for GPIO configuration
 * used by the CAN peripheral in the Battery Management System. Implementations
 * shall follow STM32H7 HAL conventions. Functions are reentrant unless
 * documented otherwise. Callers are responsible for ensuring correct
 * initialization ordering (peripheral clocks enabled, CAN disabled when
 * deinitializing, etc.).
 */

#ifndef CODEGEN_GPIO_H
#define CODEGEN_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CodeGen_BMS_config.h"
#include "stm32h7xx_hal.h"

/**
 * @brief Initialize GPIO pins used by the CAN peripheral.
 *
 * This configures GPIO ports, pins, mode (AF), speed, pull-up/pull-down and
 * alternate function mapping required for CAN TX/RX. The function preserves
 * backward-compatible signature and behavior. The caller must ensure that the
 * CAN peripheral clock is enabled prior to calling if required by the
 * platform initialization sequence.
 *
 * @retval HAL_OK Configuration successful.
 * @retval HAL_ERROR Configuration failed.
 * @retval HAL_BUSY  Peripheral busy.
 * @retval HAL_TIMEOUT Timeout occurred.
 */
HAL_StatusTypeDef BMS_CAN_GPIO_Init(void);

/**
 * @brief Deinitialize CAN GPIO pins.
 *
 * Restores GPIO state to a safe default where possible. Caller must ensure the
 * CAN peripheral is disabled and no interrupts are pending for the related
 * EXTI lines before invoking this function.
 *
 * @retval HAL_OK Deinitialization successful.
 * @retval HAL_ERROR Deinitialization failed.
 * @retval HAL_BUSY  Peripheral busy.
 * @retval HAL_TIMEOUT Timeout occurred.
 */
HAL_StatusTypeDef BMS_CAN_GPIO_DeInit(void);

/**
 * @brief Configure CAN-related GPIO interrupts and NVIC priorities.
 *
 * Sets up EXTI/IRQ mapping, enables or disables IRQs and configures priority
 * levels consistent with system requirements. This function is intended to be
 * called from a single-threaded initialization context. It returns an error if
 * interrupt configuration cannot be reliably applied.
 *
 * @retval HAL_OK Interrupts configured successfully.
 * @retval HAL_ERROR Configuration failed.
 * @retval HAL_BUSY  Peripheral busy.
 * @retval HAL_TIMEOUT Timeout occurred.
 */
HAL_StatusTypeDef BMS_CAN_GPIO_InterruptConfig(void);

#ifdef __cplusplus
}
#endif

#endif /* CODEGEN_GPIO_H */
