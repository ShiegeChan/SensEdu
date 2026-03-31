/**
 * @file dac.h
 * @brief Public API for DAC driver (DMA-based waveform generation).
 *
 * This module provides:
 * - DAC waveform generation using DMA
 * - Timer-triggered DAC sampling
 * - Burst and continuous waveform modes
 *
 * User constraints:
 * - SensEdu_DAC_Init() must be called before Enable()
 * - Both channels must use identical sampling_freq
 *
 * Notes:
 * - DAC_Channel is an opaque identifier implemented as a pointer with DAC_CH1 and DAC_CH2 only valid values
 * - DAC output is driven by DMA + Timer
 * - Both DAC channels share the same timer, meaning they MUST use the same sampling frequency
 * - The timer runs continuously once initialized
 * - Calling Enable() starts DMA transfers, not the timer
 * - In burst mode, the waveform repeats automatically until the burst count is reached
 * - Internally, single wave mode is using burst mode with burst count of 1
 * - After a burst completes, the DAC must be re-enabled to restart output
 */

#ifndef __DAC_H__
#define __DAC_H__

#include "libs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DAC_ERROR_NO_ERRORS = 0x00,
    DAC_ERROR_NULL_INPUT_SETTINGS = 0x01,
    DAC_ERROR_ALREADY_ENABLED = 0x02,
    DAC_ERROR_WRONG_DAC_CHANNEL = 0x03,
    DAC_ERROR_INIT_SAMPLING_FREQ_TOO_HIGH = 0x04,
    DAC_ERROR_INIT_SAMPLING_FREQ_MISMATCH = 0x05,
    DAC_ERROR_INIT_DMA_MEMORY = 0x06,
    DAC_ERROR_INIT_CYCLE_NUM = 0x07,
    
    DAC_ERROR_DMA_UNDERRUN = 0xA0,
    DAC_ERROR_TIMEOUT = 0xA1
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
    uint32_t sampling_freq;                 // Sampling rate in Hz
    uint16_t* mem_address;                  // Destination buffer for DMA
    uint16_t mem_size;                      // DMA buffer length in samples
    SENSEDU_DAC_MODE wave_mode;             // Continuous/Single/Burst DAC wave mode
    uint16_t burst_num;                     // Burst cycle in wave_mode = SENSEDU_DAC_MODE_BURST_WAVE
} SensEdu_DAC_Settings;

#define DAC_CH1                ((DAC_Channel *) 0)
#define DAC_CH2                ((DAC_Channel *) 1)

void SensEdu_DAC_Init(SensEdu_DAC_Settings* dac_settings);
void SensEdu_DAC_Enable(DAC_Channel* dac_channel);
void SensEdu_DAC_Disable(DAC_Channel* dac_channel);

bool SensEdu_DAC_GetBurstCompleteFlag(DAC_Channel* dac_channel);
void SensEdu_DAC_ClearBurstCompleteFlag(DAC_Channel* dac_channel);

DAC_ERROR DAC_GetError(void);
void DAC_WriteDataManually(DAC_Channel* dac_channel, uint16_t data);
uint16_t DAC_ReadCurrentOutputData(DAC_Channel* dac_channel);

void DAC_TransferCompleteDmaInterrupt(DAC_Channel* dac_channel);


#ifdef __cplusplus
}
#endif

#endif // __DAC_H__
