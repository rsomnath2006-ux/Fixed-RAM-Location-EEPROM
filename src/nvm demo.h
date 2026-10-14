#ifndef NVM_DEMO_H
#define NVM_DEMO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NVM_NUM_BLOCKS      4u
#define NVM_MIRROR_SLOTS    4u
#define NVM_MIRROR_SIZE     64u

typedef enum
{
    NVM_OK = 0,
    NVM_ERR_BAD_ID,
    NVM_ERR_BAD_SLOT,
    NVM_ERR_NO_RAM,
    NVM_ERR_ADDR_MISMATCH,
    NVM_ERR_TOO_LONG
} NvM_Status_t;

typedef struct
{
    uint8_t   ram_block;
    uint8_t   mirror_idx;
    uint8_t  *ram_addr;
    uint16_t  length;
} NvM_BlockCfg_t;

extern uint8_t        NvM_MirrorTable[NVM_MIRROR_SLOTS][NVM_MIRROR_SIZE];
extern NvM_BlockCfg_t NvM_BlockCfg[NVM_NUM_BLOCKS];

void         NvM_Init(void);
NvM_Status_t NvM_ReadAll(void);
NvM_Status_t NvM_WriteAll(void);
NvM_Status_t NvM_SetRamBlockStatus(uint8_t block_id, bool block_changed);
bool         NvM_IsValid(uint8_t block_id);
bool         NvM_PeekNv(uint8_t block_id, void *dst, uint16_t len);

void Demo_Init(void);
void Demo_Step(void);

extern NvM_Status_t Demo_LastTamper;

#endif
