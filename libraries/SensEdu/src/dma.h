#ifndef __DMA_H__
#define __DMA_H__

#include "libs.h"
#include "dac.h"

#ifdef __cplusplus
extern "C" {
#endif

// MPU calculations:
//
// check e.g. LL_MPU_REGION_SIZE_32B mapping 
// to understand region size calculations

// calculates next power of two, based on how many leading zeros in binary
// warning: minimum value is 32B
#define MPU_NEXT_POWER_OF_2(x) \
    ((x) <= 32 ? 32 : (1 << (32 - __builtin_clz((x) - 1))))

// calculated logarithm for power of two numbers
#define MPU_LOG_BASE2(x) \
    ((x) == 0 ? -1 : (31 - __builtin_clz(x)))

#define MPU_REGION_SIZE_ATTRIBUTE(x) \
    ((uint32_t)(MPU_LOG_BASE2(MPU_NEXT_POWER_OF_2(x)) - 1) << MPU_RASR_SIZE_Pos)

// DMA buffers:
//
// aligned with power of two buffer size (required for MPU config)
// hard coded for 16bit variable
#define SENSEDU_DMA_BUFFER(name, size) \
    uint16_t name[MPU_NEXT_POWER_OF_2(size * 2) / 2] \
    __attribute__((aligned(MPU_NEXT_POWER_OF_2(size * 2))))

// Compatibility macro for DMA buffer, use SENSEDU_DMA_BUFFER instead
#define SENSEDU_DAC_BUFFER(name, size) \
    SENSEDU_DMA_BUFFER(name, size)

// Compatibility macro for DMA buffer, use SENSEDU_DMA_BUFFER instead
#define SENSEDU_ADC_BUFFER(name, size) \
    SENSEDU_DMA_BUFFER(name, size)

typedef enum {
    DMA_ERROR_NO_ERRORS = 0x00,                     // Everything is fine
    DMA_ERROR_INIT = 0x01,                          // General initialization error
    DMA_ERROR_ENABLED_BEFORE_INIT = 0x02,           // DMA stream was already running during init attempt
    DMA_ERROR_ENABLED_BEFORE_ENABLE = 0x03,         // DMA stream was already running during enable attempt
    DMA_ERROR_ADC_WRONG_INSTANCE = 0x04,            // Unexpected ADC instance (ADC1, ADC2, ADC3)
    DMA_ERROR_DAC_WRONG_INSTANCE = 0x05,            // Unexpected DAC instance (DAC_CH1, DAC_CH2)
    DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR = 0x06,  // Raised interrupt Transfer Error for ADC transfer
    DMA_ERROR_DAC_INTERRUPT_TRANSFER_ERROR = 0x07,  // Raised interrupt Transfer Error for DAC transfer
    DMA_ERROR_MPU_NO_REGION_AVAILABLE = 0x08,       // Attempt to define more than 4 buffers, library designed only for x4 MPU regions

    DMA_ERROR_UNEXPECTED_FLAG_MASK = 0xA0,          // Unexpected status or clear mask for DMA flags
    DMA_ERROR_INTERRUPTS_NOT_CLEARED = 0xA1,        // Status DMA flags are not cleared
    DMA_ERROR_MPU_BASE_NOT_ALIGNED = 0xA2           // Failed MPU address calculations
} DMA_ERROR;

DMA_ERROR DMA_GetError(void);

void DMA_InitDmaForAdc(ADC_TypeDef* adc, uint16_t* buf, size_t buf_samples);
void DMA_InitDmaForDac(DAC_Channel* dac_ch, uint16_t* buf, size_t buf_samples, SENSEDU_DAC_MODE wave_mode);
void DMA_EnableDmaForAdc(ADC_TypeDef* adc);
void DMA_EnableDmaForDac(DAC_Channel* dac_ch);
void DMA_DisableDmaForAdc(ADC_TypeDef* adc);
void DMA_DisableDmaForDac(DAC_Channel* dac_ch);

#ifdef __cplusplus
}
#endif

#endif // __DMA_H__
