#ifndef __DMA_H__
#define __DMA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libs.h"
#include "dac.h"

// calculates next power of two, based on how many leading zeros in binary
#define NEXT_POWER_OF_2(x) \
    ((x) <= 1 ? 1 : (1 << (32 - __builtin_clz((x) - 1))))

// aligned with power of two buffer size (required for MPU config)
// hard coded for 16bit variable
#define SENSEDU_DAC_BUFFER(name, user_size) \
    uint16_t name[NEXT_POWER_OF_2(user_size * 2) / 2] \
    __attribute__((aligned(NEXT_POWER_OF_2(user_size * 2))))

// aligned with DCache line size
#define SENSEDU_ADC_BUFFER(name, size) uint16_t name[size] \
    __attribute__((aligned(__SCB_DCACHE_LINE_SIZE)))

typedef enum {
    DMA_ERROR_NO_ERRORS = 0x00,
    DMA_ERROR_ENABLED_BEFORE_INIT = 0x01,
    DMA_ERROR_INTERRUPTS_NOT_CLEARED = 0x02,
    DMA_ERROR_ADC1_ENABLE_BEFORE_INIT = 0x03,
    DMA_ERROR_ADC_INTERRUPT_TRANSFER_ERROR = 0x04,
    DMA_ERROR_DAC_INTERRUPT_TRANSFER_ERROR = 0x05,
    DMA_ERROR_MEMORY_WRONG_SIZE = 0x06,
    DMA_ERROR_ENABLED_BEFORE_ENABLE = 0x07,
    DMA_ERROR_ADC_WRONG_INPUT = 0x08
} DMA_ERROR;

uint8_t SensEdu_DMA_GetADCTransferStatus(ADC_TypeDef* adc);
void SensEdu_DMA_ClearADCTransferStatus(ADC_TypeDef* adc);

DMA_ERROR DMA_GetError(void);
void DMA_ADCInit(ADC_TypeDef* adc, uint16_t* mem_address, const uint16_t mem_size);
void DMA_DAC1Init(uint16_t* mem_address, const uint16_t mem_size, SENSEDU_DAC_MODE wave_mode);
void DMA_ADCEnable(ADC_TypeDef* adc);
void DMA_DAC1Enable(void);
void DMA_ADCDisable(ADC_TypeDef* adc);
void DMA_DAC1Disable(void);


#ifdef __cplusplus
}
#endif

#endif // __DMA_H__