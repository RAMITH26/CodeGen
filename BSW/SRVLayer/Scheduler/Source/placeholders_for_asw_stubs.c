
#include "stm32h7xx_hal.h"

/* Stubs for ASW functions to ensure the generated code compiles even if
   ASW implementations are provided elsewhere. These are weak so linker
   will pick real implementations if present. */

__weak void App_M1_Init(void) { /* Application module init placeholder */ }
__weak void Demo_Init(void) { /* Demo init placeholder */ }
__weak void Test_Init(void) { /* Test init placeholder */ }
__weak void Release_test_asw_Init(void) { /* Release_test_asw init placeholder */ }

__weak void App_M1_Deinit(void) { /* Deinit placeholder */ }
__weak void Demo_Deinit(void) { /* Deinit placeholder */ }
__weak void Test_Deinit(void) { /* Deinit placeholder */ }
__weak void Release_test_asw_Deinit(void) { /* Deinit placeholder */ }

__weak void App_M1_Task(void) { /* Task placeholder - must be provided by ASW */ }
__weak void Demo_Task(void) { /* Task placeholder */ }
__weak void Test_Task(void) { /* Task placeholder */ }
__weak void Release_test_asw_Task(void) { /* Task placeholder */ }

__weak void App_M1_ISR(void) { /* ISR placeholder */ }
__weak void Demo_ISR(void) { /* ISR placeholder */ }
__weak void Test_ISR(void) { /* ISR placeholder */ }
__weak void Release_test_asw_ISR(void) { /* ISR placeholder */ }