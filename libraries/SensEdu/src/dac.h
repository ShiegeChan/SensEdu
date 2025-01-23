#ifndef __DAC_H__
#define __DAC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "SensEdu.h"

typedef enum {
    DAC_ERROR_NO_ERRORS = 0x00,
    DAC_ERROR_INIT_FAILED = 0x01,
    DAC_ERROR_WRONG_CHANNEL = 0x02,
    DAC_ERROR_DMA_UNDERRUN = 0x03,
    DAC_ERROR_ENABLED_BEFORE_INIT = 0x04
} DAC_ERROR;

typedef enum {
    SENSEDU_DAC_MODE_CONTINUOUS_WAVE = 0x00,
    SENSEDU_DAC_MODE_SINGLE_WAVE = 0x01,
    SENSEDU_DAC_MODE_BURST_WAVE = 0x02
} SENSEDU_DAC_MODE;

typedef struct {
    DAC_TypeDef* dac;               // DAC1 or DAC2
    uint32_t sampling_freq;
    uint16_t* mem_address;          // Address of the array's first element written to DAC
    uint16_t mem_size;              // Number of array elements
    SENSEDU_DAC_MODE wave_mode;
    uint16_t burst_num;             // If in burst mode, how many array cycles to write
} SensEdu_DAC_Settings;

void SensEdu_DAC_Init(SensEdu_DAC_Settings* dac_settings);
void SensEdu_DAC_Enable(DAC_TypeDef* dac);
void SensEdu_DAC_Disable(DAC_TypeDef* dac);

DAC_ERROR DAC_GetError(void);
void DAC_WriteDataManually(DAC_TypeDef* dac, uint16_t data);
uint16_t DAC_ReadCurrentOutputData(DAC_TypeDef* dac);


#ifdef __cplusplus
}
#endif

#endif // __DAC_H__