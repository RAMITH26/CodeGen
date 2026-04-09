
#ifndef INIT_HANDLERS_H
#define INIT_HANDLERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Initialization function prototype type */
typedef void (*InitFunc_t)(void);

/* Register initialization functions in correct order and allow invocation */
void InitHandlers_RegisterAll(void);
void InitHandlers_InitAll(void);
void InitHandlers_DeinitAll(void);

#ifdef __cplusplus
}
#endif

#endif /* INIT_HANDLERS_H */

