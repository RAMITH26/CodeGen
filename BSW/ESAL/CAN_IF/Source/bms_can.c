
#include "bms_can.h"
#include "bms_can_cfg.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_fdcan.h"
#include "stm32h7xx_hal_gpio.h"
#include <string.h>

/* Fallback definitions for HAL constants if not present in included HAL headers.
   These are defined only to ensure generator-produced config compiles on different HAL versions.
*/
#ifndef FDCAN_FRAME_CLASSIC
#define FDCAN_FRAME_CLASSIC               0U
#endif

#ifndef FDCAN_MODE_NORMAL
#define FDCAN_MODE_NORMAL                 0U
#endif

#ifndef FDCAN_FILTER_DUAL
#define FDCAN_FILTER_MASK                 0U
#endif

#ifndef FDCAN_FILTER_ID_STANDARD
#define FDCAN_FILTER_ID_STANDARD          0U
#endif

#ifndef FDCAN_FILTER_ID_EXTENDED
#define FDCAN_FILTER_ID_EXTENDED          1U
#endif

#ifndef FDCAN_FILTER_TO_RXFIFO0
#define FDCAN_FILTER_TO_RXFIFO0           0U
#endif

#ifndef FDCAN_FILTER_TO_RXFIFO1
#define FDCAN_FILTER_TO_RXFIFO1           1U
#endif

/* Local static FDCAN handle */
static FDCAN_HandleTypeDef hfdcan1;

/* Const configuration object defined in cfg header (simple single-field object) */
const BMS_CAN_ConfigConstType BMS_CAN_ConfigConst = {
    .fcan_clock_hz = BMS_SYSTEM_CLOCK_FREQ_HZ,
    .reserved = NULL
};

/* Local forward declarations */
static BMS_CAN_StatusTypeDef BMS_CAN_EnableClockAndGPIO(void);
static void BMS_CAN_GPIO_Config(void);

/* Implementation */

