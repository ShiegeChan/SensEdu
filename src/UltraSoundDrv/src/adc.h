#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "UltraSoundDrv.h"

typedef enum {
    ADC_ERROR_NO_ERRORS = 0x00,
    ADC_ERROR_WRONG_ADC = 0x01, // you can only use ADC1 or ADC2
    ADC_ERROR_PLL_CONFIG = 0x02,
    ADC_ERROR_ADC_CONFIG_VOLTAGE_REGULATOR = 0x03,
    ADC_ERROR_ADC_DISABLE_FAIL = 0x04,
    ADC_ERROR_ADC_ENABLE_FAIL = 0x05,
    ADC_ERROR_PICKED_WRONG_CHANNEL = 0x06,
    ADC_ERROR_WRONG_SEQUENCE = 0x07
} ADC_ERROR;

typedef struct {
    uint8_t* adc_pins;
    volatile uint8_t eoc_flag;
    uint8_t tim_trigger;
    uint8_t conv_length;
    uint16_t sequence_data[16];
} ADC_Settings;

ADC_ERROR ADC_GetError(void);
void ADC_InitPeriph(ADC_TypeDef* ADC, uint8_t* arduino_pins, uint8_t adc_pin_num, uint8_t tim_trigger);
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