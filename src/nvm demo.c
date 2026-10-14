#include "nvm demo.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "sdk_project_config.h"

#define BTN_SAVE    12
#define BTN_TAMPER  13
#define BTN_ACTIVE_HIGH 1
#define FLASH_MAGIC 0xA5C35A3Cu
#define ADC_AVG     8u
#define TEMP_ID     1u
#define TEMP_SLOT   3u
#define TEMP_MIN_C  10
#define TEMP_MAX_C  40
#define TEMP_DEFAULT_C 25
#define DISPLAY_EVERY  5u

#define ADDR(p)  ((unsigned long)(uintptr_t)(p))
#define TEMP_RAM (NvM_MirrorTable[TEMP_SLOT])
#define CASE(x)  case x: return #x;

uint8_t NvM_MirrorTable[NVM_MIRROR_SLOTS][NVM_MIRROR_SIZE]
    __attribute__((section(".nvm_mirror"), aligned(4)));

static struct { uint32_t magic; uint8_t data[NVM_MIRROR_SIZE]; }
    NvM_FlashSim[NVM_NUM_BLOCKS] __attribute__((section(".nvm_flash"), aligned(4)));

static struct { bool valid, changed; } NvM_BlockAdmin[NVM_NUM_BLOCKS];

NvM_BlockCfg_t NvM_BlockCfg[NVM_NUM_BLOCKS] =
{
    [TEMP_ID] = { .ram_block = 1u, .mirror_idx = TEMP_SLOT,
                  .ram_addr = NULL, .length = sizeof(int16_t) }
};

NvM_Status_t Demo_LastTamper = NVM_OK;

static uint8_t tamper_buf[NVM_MIRROR_SIZE], tick;
static bool prev_save, prev_tamper;

static bool NvM_FlashRead(uint8_t id, uint8_t *dst, uint16_t len)
{
    if (NvM_FlashSim[id].magic != FLASH_MAGIC) return false;
    memcpy(dst, NvM_FlashSim[id].data, len);
    return true;
}

static NvM_Status_t NvM_CheckRamLocation(NvM_BlockCfg_t *cfg)
{
    if (cfg->ram_block == 1u)
    {
        uint8_t *fixed;
        if (cfg->mirror_idx >= NVM_MIRROR_SLOTS) return NVM_ERR_BAD_SLOT;
        fixed = NvM_MirrorTable[cfg->mirror_idx];
        if (cfg->ram_addr == NULL) cfg->ram_addr = fixed;
        else if (cfg->ram_addr != fixed) return NVM_ERR_ADDR_MISMATCH;
    }
    else if (cfg->ram_addr == NULL) return NVM_ERR_NO_RAM;

    return NVM_OK;
}

void NvM_Init(void)
{
    memset(NvM_MirrorTable, 0, sizeof(NvM_MirrorTable));
    memset(NvM_BlockAdmin, 0, sizeof(NvM_BlockAdmin));
}

NvM_Status_t NvM_ReadAll(void)
{
    NvM_Status_t result = NVM_OK, st;
    uint8_t id;

    for (id = 1u; id < NVM_NUM_BLOCKS; id++)
    {
        NvM_BlockCfg_t *cfg = &NvM_BlockCfg[id];

        if (cfg->length == 0u) continue;

        st = (cfg->length > NVM_MIRROR_SIZE) ? NVM_ERR_TOO_LONG
                                             : NvM_CheckRamLocation(cfg);
        if (st != NVM_OK) { result = st; continue; }

        NvM_BlockAdmin[id].valid = NvM_FlashRead(id, cfg->ram_addr, cfg->length);
        if (!NvM_BlockAdmin[id].valid) memset(cfg->ram_addr, 0, cfg->length);
        NvM_BlockAdmin[id].changed = false;
    }
    return result;
}

NvM_Status_t NvM_WriteAll(void)
{
    NvM_Status_t result = NVM_OK, st;
    uint8_t id;

    for (id = 1u; id < NVM_NUM_BLOCKS; id++)
    {
        NvM_BlockCfg_t *cfg = &NvM_BlockCfg[id];

        if (cfg->length == 0u || !NvM_BlockAdmin[id].changed) continue;

        st = NvM_CheckRamLocation(cfg);
        if (st != NVM_OK) { result = st; continue; }

        memcpy(NvM_FlashSim[id].data, cfg->ram_addr, cfg->length);
        NvM_FlashSim[id].magic = FLASH_MAGIC;
        NvM_BlockAdmin[id].changed = false;
    }
    return result;
}

NvM_Status_t NvM_SetRamBlockStatus(uint8_t id, bool changed)
{
    NvM_Status_t st;

    if (id == 0u || id >= NVM_NUM_BLOCKS) return NVM_ERR_BAD_ID;

    st = NvM_CheckRamLocation(&NvM_BlockCfg[id]);
    if (st != NVM_OK) return st;

    if (changed) NvM_BlockAdmin[id].valid = true;
    NvM_BlockAdmin[id].changed = changed;
    return NVM_OK;
}

bool NvM_IsValid(uint8_t id)
{
    return (id < NVM_NUM_BLOCKS) && NvM_BlockAdmin[id].valid;
}