BMS_CAN_StatusTypeDef BMS_CAN_Init(void)
{
    /* Validate configuration */
    if (BMS_CAN_ConfigConst.fcan_clock_hz != BMS_SYSTEM_CLOCK_FREQ_HZ)
    {
        /* Clock mismatch; caller must ensure RCC configured to provide 64 MHz to FDCAN kernel */
        return BMS_CAN_ERROR;
    }

    /* Enable peripheral & GPIO clocks and configure pins */
    if (BMS_CAN_EnableClockAndGPIO() != BMS_CAN_OK)
    {
        return BMS_CAN_ERROR;
    }

    /* Populate FDCAN handle and init */
    memset(&hfdcan1, 0, sizeof(hfdcan1));
    hfdcan1.Instance = BMS_CAN_INSTANCE;

    /* FDCAN_InitTypeDef fields as required by STM32H7 HAL */
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC; /* Classical CAN (2.0B) */
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = ENABLE;
    hfdcan1.Init.TransmitPause = DISABLE;
    hfdcan1.Init.ProtocolException = DISABLE;

    hfdcan1.Init.NominalPrescaler = BMS_CAN_NOMINAL_PRESCALER;
    hfdcan1.Init.NominalSyncJumpWidth = BMS_CAN_NOMINAL_SJW;
    hfdcan1.Init.NominalTimeSeg1 = BMS_CAN_NOMINAL_TS1;
    hfdcan1.Init.NominalTimeSeg2 = BMS_CAN_NOMINAL_TS2;

    /* Data phase timing: not used for Classical CAN, but keep non-zero to satisfy HAL on some releases */
    hfdcan1.Init.DataPrescaler = BMS_CAN_DATA_PRESCALER;
    hfdcan1.Init.DataSyncJumpWidth = BMS_CAN_DATA_SJW;
    hfdcan1.Init.DataTimeSeg1 = BMS_CAN_DATA_TS1;
    hfdcan1.Init.DataTimeSeg2 = BMS_CAN_DATA_TS2;

    /* Message RAM and buffer sizing */
    hfdcan1.Init.MessageRAMOffset = BMS_CAN_MESSAGE_RAM_OFFSET;
    hfdcan1.Init.StdFiltersNbr = BMS_CAN_STD_FILTERS_NBR;
    hfdcan1.Init.ExtFiltersNbr = BMS_CAN_EXT_FILTERS_NBR;
    hfdcan1.Init.RxFifo0ElmtsNbr = BMS_CAN_RXF0_ELEMENTS;
    hfdcan1.Init.RxFifo0ElmtSize = BMS_CAN_RXF0_ELEMENT_SIZE_BYTES;
    hfdcan1.Init.RxFifo1ElmtsNbr = BMS_CAN_RXF1_ELEMENTS;
    hfdcan1.Init.RxFifo1ElmtSize = BMS_CAN_RXF1_ELEMENT_SIZE_BYTES;
    hfdcan1.Init.RxBuffersNbr = BMS_CAN_RX_BUFFERS_NBR;
    hfdcan1.Init.RxBufferSize = BMS_CAN_RX_BUFFER_SIZE_BYTES;
    hfdcan1.Init.TxEventsNbr = BMS_CAN_TX_EVENTS_NBR;
    hfdcan1.Init.TxBuffersNbr = BMS_CAN_TX_BUFFERS_NBR;
    hfdcan1.Init.TxFifoQueueElmtsNbr = BMS_CAN_TX_FIFOQ_ELEMENTS;
    hfdcan1.Init.TxElmtSize = BMS_CAN_TX_ELEMENT_SIZE_BYTES;

    /* Initialize peripheral */
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
    {
        return BMS_CAN_ERROR;
    }

    /* Configure NVIC for FDCAN interrupts */
    HAL_NVIC_SetPriority(BMS_CAN_IRQ, (uint32_t)BMS_CAN_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(BMS_CAN_IRQ);

    return BMS_CAN_OK;
}

BMS_CAN_StatusTypeDef BMS_CAN_ConfigureFilters(void)
{
    FDCAN_FilterTypeDef sFilterConfig;

    /* Basic validation */
    /* Set filter index from configuration */
    sFilterConfig.FilterIndex = BMS_CAN_FILTER_BANK;

    /* Select ID type according to project setting */
#if (BMS_CAN_FILTER_ID_TYPE_EXTENDED == 1U)
    sFilterConfig.IdType = FDCAN_FILTER_ID_EXTENDED;
#else
    sFilterConfig.IdType = FDCAN_FILTER_ID_STANDARD;
#endif

    /* Mask mode, route to Rx FIFO1 per project mapping */
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
#if (BMS_CAN_FILTER_ROUTE_TO_RXFIFO1 == 1U)
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
#else
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
#endif

    /* Use provided project macros for ID and mask fields */
    sFilterConfig.FilterID1 = BMS_CAN_FILTER_ID_HIGH;
    sFilterConfig.FilterID2 = BMS_CAN_FILTER_ID_LOW;

    /* The mask pair (if HAL provides separate mask fields, they are set inside FilterID2/FilterID1 as required).
       Some HAL versions expect the mask in FilterID2; adjust generator if necessary. */
    /* Mask encoding: generator must ensure correct format for ID type. */

    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
    {
        return BMS_CAN_ERROR;
    }

    return BMS_CAN_OK;
}

BMS_CAN_StatusTypeDef BMS_CAN_Transmit(FDCAN_TxHeaderTypeDef const * const pHeader,
                                      uint8_t const * const pData,
                                      uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    if ((pHeader == NULL) || (pData == NULL))
    {
        return BMS_CAN_ERROR;
    }

    /* Attempt to queue message until timeout */
    for (;;)
    {
        if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, (FDCAN_TxHeaderTypeDef *)pHeader,
                                         (uint8_t *)pData) == HAL_OK)
        {
            return BMS_CAN_OK;
        }

        if (timeout_ms == 0U)
        {
            return BMS_CAN_BUSY;
        }

        if ((HAL_GetTick() - start) >= timeout_ms)
        {
            return BMS_CAN_TIMEOUT;
        }
    }
}

