#ifndef __DAC_H__
#define __DAC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libs.h"

typedef enum {
    DAC_ERROR_NO_ERRORS = 0x00,
    DAC_ERROR_ALREADY_ENABLED = 0x01,
    DAC_ERROR_WRONG_DAC_CHANNEL = 0x02,
    DAC_ERROR_INIT_SAMPLING_FREQ_TOO_HIGH = 0x03,
    DAC_ERROR_INIT_DMA_MEMORY = 0x04,
    DAC_ERROR_INIT_CYCLE_NUM = 0x05,
    
    DAC_ERROR_DMA_UNDERRUN = 0xA0
} DAC_ERROR;

typedef enum {
    SENSEDU_DAC_MODE_CONTINUOUS_WAVE = 0x00,
    SENSEDU_DAC_MODE_SINGLE_WAVE = 0x01,
    SENSEDU_DAC_MODE_BURST_WAVE = 0x02
} SENSEDU_DAC_MODE;

typedef struct
{
} DAC_Channel;

typedef struct {
    DAC_Channel* dac_channel;               // DAC_CH1 or DAC_CH2
    uint32_t sampling_freq;
    uint16_t* mem_address;                  // Address of the array's first element written to DAC
    uint16_t mem_size;                      // Number of array elements
    SENSEDU_DAC_MODE wave_mode;
    uint16_t burst_num;                     // If in burst mode, how many array cycles to write
} SensEdu_DAC_Settings;

#define DAC_CH1                ((DAC_Channel *) 0)
#define DAC_CH2                ((DAC_Channel *) 1)


void SensEdu_DAC_Init(SensEdu_DAC_Settings* dac_settings);
void SensEdu_DAC_Enable(DAC_Channel* dac_channel);
void SensEdu_DAC_Disable(DAC_Channel* dac_channel);

uint8_t SensEdu_DAC_GetBurstCompleteFlag(DAC_Channel* dac_channel);
void SensEdu_DAC_ClearBurstCompleteFlag(DAC_Channel* dac_channel);

DAC_ERROR DAC_GetError(void);
void DAC_WriteDataManually(DAC_Channel* dac_channel, uint16_t data);
uint16_t DAC_ReadCurrentOutputData(DAC_Channel* dac_channel);

void DAC_TransferCompleteDMAinterrupt(DAC_Channel* dac_channel);


#ifdef __cplusplus
}
#endif

#endif // __DAC_H__