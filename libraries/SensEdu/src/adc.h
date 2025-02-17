#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libs.h"

// redefine pure analog pins
#define A8 (86u)
#define A9 (87u)
#define A10 (88u)
#define A11 (89u)

typedef enum {
    ADC_ERROR_NO_ERRORS = 0x00,
    ADC_ERROR_WRONG_ADC = 0x01, // use only ADC1 or ADC2
    ADC_ERROR_PLL_CONFIG = 0x02,
    ADC_ERROR_ADC_CONFIG_VOLTAGE_REGULATOR = 0x03,
    ADC_ERROR_ADC_DISABLE_FAIL = 0x04,
    ADC_ERROR_ADC_ENABLE_FAIL = 0x05,
    ADC_ERROR_PICKED_WRONG_CHANNEL = 0x06,
    ADC_ERROR_WRONG_SEQUENCE = 0x07,
    ADC_ERROR_SAMPLE_TIME_SETTING = 0x08,
    ADC_ERROR_WRONG_OPERATION_MODE = 0x09,
    ADC_ERROR_WRONG_DATA_MANAGEMENT_MODE = 0x0A,
    ADC_ERROR_WRONG_INIT_SETTINGS_PARAMETERS = 0x0B,
    ADC_ERROR_BAD_SAMPLING_FREQ = 0x0C
} ADC_ERROR;

typedef enum {
    SENSEDU_ADC_MODE_ONE_SHOT = 0x01,
    SENSEDU_ADC_MODE_CONT = 0x02,
    SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED = 0x03
} SENSEDU_ADC_CONVMODE;

typedef enum {
    SENSEDU_ADC_DMA_CONNECT = 0x01,
    SENSEDU_ADC_DMA_DISCONNECT = 0x02
} SENSEDU_ADC_DMA;

typedef struct {
    ADC_TypeDef* adc;               // ADC1 or ADC2
    uint8_t* pins;                  // pin array
    uint8_t pin_num;                // how many pins in pin array

    SENSEDU_ADC_CONVMODE conv_mode;
    uint32_t sampling_freq;
    
    SENSEDU_ADC_DMA dma_mode;
    uint16_t* mem_address;          // Address of the array's first element
    uint16_t mem_size;              // Number of array elements
} SensEdu_ADC_Settings;

void SensEdu_ADC_Init(SensEdu_ADC_Settings* adc_settings);
void SensEdu_ADC_Enable(ADC_TypeDef* ADC);
void SensEdu_ADC_Disable(ADC_TypeDef* ADC);
void SensEdu_ADC_Start(ADC_TypeDef* ADC);
uint16_t* SensEdu_ADC_ReadSingleSequence(ADC_TypeDef* ADC);

uint8_t SensEdu_ADC_GetTransferStatus(ADC_TypeDef* adc);
void SensEdu_ADC_ClearTransferStatus(ADC_TypeDef* adc);

void SensEdu_ADC_ShortA4toA9(void);

ADC_ERROR ADC_GetError(void);
void ADC_TransferCompleteDMAinterrupt(ADC_TypeDef* adc);


#ifdef __cplusplus
}
#endif

#endif // __ADC_H__