
/* This file provides weak stubs to ensure logging symbols exist if CodeGen_Logging
   does not implement specific helpers. These are weak to allow ASW/BSW to override. */

#include "CodeGen_Logging.h"
#include <stdint.h>

__attribute__((weak)) int CodeGen_Logging_Init(void)
{
    return 0;
}

__attribute__((weak)) int CodeGen_Logging_LogEvent(const char *module, const char *event, uint32_t code)
{
    (void)module;
    (void)event;
    (void)code;
    return 0;
}

