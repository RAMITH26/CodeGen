
#include "init_handlers.h"
#include "task_config.h"

/* Forward declarations for ASW init functions and names.
   These should be provided by ASW modules; declare as extern so the linker
   will bind to the ASW implementations. Names are strict per requirements. */
extern void App_M1_Init(void);
extern void Demo_Init(void);
extern void Test_Init(void);
extern void Release_test_asw_Init(void);

/* Deinitialization prototypes */
extern void App_M1_Deinit(void);
extern void Demo_Deinit(void);
extern void Test_Deinit(void);
extern void Release_test_asw_Deinit(void);

/* Ordered list of init functions.
   Order determined to ensure system brings up services before dependents.
   For this project all ASW modules are in Init phase. Order chosen:
   1) Demo (address managers/services)
   2) Test (test harness)
   3) App_M1 (application logic)
   4) Release_test_asw (top-level ASW)
*/
static InitFunc_t g_init_list[] =
{
#if TASK_ENABLE_DEMO
    Demo_Init,
#endif
#if TASK_ENABLE_TEST
    Test_Init,
#endif
#if TASK_ENABLE_APP_M1
    App_M1_Init,
#endif
#if TASK_ENABLE_RELEASE_TEST
    Release_test_asw_Init,
#endif
};

/* Ordered list of deinit functions (reverse order of init) */
static InitFunc_t g_deinit_list[] =
{
#if TASK_ENABLE_RELEASE_TEST
    Release_test_asw_Deinit,
#endif
#if TASK_ENABLE_APP_M1
    App_M1_Deinit,
#endif
#if TASK_ENABLE_TEST
    Test_Deinit,
#endif
#if TASK_ENABLE_DEMO
    Demo_Deinit,
#endif
};

static uint32_t g_init_count = (uint32_t)(sizeof(g_init_list) / sizeof(g_init_list[0]));
static uint32_t g_deinit_count = (uint32_t)(sizeof(g_deinit_list) / sizeof(g_deinit_list[0]));

void InitHandlers_RegisterAll(void)
{
    /* No dynamic registration required in static generated code.
       Function retained for API completeness. */
    /* Intentionally empty */
    (void)g_init_count;
    (void)g_deinit_count;
}

void InitHandlers_InitAll(void)
{
    uint32_t i;
    for (i = 0u; i < g_init_count; ++i)
    {
        if (g_init_list[i] != (InitFunc_t)0u)
        {
            g_init_list[i]();
        }
    }
}

void InitHandlers_DeinitAll(void)
{
    uint32_t i;
    for (i = 0u; i < g_deinit_count; ++i)
    {
        if (g_deinit_list[i] != (InitFunc_t)0u)
        {
            g_deinit_list[i]();
        }
    }
}

