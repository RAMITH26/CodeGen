/* ===== File: CodeGen_BMS_Addressing.h ===== */
#ifndef BMS_ADDRESSING_H
#define BMS_ADDRESSING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "CodeGen_BMS_config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Address Assignment CAN Message Identifiers */
#define BMS_ADDRESSING_BROADCAST_ID     (0x18FFF000UL)
#define BMS_ADDRESSING_REQUEST_BASE_ID  (0x18FFF000UL)
#define BMS_ADDRESSING_RESPONSE_BASE_ID (0x18FFF800UL)

/* Address Assignment Message Formats */
#define BMS_ADDRESSING_MSG_LENGTH       (8U)
#define BMS_ADDRESSING_TOKEN_INDEX      (0U)
#define BMS_ADDRESSING_ADDR_INDEX       (1U)
#define BMS_ADDRESSING_CRC_INDEX        (2U)

/* Bitmap utilities derived from configuration */
#if (defined(BMS_MAX_RACK_COUNT) && (BMS_MAX_RACK_COUNT > 0))
/* Compute number of bytes required to hold BMS_MAX_RACK_COUNT bits */
#define BMS_ADDRESSING_BITMAP_BYTES (((BMS_MAX_RACK_COUNT) + 7U) / 8U)
#else
#define BMS_ADDRESSING_BITMAP_BYTES (1U)
#endif

/* Address Assignment States */
typedef enum {
    BMS_ADDR_STATE_INIT = 0,
    BMS_ADDR_STATE_REQUESTING,
    BMS_ADDR_STATE_ASSIGNED,
    BMS_ADDR_STATE_COLLISION,
    BMS_ADDR_STATE_ERROR
} BMS_AddressState;

/* Address Assignment Control Structure
   Reordered members for natural alignment and efficient access on Cortex-M4 */
typedef struct {
    uint32_t           addressing_token;
    uint16_t           address_crc;
    BMS_AddressState   state;
    uint8_t            proposed_address;
    uint8_t            retry_count;
} BMS_AddressControl;

/* Address Management Configuration
   Bitmap size is computed at compile time to avoid repeated calculations. */
typedef struct {
    uint8_t rack_bitmap[BMS_ADDRESSING_BITMAP_BYTES];
    uint8_t assigned_addresses[BMS_MAX_RACK_COUNT];
    uint8_t unassigned_rack_count;
    uint8_t current_retry_count;
} BMS_AddressManager;

/* Function Prototypes for Address Assignment */
HAL_StatusTypeDef BMS_InitAddressAssignment(void);
HAL_StatusTypeDef BMS_PerformDynamicAddressing(void);
HAL_StatusTypeDef BMS_HandleAddressingResponse(uint32_t can_id, uint8_t* payload);
HAL_StatusTypeDef BMS_ResolveAddressCollision(uint8_t conflicting_address);

/* Utility Function Prototypes */
bool BMS_IsAddressAvailable(uint8_t address);
uint16_t BMS_ComputeAddressCRC(const uint8_t* data, size_t length);
uint8_t BMS_GenerateAddressingToken(void);

/* External Global Variables */
extern BMS_AddressManager g_bms_address_manager;
extern BMS_AddressControl g_bms_address_control;

/* Inline bitmap helpers for extremely hot code paths.
   These are implemented as static inline to avoid function call overhead
   while preserving the original API surface (no public prototypes changed). */
static inline bool BMS_RackBitmapTest(const uint8_t *bitmap, uint16_t index)
{
    const uint16_t byteIndex = (uint16_t)(index >> 3);
    const uint8_t  bitMask   = (uint8_t)(1U << (index & 7U));
    return (bitmap[byteIndex] & bitMask) != 0U;
}

static inline void BMS_RackBitmapSet(uint8_t *bitmap, uint16_t index)
{
    const uint16_t byteIndex = (uint16_t)(index >> 3);
    const uint8_t  bitMask   = (uint8_t)(1U << (index & 7U));
    bitmap[byteIndex] |= bitMask;
}

static inline void BMS_RackBitmapClear(uint8_t *bitmap, uint16_t index)
{
    const uint16_t byteIndex = (uint16_t)(index >> 3);
    const uint8_t  bitMask   = (uint8_t)(1U << (index & 7U));
    bitmap[byteIndex] &= (uint8_t)(~bitMask);
}

/* Fast inline token generator using LCG seeded once.
   Uses HAL_GetTick() for seed entropy; avoids heavy RNG APIs in hot paths. */
static inline uint8_t BMS_GenerateAddressingToken_inline(void)
{
    /* Static ensures state is preserved per translation unit; internal linkage. */
    static uint32_t lcg_state = 0U;
    if (lcg_state == 0U)
    {
        /* Seed with tick count and address of state variable to add variability. */
        lcg_state = (uint32_t)HAL_GetTick();
        /* Mix in lower bits of the pointer to add more entropy on repeated boots. */
        lcg_state ^= (uint32_t)(uintptr_t)&lcg_state;
        /* Avoid zero state: LCG multiplier will not progress from zero. */
        if (lcg_state == 0U)
        {
            lcg_state = 0xA5A5A5A5U;
        }
    }
    /* LCG parameters from Numerical Recipes: good tradeoff for speed */
    lcg_state = (uint32_t)(lcg_state * 1664525U + 1013904223U);
    return (uint8_t)(lcg_state >> 24);
}

/* Provide a mapping from the public prototype to the fast inline implementation.
   This keeps the external API unchanged while allowing compilers to inline calls
   when included headers are visible to the compiler. */
static inline uint8_t BMS_GenerateAddressingToken(void)
{
    return BMS_GenerateAddressingToken_inline();
}

#ifdef __cplusplus
}
#endif

#endif /* BMS_ADDRESSING_H */