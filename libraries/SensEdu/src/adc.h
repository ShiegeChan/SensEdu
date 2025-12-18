#ifndef __ADC_H__
#define __ADC_H__

#include "libs.h"

#ifdef __cplusplus
extern "C" {
#endif

// redefine pure analog pins
#define A8 (86u)
#define A9 (87u)
#define A10 (88u)
#define A11 (89u)

typedef enum {
    ADC_ERROR_NO_ERRORS = 0x00,
    ADC_ERROR_WRONG_ADC_INSTANCE = 0x01,
    ADC_ERROR_INIT = 0x02,
    ADC_ERROR_INIT_PIN_NUMBER = 0x03,
    ADC_ERROR_INIT_PIN_ARRAY = 0x04,
    ADC_ERROR_INIT_SAMPLING_RATE = 0x05,
    ADC_ERROR_INIT_DMA = 0x06,
    ADC_ERROR_INIT_ONE_SHOT_SR = 0x07,
    ADC_ERROR_PICKED_WRONG_CHANNEL = 0x08,
    ADC_ERROR_ENABLE_FAIL = 0x09,
    ADC_ERROR_DISABLE_FAIL = 0x0A,
    ADC_ERROR_SOFT_POLLING_IN_DMA_MODE = 0x0B,
    ADC_ERROR_SOFT_POLLING_ADC_NOT_STARTED= 0x0C,

    ADC_ERROR_UNDEFINED_BEHAVIOUR = 0xA0,
    ADC_ERROR_PLL_CONFIG = 0xA1,
    ADC_ERROR_CHANNEL_SETTING = 0xA2,
    ADC_ERROR_SAMPLE_TIME_SETTING = 0xA3,
    ADC_ERROR_WRONG_OPERATION_MODE = 0xA4,
    ADC_ERROR_WRONG_DATA_MANAGEMENT_MODE = 0xA5
} ADC_ERROR;

typedef enum {
    SENSEDU_ADC_MODE_POLLING_ONE_SHOT = 0x01,   // ADC performs exactly one sequence and stops
    SENSEDU_ADC_MODE_POLLING_CONT = 0x02,       // ADC runs indefinitely until stopped
    SENSEDU_ADC_MODE_DMA_NORMAL = 0x03,         // DMA fills the buffer and stops
    SENSEDU_ADC_MODE_DMA_CIRCULAR = 0x04,       // DMA fills the buffer "in a ring", wrapping indefinitely
} SENSEDU_ADC_MODE;

typedef enum {
    SENSEDU_ADC_SR_MODE_FREE = 0x00,    // As fast as possible
    SENSEDU_ADC_SR_MODE_FIXED = 0x01    // Fixed timer-triggered rate
} SENSEDU_ADC_SR_MODE;

typedef struct {
    ADC_TypeDef* adc;               // ADC instance (ADC1/2/3)
    uint8_t* pins;                  // Array of pins to sample
    uint8_t pin_num;                // Number of pins in pin array

    SENSEDU_ADC_SR_MODE sr_mode;    // FREE: free-run; FIXED: with timer-triggered rate
    uint32_t sampling_rate_hz;      // SR in Hz (if sr_mode = FIXED)
    
    SENSEDU_ADC_MODE adc_mode;      // Polling one-shot/continuous, or DMA normal/circular
    uint16_t* mem_address;          // Destination buffer for DMA (if adc_mode = DMA_xxx)
    uint16_t mem_size;              // DMA buffer length in samples (if adc_mode = DMA_xxx)
} SensEdu_ADC_Settings;

void SensEdu_ADC_Init(SensEdu_ADC_Settings* new_settings);
void SensEdu_ADC_Enable(ADC_TypeDef* adc);
void SensEdu_ADC_Disable(ADC_TypeDef* adc);
void SensEdu_ADC_Start(ADC_TypeDef* adc);

uint16_t SensEdu_ADC_ReadConversion(ADC_TypeDef* adc);
uint16_t* SensEdu_ADC_ReadSequence(ADC_TypeDef* adc);

void SensEdu_ADC_EnableOverrunInterrupt(ADC_TypeDef* adc);
void SensEdu_ADC_DisableOverrunInterrupt(ADC_TypeDef* adc);
bool SensEdu_ADC_IsOverrun(ADC_TypeDef* adc);
void SensEdu_ADC_ClearOverrun(ADC_TypeDef* adc);
uint32_t SensEdu_ADC_GetOverrunCount(ADC_TypeDef* adc);

bool SensEdu_ADC_IsDmaTransferComplete(ADC_TypeDef* adc);
void SensEdu_ADC_ClearDmaTransferComplete(ADC_TypeDef* adc);
bool SensEdu_ADC_IsDmaHalfTransferComplete(ADC_TypeDef* adc);
void SensEdu_ADC_ClearDmaHalfTransferComplete(ADC_TypeDef* adc);

ADC_ERROR ADC_GetError(void);
void ADC_SetDmaTransferComplete(ADC_TypeDef* adc);
void ADC_SetDmaHalfTransferComplete(ADC_TypeDef* adc);


#ifdef __cplusplus
}
#endif

#endif // __ADC_H__
