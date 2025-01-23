#ifndef __SENSEDU_H__
#define __SENSEDU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>

#include "stm32h747xx.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_pwr.h"

#include "dac.h"
#include "timer.h"
#include "dma.h"

// redefine pure analog pins
#define A8 (86u)
#define A9 (87u)
#define A10 (88u)
#define A11 (89u)

typedef enum {
    SENSEDU_NO_ERRORS = 0x0000,
    SENSEDU_ERROR_TIMER = 0x1000,
    SENSEDU_ERROR_ADC = 0x2000,
    SENSEDU_ERROR_DMA = 0x3000,
    SENSEDU_ERROR_DAC = 0x4000
} SENSEDU_ERROR;

typedef enum {
    SENSEDU_ADC_MODE_ONE_SHOT = 0x01,
    SENSEDU_ADC_MODE_CONT = 0x02,
    SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED = 0x03
} SENSEDU_ADC_MODE;

typedef enum {
    SENSEDU_ADC_DMA_CONNECT = 0x01,
    SENSEDU_ADC_DMA_DISCONNECT = 0x02
} SENSEDU_ADC_DMA;



SENSEDU_ERROR SensEdu_GetError(void);
void SensEdu_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, SENSEDU_ADC_MODE mode, uint32_t trigger_freq, SENSEDU_ADC_DMA adc_dma);

void SensEdu_TIMER_Init();
void SensEdu_Delay_us(uint32_t delay_value);

void SensEdu_ADC_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, SENSEDU_ADC_MODE mode, uint32_t trigger_freq, SENSEDU_ADC_DMA adc_dma);
void SensEdu_ADC_Enable(ADC_TypeDef* ADC);
void SensEdu_ADC_Disable(ADC_TypeDef* ADC);
void SensEdu_ADC_Start(ADC_TypeDef* ADC);
uint16_t* SensEdu_ADC_Read(ADC_TypeDef* ADC);
uint8_t get_msg();

void SensEdu_DMA_Init(uint16_t* mem_address, const uint16_t mem_size);
void SensEdu_DMA_Enable(uint16_t* mem_address, const uint16_t mem_size);
uint8_t SensEdu_DMA_GetTransferStatus(void);
void SensEdu_DMA_ClearTransferStatus(void);

#ifdef __cplusplus
}
#endif

#endif // __SENSEDU_H__