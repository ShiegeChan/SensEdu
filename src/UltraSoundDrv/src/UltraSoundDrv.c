#include "UltraSoundDrv.h"
#include "timer.h"
#include "adc.h"


#include "stm32h7xx_hal.h"
#include "stm32h747xx.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_adc.h"

// RCC is configured by arduino by default with SYSCLK = 480MHz, HCLK = 240MHz, PCLK1/PCLK2 = 120MHz

/* General*/
void UltraSoundDrv_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, const uint8_t adc_pin_num, uint8_t tim_trigger, uint32_t trigger_freq) {
    #ifndef ARDUINO_GIGA
        #error Only Arduino Giga R1 is supported
    #endif
    UltraSoundDrv_TIMER_Init();
    UltraSoundDrv_ADC_Init(ADC, adc_pins, adc_pin_num, tim_trigger, trigger_freq);
}

ULTRASOUND_DRV_ERROR UltraSoundDrv_GetError(void) {
    ULTRASOUND_DRV_ERROR error = ULTRASOUND_DRV_NO_ERRORS;

    error |= TIMER_GetError();
    if (error) {
        error |= ULTRASOUND_DRV_ERROR_TIMER;
        return error;
    }

    error |= ADC_GetError();
    if (error) {
        error |= ULTRASOUND_DRV_ERROR_ADC;
        return error;
    }

    return error;
}

/* Timer */
void UltraSoundDrv_TIMER_Init(void) {
    #ifndef ARDUINO_GIGA
        #error Only Arduino Giga R1 is supported
    #endif
    TIMER_Init();
}

void UltraSoundDrv_Delay_us(uint32_t delay_value) {
    TIMER_Delay_us(delay_value);
}

/* ADC */
void UltraSoundDrv_ADC_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, const uint8_t adc_pin_num, uint8_t tim_trigger, uint32_t trigger_freq) {
    #ifndef ARDUINO_GIGA
        #error Only Arduino Giga R1 is supported
    #endif

    if (ADC_GetSettings(ADC)->tim_trigger) {
        TIMER_ADCtrigger_SetFreq(trigger_freq);
    }
    ADC_InitPeriph(ADC, adc_pins, adc_pin_num, tim_trigger);
}

void UltraSoundDrv_ADC_Enable(ADC_TypeDef* ADC) {
    if (ADC_GetSettings(ADC)->tim_trigger) {
        TIMER_ADCtrigger_Enable();
    }

    ADC_EnablePeriph(ADC);
}

void UltraSoundDrv_ADC_Disable(ADC_TypeDef* ADC) {
    ADC_DisablePeriph(ADC);
}

void UltraSoundDrv_ADC_Start(ADC_TypeDef* ADC) {
    ADC_StartConversion(ADC);
}

uint16_t* UltraSoundDrv_ADC_Read(ADC_TypeDef* ADC) {
    return ADC_ReadSingleSequence(ADC);
}

uint8_t get_msg() {
    return get_adc_msg();
}