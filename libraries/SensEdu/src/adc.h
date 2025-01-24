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
    ADC_ERROR_WRONG_DATA_MANAGEMENT_MODE = 0x0A
} ADC_ERROR;

typedef enum {
    SENSEDU_ADC_MODE_ONE_SHOT = 0x01,
    SENSEDU_ADC_MODE_CONT = 0x02,
    SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED = 0x03
} SENSEDU_ADC_MODE;

typedef enum {
    SENSEDU_ADC_DMA_CONNECT = 0x01,
    SENSEDU_ADC_DMA_DISCONNECT = 0x02
} SENSEDU_ADC_DMA;

typedef struct {
    uint8_t* adc_pins;
    volatile uint8_t eoc_flag;
    SENSEDU_ADC_MODE mode;
    SENSEDU_ADC_DMA dma_mode;
    uint8_t conv_length;
    uint16_t sequence_data[16];
} ADC_Settings;

ADC_ERROR ADC_GetError(void);
void ADC_InitPeriph(ADC_TypeDef* ADC, uint8_t* arduino_pins, uint8_t adc_pin_num, SENSEDU_ADC_MODE mode, SENSEDU_ADC_DMA adc_dma);
void ADC_EnablePeriph(ADC_TypeDef* ADC);
void ADC_DisablePeriph(ADC_TypeDef* ADC);
void ADC_StartConversion(ADC_TypeDef* ADC);
uint16_t* ADC_ReadSingleSequence(ADC_TypeDef* ADC);
ADC_Settings* ADC_GetSettings(ADC_TypeDef* ADC);

uint8_t get_adc_msg();

#ifdef __cplusplus
}
#endif

#endif // __ADC_H__