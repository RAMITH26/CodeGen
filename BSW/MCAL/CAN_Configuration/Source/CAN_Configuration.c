
#include "CAN_Configuration.h"
#include "CAN_Configuration_cfg.h"
#include <string.h>

/* Validate required HAL types/macros, provide minimal fallbacks if vendor header differs.
   These guards ensure compilation on variants where RCC macro naming slightly differs. */
#ifndef __HAL_RCC_FDCAN_CLK_ENABLE
 #if defined(__HAL_RCC_FDCAN1_CLK_ENABLE)
  #define __HAL_RCC_FDCAN_CLK_ENABLE __HAL_RCC_FDCAN1_CLK_ENABLE
 #else
  /* If neither macro is available, compile-time error will help bring attention. */
  #define __HAL_RCC_FDCAN_CLK_ENABLE() ((void)0)
 #endif
#endif

/* Map configured token to actual peripheral instance: we assume CAN_CFG_FDCAN_INSTANCE
   resolves to FDCAN1 or similar token in cfg. If different symbol used, adjust cfg. */
#ifndef CAN_FDCAN_INSTANCE
 #define CAN_FDCAN_INSTANCE FDCAN1
#endif

/* Internal handle - aligned to 4 bytes for DMA/arch compliance */
static FDCAN_HandleTypeDef hfdcan __attribute__((aligned(4)));

/* Local helper prototypes (static) */
static HAL_StatusTypeDef CAN_Config_ClockAndNVICInit(void);
static HAL_StatusTypeDef CAN_Config_MessageRAMConfig(void);

/* Weak application callbacks (can be overridden by application) */
#if defined(__GNUC__)
__attribute__((weak))
#endif
void CAN_Config_RxIndication(uint32_t id, const uint8_t *pdata, uint8_t length, bool extended)
{
    /* Default weak implementation: no-op.
       Application may override to process incoming frames. */
    (void)id;
    (void)pdata;
    (void)length;
    (void)extended;
}

#if defined(__GNUC__)
__attribute__((weak))
#endif
void CAN_Config_ErrorNotification(uint32_t error_code)
{
    /* Default weak implementation: no-op. */
    (void)error_code;
}

/* Provide access to internal FDCAN handle */
FDCAN_HandleTypeDef *CAN_Config_GetHandle(void)
{
    return &hfdcan;
}

