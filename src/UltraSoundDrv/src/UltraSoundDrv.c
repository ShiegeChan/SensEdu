#include "UltraSoundDrv.h"
#include "timer.h"
#include "adc.h"
#include "dma.h"


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

void check_board();

/* General*/
void UltraSoundDrv_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, ULTRASOUND_DRV_ADC_MODE mode, uint32_t trigger_freq) {
    check_board();
    UltraSoundDrv_TIMER_Init();
    UltraSoundDrv_ADC_Init(ADC, adc_pins, adc_pin_num, mode, trigger_freq);
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

    error |= DMA_GetError();
    if (error) {
        error |= ULTRASOUND_DRV_ERROR_DMA;
        return error;
    }

    return error;
}

/* Timer */
void UltraSoundDrv_TIMER_Init(void) {
    check_board();
    TIMER_Init();
}

void UltraSoundDrv_Delay_us(uint32_t delay_value) {
    TIMER_Delay_us(delay_value);
}

/* ADC */
void UltraSoundDrv_ADC_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, ULTRASOUND_DRV_ADC_MODE mode, uint32_t trigger_freq) {
    check_board();

    ADC_InitPeriph(ADC, adc_pins, adc_pin_num, mode);
    if (ADC_GetSettings(ADC)->mode == ULTRASOUND_DRV_ADC_MODE_CONT_TIM_TRIGGERED) {
        TIMER_ADCtrigger_SetFreq(trigger_freq);
    }
}

void UltraSoundDrv_ADC_Enable(ADC_TypeDef* ADC) {
    if (ADC_GetSettings(ADC)->mode == ULTRASOUND_DRV_ADC_MODE_CONT_TIM_TRIGGERED) {
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

/* DMA */
void UltraSoundDrv_DMA_Init(uint16_t* memory0_address) {
    DMA_InitPeriph(memory0_address);
}

void UltraSoundDrv_DMA_Enable(uint16_t* mem_address, const uint16_t mem_size) {
    DMA_EnablePeriph(mem_address, mem_size);
}

uint8_t UltraSoundDrv_DMA_GetTransferStatus(void) {
    return DMA_GetTransferStatus();
}
void UltraSoundDrv_DMA_ClearTransferStatus() {
    DMA_ClearTransferStatus();
}

/* local */
void check_board() {
    #ifndef ARDUINO_GIGA
        #error Only Arduino Giga R1 is supported
    #endif
}
