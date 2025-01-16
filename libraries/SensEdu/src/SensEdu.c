#include "SensEdu.h"
#include "timer.h"
#include "adc.h"
#include "dma.h"

// RCC is configured by arduino by default with SYSCLK = 480MHz, HCLK = 240MHz, PCLK1/PCLK2 = 120MHz

void check_board();

/* General*/
void SensEdu_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, SENSEDU_ADC_MODE mode, uint32_t trigger_freq) {
    check_board();
    SensEdu_TIMER_Init();
    SensEdu_ADC_Init(ADC, adc_pins, adc_pin_num, mode, trigger_freq);
}

SENSEDU_ERROR SensEdu_GetError(void) {
    SENSEDU_ERROR error = SENSEDU_NO_ERRORS;

    error |= TIMER_GetError();
    if (error) {
        error |= SENSEDU_ERROR_TIMER;
        return error;
    }

    error |= ADC_GetError();
    if (error) {
        error |= SENSEDU_ERROR_ADC;
        return error;
    }

    error |= DMA_GetError();
    if (error) {
        error |= SENSEDU_ERROR_DMA;
        return error;
    }

    return error;
}

/* Timer */
void SensEdu_TIMER_Init(void) {
    check_board();
    TIMER_Init();
}

void SensEdu_Delay_us(uint32_t delay_value) {
    TIMER_Delay_us(delay_value);
}

/* ADC */
void SensEdu_ADC_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, SENSEDU_ADC_MODE mode, uint32_t trigger_freq) {
    check_board();

    ADC_InitPeriph(ADC, adc_pins, adc_pin_num, mode);
    if (ADC_GetSettings(ADC)->mode == SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED) {
        TIMER_ADCtrigger_SetFreq(trigger_freq);
    }
}

void SensEdu_ADC_Enable(ADC_TypeDef* ADC) {
    if (ADC_GetSettings(ADC)->mode == SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED) {
        TIMER_ADCtrigger_Enable();
    }

    ADC_EnablePeriph(ADC);
}

void SensEdu_ADC_Disable(ADC_TypeDef* ADC) {
    ADC_DisablePeriph(ADC);
}

void SensEdu_ADC_Start(ADC_TypeDef* ADC) {
    ADC_StartConversion(ADC);
}

uint16_t* SensEdu_ADC_Read(ADC_TypeDef* ADC) {
    return ADC_ReadSingleSequence(ADC);
}

uint8_t get_msg() {
    return get_adc_msg();
}

/* DMA */
void SensEdu_DMA_Init(uint16_t* memory0_address) {
    DMA_InitPeriph(memory0_address);
}

void SensEdu_DMA_Enable(uint16_t* mem_address, const uint16_t mem_size) {
    DMA_EnablePeriph(mem_address, mem_size);
}

uint8_t SensEdu_DMA_GetTransferStatus(void) {
    return DMA_GetTransferStatus();
}
void SensEdu_DMA_ClearTransferStatus() {
    DMA_ClearTransferStatus();
}

/* local */
void check_board() {
    #ifndef ARDUINO_GIGA
        #error Only Arduino Giga R1 is supported
    #endif
}