bool NvM_PeekNv(uint8_t id, void *dst, uint16_t len)
{
    return (id < NVM_NUM_BLOCKS) && (len <= NVM_MIRROR_SIZE) &&
           NvM_FlashRead(id, dst, len);
}

static void Log(const char *fmt, ...)
{
    char buf[128], out[256];
    uint32_t n = 0u;
    const char *p = buf;
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    for (; *p && n < sizeof(out) - 2u; p++)
    {
        if (*p == '\n') out[n++] = '\r';
        out[n++] = *p;
    }
    (void)LPUART_DRV_SendDataBlocking(INST_LPUART_1, (const uint8_t *)out, n, 200u);
}

static uint16_t Adc_Read(void)
{
    uint32_t sum = 0u, i;
    uint16_t r = 0u;

    for (i = 0u; i < ADC_AVG; i++)
    {
        ADC_DRV_ConfigChan(INST_ADC_CONFIG_1, 0u, &adc_config_1_ChnConfig0);
        ADC_DRV_WaitConvDone(INST_ADC_CONFIG_1);
        ADC_DRV_GetChanResult(INST_ADC_CONFIG_1, 0u, &r);
        sum += r;
    }
    return (uint16_t)(sum / ADC_AVG);
}

static bool Button(uint32_t pin)
{
    return (((PINS_DRV_ReadPins(PTC) >> pin) & 1u) != 0u) == BTN_ACTIVE_HIGH;
}

static int16_t Ram_Get(void)
{
    int16_t v;
    memcpy(&v, TEMP_RAM, sizeof(v));
    return v;
}

static void Ram_Set(int16_t v) { memcpy(TEMP_RAM, &v, sizeof(v)); }

static const char *Name(NvM_Status_t s)
{
    switch (s)
    {
        CASE(NVM_OK)
        CASE(NVM_ERR_BAD_ID)
        CASE(NVM_ERR_BAD_SLOT)
        CASE(NVM_ERR_NO_RAM)
        CASE(NVM_ERR_ADDR_MISMATCH)
        default: return "NVM_ERR_TOO_LONG";
    }
}

void Demo_Init(void)
{
    NvM_Status_t s;

    CLOCK_DRV_Init(&clockMan1_InitConfig0);
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);
    ADC_DRV_ConfigConverter(INST_ADC_CONFIG_1, &adc_config_1_ConvConfig0);
    ADC_DRV_AutoCalibration(INST_ADC_CONFIG_1);
    LPUART_DRV_Init(INST_LPUART_1, &lpUartState0, &lpuart_0_InitConfig0);

    NvM_Init();
    s = NvM_ReadAll();

    Log("\n=== NvM temperature demo (RAM_BLOCK = 1) ===\n");
    Log("ReadAll : %s\n", Name(s));
    Log("Mirror slot %u : address 0x%08lX\n", TEMP_SLOT, ADDR(TEMP_RAM));

    if (NvM_IsValid(TEMP_ID))
    {
        Log("Restored from NVM: %d C\n", Ram_Get());
    }
    else
    {
        Ram_Set(TEMP_DEFAULT_C);
        (void)NvM_SetRamBlockStatus(TEMP_ID, true);
        (void)NvM_WriteAll();
        Log("First boot: default %d C saved to NVM\n", TEMP_DEFAULT_C);
    }

    Log("SW2 = save to NVM, SW3 = tamper test, turn the knob to change temp\n");

    prev_save   = Button(BTN_SAVE);
    prev_tamper = Button(BTN_TAMPER);
}

void Demo_Step(void)
{
    bool b_save   = Button(BTN_SAVE);
    bool b_tamper = Button(BTN_TAMPER);
    NvM_BlockCfg_t *cfg = &NvM_BlockCfg[TEMP_ID];
    NvM_Status_t s;

    Ram_Set((int16_t)(TEMP_MIN_C +
            ((int32_t)Adc_Read() * (TEMP_MAX_C - TEMP_MIN_C)) / 4095));

    if (b_save && !prev_save)
    {
        s = NvM_SetRamBlockStatus(TEMP_ID, true);
        if (s == NVM_OK) s = NvM_WriteAll();
        Log("\nSAVE %d C -> NVM : %s\n", Ram_Get(), Name(s));
    }

    if (b_tamper && !prev_tamper)
    {
        uint8_t *good = cfg->ram_addr;

        cfg->ram_addr = tamper_buf;
        Demo_LastTamper = NvM_SetRamBlockStatus(TEMP_ID, true);
        cfg->ram_addr = good;

        Log("\nTAMPER: block pointed at 0x%08lX (expected 0x%08lX) -> %s\n",
            ADDR(tamper_buf), ADDR(good), Name(Demo_LastTamper));
    }

    prev_save   = b_save;
    prev_tamper = b_tamper;

    if (++tick >= DISPLAY_EVERY)
    {
        int16_t nv = 0;
        tick = 0u;
        (void)NvM_PeekNv(TEMP_ID, &nv, sizeof(nv));

        Log("\rNVM=%2d C  RAM=%2d C  RAM addr=0x%08lX  %s   ",
            nv, Ram_Get(), ADDR(cfg->ram_addr),
            (nv == Ram_Get()) ? "SAVED" : "UNSAVED");
    }

    OSIF_TimeDelay(100u);
}
