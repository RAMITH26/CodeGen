
/* Lightweight ISR binding helpers.
   Each ASW that requires an ISR must implement the <Module>_ISR symbol.
   Here we declare weak IRQ handlers that call module-specific ISR functions.
   The actual vector table should reference these CMSIS-style names. */

#include "stm32h7xx_hal.h"

/* External ISR entrypoints expected from integration layer/ASW */
extern void Dynamic_address_assignment_ISR(void);
extern void Soc_estimation_ISR(void);
extern void Passive_Cell_balancing_ISR(void);

/* Example timer IRQ handler forwarding to integration ISR wrapper.
   Real vector table bindings should call these handlers. */

void TIMx_IRQHandler(void)
{
    /* Clear interrupt flag via HAL */
    HAL_TIM_IRQHandler(NULL);

    /* Forward to module ISR(s) if they are relevant to this timer */
    Dynamic_address_assignment_ISR();
    Passive_Cell_balancing_ISR();
}

/* Weak default implementations of module ISRs in case ASW doesn't provide them */
__attribute__((weak)) void Dynamic_address_assignment_ISR(void)
{
    /* Default empty ISR: modules can override */
    __DSB();
}

__attribute__((weak)) void Soc_estimation_ISR(void)
{
    __DSB();
}

__attribute__((weak)) void Passive_Cell_balancing_ISR(void)
{
    __DSB();
}