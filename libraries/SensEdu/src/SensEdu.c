#include "SensEdu.h"

// RCC is configured by arduino by default with SYSCLK = 480MHz, HCLK = 240MHz, PCLK1/PCLK2 = 120MHz

static volatile SENSEDU_ERROR error = SENSEDU_NO_ERRORS;

void check_board();

/* General*/
void SensEdu_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, SENSEDU_ADC_MODE mode, uint32_t trigger_freq, SENSEDU_ADC_DMA adc_dma) {
    check_board();
    SensEdu_TIMER_Init();
    SensEdu_ADC_Init(ADC, adc_pins, adc_pin_num, mode, trigger_freq, adc_dma);
}

SENSEDU_ERROR SensEdu_GetError(void) {
    
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

    error |= DAC_GetError();
    if (error) {
        error |= SENSEDU_ERROR_DAC;
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
void SensEdu_ADC_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, SENSEDU_ADC_MODE mode, uint32_t trigger_freq, SENSEDU_ADC_DMA adc_dma) {
    check_board();

    ADC_InitPeriph(ADC, adc_pins, adc_pin_num, mode, adc_dma);
    if (ADC_GetSettings(ADC)->mode == SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED) {
        TIMER_ADCtrigger_SetFreq(trigger_freq);
    }

    if (ADC_GetSettings(ADC)->dma_mode == SENSEDU_ADC_DMA_CONNECT && ADC == ADC1) {
        //DMA_ADC1Init(); TODO: fix it here with passing memory and size
    }
}

void SensEdu_ADC_Enable(ADC_TypeDef* ADC) {
    if (ADC_GetSettings(ADC)->mode == SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED) {
        TIMER_ADCtrigger_Enable();
    }

    ADC_EnablePeriph(ADC);
}

void SensEdu_ADC_Disable(ADC_TypeDef* ADC) {
    // TODO: disable here DMA
    ADC_DisablePeriph(ADC);
}

void SensEdu_ADC_Start(ADC_TypeDef* ADC) {
    if (ADC_GetSettings(ADC)->dma_mode == SENSEDU_ADC_DMA_CONNECT && ADC == ADC1) {
        DMA_ADC1Enable();
    }
    ADC_StartConversion(ADC);
}

uint16_t* SensEdu_ADC_Read(ADC_TypeDef* ADC) {
    return ADC_ReadSingleSequence(ADC);
}

uint8_t get_msg() {
    return get_adc_msg();
}

/* DMA */
void SensEdu_DMA_Init(uint16_t* mem_address, const uint16_t mem_size) {
    DMA_ADCInitPeriph(mem_address, mem_size);
}

void SensEdu_DMA_Enable(uint16_t* mem_address, const uint16_t mem_size) {
    DMA_ADCEnablePeriph(mem_address, mem_size);
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