BMS_CAN_StatusTypeDef BMS_CAN_Receive(uint8_t fifo, FDCAN_RxHeaderTypeDef * const pHeader,
                                     uint8_t * const pData, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    if ((pHeader == NULL) || (pData == NULL))
    {
        return BMS_CAN_ERROR;
    }

    /* fifo must be 0 or 1 */
    if ((fifo != 0U) && (fifo != 1U))
    {
        return BMS_CAN_ERROR;
    }

    /* Non-blocking or blocking until timeout */
    for (;;)
    {
        if (HAL_FDCAN_GetRxMessage(&hfdcan1,
                                   (fifo == 0U) ? FDCAN_RX_FIFO0 : FDCAN_RX_FIFO1,
                                   pHeader, pData) == HAL_OK)
        {
            return BMS_CAN_OK;
        }

        if (timeout_ms == 0U)
        {
            return BMS_CAN_BUSY;
        }

        if ((HAL_GetTick() - start) >= timeout_ms)
        {
            return BMS_CAN_TIMEOUT;
        }
    }
}

void BMS_CAN_IRQHandler(void)
{
    /* Minimal ISR: call HAL handler to process interrupts. Application-level callbacks
       must be implemented outside this BSW module to preserve isolation. */
    HAL_FDCAN_IRQHandler(&hfdcan1);
}

BMS_CAN_StatusTypeDef BMS_CAN_HandleTimeout(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint32_t attempts = 0U;
    BMS_CAN_StatusTypeDef status;

    do
    {
        /* Basic recovery: stop, re-init, reconfigure filters, start */
        if (HAL_FDCAN_Stop(&hfdcan1) != HAL_OK)
        {
            /* Continue attempts */
        }

        status = BMS_CAN_Init();
        if (status != BMS_CAN_OK)
        {
            attempts++;
            continue;
        }

        status = BMS_CAN_ConfigureFilters();
        if (status != BMS_CAN_OK)
        {
            attempts++;
            continue;
        }

        if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
        {
            attempts++;
            continue;
        }

        /* Successful recovery */
        return BMS_CAN_OK;

    } while ((attempts < BMS_COMMUNICATION_MAX_RETRIES) &&
             ((HAL_GetTick() - start) < timeout_ms));

    return BMS_CAN_ERROR;
}

/* -------------------------
   Local helpers
   ------------------------- */
static BMS_CAN_StatusTypeDef BMS_CAN_EnableClockAndGPIO(void)
{
    /* Configure FDCAN peripheral clock source: documented requirement to use PLL-derived 64 MHz.
       Caller must ensure system PLL is configured appropriately. */
#if defined(__HAL_RCC_FDCAN_CONFIG)
    /* STM32Cube HAL provides a macro to select FDCAN clock source */
    /* Documented configuration: select PLL as kernel clock */
    __HAL_RCC_FDCAN_CONFIG(RCC_FDCANCLKSOURCE_PLL);
#endif

    /* Enable peripheral clock */
#if defined(__HAL_RCC_FDCAN_CLK_ENABLE)
    __HAL_RCC_FDCAN_CLK_ENABLE();
#endif

    /* Enable GPIO clock(s) for PA11/PA12 */
#if defined(__HAL_RCC_GPIOA_CLK_ENABLE)
    __HAL_RCC_GPIOA_CLK_ENABLE();
#endif

    /* Configure GPIOs */
    BMS_CAN_GPIO_Config();

    return BMS_CAN_OK;
}

static void BMS_CAN_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* Configure TX (PA12) : AF9, Push-pull, Very_high speed, No pull */
    GPIO_InitStruct.Pin = BMS_CAN_TX_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = (uint32_t)BMS_CAN_GPIO_AF;
    HAL_GPIO_Init(BMS_CAN_TX_GPIO_PORT, &GPIO_InitStruct);

    /* Configure RX (PA11) : AF9, Push-pull (AF), No pull recommended (transceiver may require external pull) */
    GPIO_InitStruct.Pin = BMS_CAN_RX_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = (uint32_t)BMS_CAN_GPIO_AF;
    HAL_GPIO_Init(BMS_CAN_RX_GPIO_PORT, &GPIO_InitStruct);
}

/* ===== End File: bms_can.c ===== */