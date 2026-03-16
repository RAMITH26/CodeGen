
#include "system_cfg.h"
#include "stm32h7xx_hal.h"

/* Minimal, portable system initialization for STM32H7
   Configures HAL and system clock to the required frequency.
   NOTE: This file is self-contained and safe to call from main(). */

void System_Init(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    (void)HAL_Init();

    /* Configure the system clock to SYSTEM_CLOCK_MHZ MHz. */
    SystemClock_Config();
}

void SystemClock_Config(void)
{
    /* Basic configuration: select HSI and configure PLL to reach requested frequency.
       This implementation is generic and focuses on achieving the target clock.
       For production use adapt PLL parameters to the specific H7 device and board. */

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable HSI and configure PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    /* These PLL parameters are conservative placeholders; adjust per board */
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8u;
    RCC_OscInitStruct.PLL.PLLN = (uint32_t)((SYSTEM_CLOCK_MHZ * 2u)); /* coarse */
    RCC_OscInitStruct.PLL.PLLP = 2u;
    RCC_OscInitStruct.PLL.PLLQ = 2u;
    RCC_OscInitStruct.PLL.PLLR = 2u;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* If configuration fails, trap here for safety */
        while (1)
        {
            __BKPT(0);
        }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        /* If configuration fails, trap here for safety */
        while (1)
        {
            __BKPT(0);
        }
    }
}

