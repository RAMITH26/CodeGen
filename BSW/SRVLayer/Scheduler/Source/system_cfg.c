
#include "system_cfg.h"
#include "stm32h7xx_hal.h"
#include <stddef.h>

/* TIM handle for 1 ms tick generation using TIM2 */
TIM_HandleTypeDef htim2;

void System_Init(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_TIM2_Init();
    MX_NVIC_Config();
}

void SystemClock_Config(void)
{
    /* Minimal system clock configuration for 1 MHz system clock.
       This function should be adapted to platform-specific clock tree.
       For generated code we set up the CPU for a simple HSI usage path
       only if HAL clock configuration is performed elsewhere.
       Keep stub minimal to allow compilation. */
    /* Intentionally minimal to avoid overriding user configurations. */
    (void)SYSTEM_CLOCK_HZ;
}

void MX_TIM2_Init(void)
{
    /* Configure TIM2 for 1 kHz interrupts (1 ms tick) */
    /* Assuming SystemCoreClock is configured correctly by SystemClock_Config */
    uint32_t uwPrescalerValue;
    uint32_t tim_clk_hz;

    /* Use HAL function to get clock; if not available, assume SYSTEM_CLOCK_HZ */
#if defined(SYS_CLOCK_FREQ)
    tim_clk_hz = (uint32_t)SYS_CLOCK_FREQ;
#else
    tim_clk_hz = (uint32_t)SYSTEM_CLOCK_HZ;
#endif

    /* Compute prescaler to get 1 kHz counter frequency with 16-bit ARR */
    /* timer counter clock = tim_clk_hz / (Prescaler + 1) */
    /* we want timer tick = 1 kHz -> period_ms = 1 -> ARR = (timer counter clock / 1000) - 1 */
    if (tim_clk_hz == 0u)
    {
        tim_clk_hz = 1000000u;
    }

    /* For simplicity choose prescaler so that ARR fits in 32-bit */
    uwPrescalerValue = (tim_clk_hz / SCHEDULER_TICK_HZ) - 1u;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = uwPrescalerValue;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1u; /* Not used as we trigger via update interrupt every tick via prescaler division */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    /* Initialize the TIM base */
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        /* Initialization Error: report via diag manager or loop */
        while (1) { /* trap */ }
    }

    /* Start timer in interrupt mode */
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        while (1) { /* trap */ }
    }
}

void MX_NVIC_Config(void)
{
    /* Set TIM2 interrupt priority and enable it */
    HAL_NVIC_SetPriority(TIM2_IRQn, 5u, 0u);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

