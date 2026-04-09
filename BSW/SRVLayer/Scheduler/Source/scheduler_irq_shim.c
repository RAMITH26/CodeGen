
#include "scheduler.h"
#include "stm32h7xx_hal.h"

/* TIM2 IRQHandler - calls HAL handler which will invoke callback to our shim */
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

/* HAL TIM period elapsed callback - called in ISR context */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if ((htim != (TIM_HandleTypeDef *)0) && (htim->Instance == TIM2))
    {
        /* Increment scheduler tick safely from ISR */
        Scheduler_TimerIRQHandler();
    }
}