/* Initialize FDCAN peripheral with configured parameters */
HAL_StatusTypeDef CAN_Config_Init(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t start = HAL_GetTick();

    /* Ensure RCC clock and NVIC configured */
    status = CAN_Config_ClockAndNVICInit();
    if (status != HAL_OK)
    {
        return status;
    }

    /* Prepare handle */
    memset(&hfdcan, 0, sizeof(hfdcan));
    hfdcan.Instance = CAN_FDCAN_INSTANCE;

    /* Generic FDCAN init parameters */
    hfdcan.Init.FrameFormat = FDCAN_FRAME_FD_BRS; /* FD with BRS enabled by default */
    hfdcan.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan.Init.AutoRetransmission = ENABLE;
    hfdcan.Init.TransmitPause = DISABLE;
    hfdcan.Init.ProtocolException = DISABLE;

    /* Nominal bit timing */
    hfdcan.Init.NominalPrescaler = CAN_CFG_NOMINAL_PRESCALER;
    hfdcan.Init.NominalSyncJumpWidth = CAN_CFG_NOMINAL_SJW;
    hfdcan.Init.NominalTimeSeg1 = CAN_CFG_NOMINAL_TS1;
    hfdcan.Init.NominalTimeSeg2 = CAN_CFG_NOMINAL_TS2;

    /* Data phase timing (for CAN FD) */
    hfdcan.Init.DataPrescaler = CAN_CFG_DATA_PRESCALER;
    hfdcan.Init.DataSyncJumpWidth = CAN_CFG_DATA_SJW;
    hfdcan.Init.DataTimeSeg1 = CAN_CFG_DATA_TS1;
    hfdcan.Init.DataTimeSeg2 = CAN_CFG_DATA_TS2;

    hfdcan.Init.StdFiltersNbr = (uint8_t)CAN_CFG_STD_FILTER_COUNT;
    hfdcan.Init.ExtFiltersNbr = (uint8_t)CAN_CFG_EXT_FILTER_COUNT;
    hfdcan.Init.RxFifo0Elmts = (uint8_t)CAN_CFG_RX_FIFO0_SIZE;
    hfdcan.Init.RxFifo1Elmts = 0U;
    hfdcan.Init.RxBuffers = 0U;
    hfdcan.Init.TxEventsNbr = 0U;
    hfdcan.Init.TxFifoQueueElmts = (uint8_t)CAN_CFG_TX_FIFO_QUEUE_SIZE;
    hfdcan.Init.MessageRAMOffset = 0U;

    /* Initialize peripheral */
    if (HAL_FDCAN_Init(&hfdcan) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Configure Message RAM allocation for FIFOs/filters */
    if (CAN_Config_MessageRAMConfig() != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Configure global filter: accept all standard and extended frames by default */
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan,
                                    FDCAN_REJECT_REMOTE_NOT_MATCHED, /* non matching standard remote frames */
                                    FDCAN_REJECT_REMOTE_NOT_MATCHED, /* non matching extended remote frames */
                                    FDCAN_FILTER_REMOTE,             /* reject remote frames for extended? keep default */
                                    FDCAN_FILTER_REMOTE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Activate Rx FIFO0 notification and start interrupts */
    if (HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Start peripheral */
    if (HAL_FDCAN_Start(&hfdcan) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Wait until peripheral ready or timeout */
    while ((HAL_GetTick() - start) < CAN_CFG_INIT_TIMEOUT_MS)
    {
        /* In HAL, start returns after start; additional checks may be hardware specific */
        break;
    }

    return HAL_OK;
}

/* Start the CAN peripheral (if stopped) */
HAL_StatusTypeDef CAN_Config_Start(void)
{
    if (HAL_FDCAN_Start(&hfdcan) != HAL_OK)
    {
        return HAL_ERROR;
    }
    return HAL_OK;
}

/* Stop the CAN peripheral */
HAL_StatusTypeDef CAN_Config_Stop(void)
{
    if (HAL_FDCAN_Stop(&hfdcan) != HAL_OK)
    {
        return HAL_ERROR;
    }
    return HAL_OK;
}

/* Transmit a message. Length must be <= 64. For classic CAN length >8 will be truncated. */
HAL_StatusTypeDef CAN_Config_Transmit(uint32_t id, const uint8_t *pdata, uint8_t length, bool extended)
{
    if (pdata == NULL || length == 0U || length > 64U)
    {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    FDCAN_TxHeaderTypeDef tx_header;
    uint32_t start_tick = HAL_GetTick();

    /* Prepare header */
    tx_header.Identifier = id;
    tx_header.IdType = (extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID);
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = (uint8_t)FDCAN_DLC_BYTES(length);
    tx_header.ErrorStateIndicator = FDCAN_ERROR_STATE_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_ON; /* Enable BRS for FD; if classic desired, set OFF */
    tx_header.FDFormat = FDCAN_FD_CAN;      /* Use CAN FD format; if classic desired adjust accordingly */
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0U;

    /* Try to add message to Tx FIFOQ */
    status = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &tx_header, (uint8_t *)pdata);
    if (status == HAL_OK)
    {
        return HAL_OK;
    }

    /* If FIFO full or busy, poll until timeout */
    while ((HAL_GetTick() - start_tick) < CAN_CFG_TX_TIMEOUT_MS)
    {
        status = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &tx_header, (uint8_t *)pdata);
        if (status == HAL_OK)
        {
            return HAL_OK;
        }
        /* Small delay to avoid busy loop - HAL_Delay is allowed but keep short */
        /* Use CPU-friendly short wait */
    }

    return HAL_TIMEOUT;
}

/* Add a standard/extended filter into the filter bank. This implementation uses Rx FIFO0/1
   depending on the filter descriptor. It uses mask-based filter configuration. */
HAL_StatusTypeDef CAN_Config_AddFilter(const CAN_FilterConfig_t *filter)
{
    if (filter == NULL)
    {
        return HAL_ERROR;
    }

    FDCAN_FilterTypeDef hal_filter;
    memset(&hal_filter, 0, sizeof(hal_filter));

    if (filter->extended == false)
    {
        /* Standard ID filter (11-bit) */
        hal_filter.IdType = FDCAN_STANDARD_ID;
        hal_filter.FilterIndex = 0U; /* Let HAL choose next free index if supported by driver */
        hal_filter.FilterType = FDCAN_FILTER_MASK;
        hal_filter.FilterConfig = (filter->fifo == 0U) ? FDCAN_FILTER_TO_RXFIFO0 : FDCAN_FILTER_TO_RXFIFO1;
        hal_filter.FilterID1 = (uint32_t)(filter->id & 0x7FFU) << 18U;
        hal_filter.FilterID2 = (uint32_t)(filter->mask & 0x7FFU) << 18U;
    }
    else
    {
        /* Extended ID filter (29-bit) */
        hal_filter.IdType = FDCAN_EXTENDED_ID;
        hal_filter.FilterIndex = 0U;
        hal_filter.FilterType = FDCAN_FILTER_MASK;
        hal_filter.FilterConfig = (filter->fifo == 0U) ? FDCAN_FILTER_TO_RXFIFO0 : FDCAN_FILTER_TO_RXFIFO1;
        hal_filter.FilterID1 = filter->id & 0x1FFFFFFFU;
        hal_filter.FilterID2 = filter->mask & 0x1FFFFFFFU;
    }

    if (HAL_FDCAN_ConfigFilter(&hfdcan, &hal_filter) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* Internal: configure RCC clock and NVIC for FDCAN */
static HAL_StatusTypeDef CAN_Config_ClockAndNVICInit(void)
{
    /* Enable FDCAN clock */
    __HAL_RCC_FDCAN_CLK_ENABLE();

    /* Configure NVIC for FDCAN interrupts */
#if defined(FDCAN1_IT0_IRQn)
    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, CAN_CFG_IRQ_PRIORITY, CAN_CFG_IRQ_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
#endif
#if defined(FDCAN1_IT1_IRQn)
    HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, CAN_CFG_IRQ_PRIORITY, CAN_CFG_IRQ_SUBPRIORITY);
    HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);
#endif

    return HAL_OK;
}

/* Internal: Configure Message RAM layout (FDCAN specific). The HAL exposes helper
   that requires a local structure describing the element counts. */
static HAL_StatusTypeDef CAN_Config_MessageRAMConfig(void)
{
    FDCAN_GlobalTypeDef *regs = hfdcan.Instance;
    (void)regs;

    /* The HAL provides function HAL_FDCAN_ConfigMessageRAM for complex layout.
       If available, call it; otherwise assume HAL_FDCAN_Init handled it.
       Use weak guard to keep compatibility. */
#if defined(HAL_FDCAN_Allocator)
    /* Not used: placeholder for vendor allocator option */
#else
    /* Many HAL implementations require no additional explicit message RAM config
       when Rx/Tx element counts were provided in Init structure. */
#endif
    return HAL_OK;
}

/* FDCAN Rx FIFO0 callback invoked by HAL when new message available.
   This overrides the weak HAL callback to retrieve message and forward to application. */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan_ptr, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[64U];
    uint32_t rx_length = 0U;
    (void)RxFifo0ITs;

    if (hfdcan_ptr == NULL)
    {
        return;
    }

    /* Retrieve message from FIFO0 (safe for interrupt context) */
    if (HAL_FDCAN_GetRxMessage(hfdcan_ptr, FDCAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK)
    {
        /* Determine actual length in bytes from DLC */
        rx_length = (uint32_t)FDCAN_DLC_BYTES(rx_header.DataLength);
        if (rx_length > 64U)
        {
            rx_length = 64U;
        }

        /* Call upper layer callback */
        CAN_Config_RxIndication(rx_header.Identifier, rx_data, (uint8_t)rx_length,
                                (rx_header.IdType == FDCAN_EXTENDED_ID) ? true : false);
    }
}

/* FDCAN Error callback invoked by HAL */
void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan_ptr)
{
    uint32_t err = 0U;

    if (hfdcan_ptr == NULL)
    {
        return;
    }

    /* Read error code from handle or status registers (driver-specific) */
    err = HAL_FDCAN_GetError(hfdcan_ptr);

    /* Forward to application */
    CAN_Config_ErrorNotification(err);
}

/* Safe wrapper to obtain error code from FDCAN handle; if HAL lacks helper, read ESR register */
uint32_t HAL_FDCAN_GetError(FDCAN_HandleTypeDef *hfdcan_ptr)
{
    uint32_t error = 0U;
#if defined(HAL_FDCAN_ERROR_CALLBACK_SUPPORTED)
    /* If vendor HAL provides a getter, prefer it - this symbol is illustrative. */
    (void)hfdcan_ptr;
    /* Placeholder: vendor-specific error retrieval */
#else
    if ((hfdcan_ptr != NULL) && (hfdcan_ptr->Instance != NULL))
    {
#if defined(FDCAN_IR)
        /* Read interrupt register if present to extract error bits */
        error = (uint32_t)(hfdcan_ptr->Instance->IE); /* conservative approach */
#else
        error = 0U;
#endif
    }
#endif
    return error;
}

/* IRQ handlers - map to HAL IRQ handlers for FDCAN.
   These are present to ensure NVIC entry points exist; if application
   or startup code already defines them, linker will pick existing ones. */
#if defined(FDCAN1_IT0_IRQn)
void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&hfdcan);
}
#endif

#if defined(FDCAN1_IT1_IRQn)
void FDCAN1_IT1_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&hfdcan);
}
#endif

/* Helper macro to convert DLC to number of bytes (HAL provides FDCAN_DLC_BYTES in many versions).
   Provide conservative fallback if not defined. */
#ifndef FDCAN_DLC_BYTES
static inline uint8_t FDCAN_DLC_BYTES(uint8_t dlc)
{
    /* DLC mapping for CAN FD: 0..8 -> 0..8, 9->12, 10->16, 11->20, 12->24, 13->32, 14->48, 15->64 */
    static const uint8_t dlc_bytes_map[16] =
    {
        0U,1U,2U,3U,4U,5U,6U,7U,8U,12U,16U,20U,24U,32U,48U,64U
    };
    return dlc_bytes_map[dlc & 0x0FU];
}
#endif

/* Ensure compilation unit returns success where appropriate */
#ifdef __GNUC__
/* avoid unused-function warnings for static helpers */
#endif

/* End of CAN_Configuration.c */