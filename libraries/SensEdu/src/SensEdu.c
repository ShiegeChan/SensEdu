#include "SensEdu.h"

// RCC is configured by arduino by default with SYSCLK = 480MHz, HCLK = 240MHz, PCLK1/PCLK2 = 120MHz

static volatile SENSEDU_ERROR error = SENSEDU_NO_ERRORS;

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

    error |= DAC_GetError();
    if (error) {
        error |= SENSEDU_ERROR_DAC;
        return error;
    }

    error |= DMA_GetError();
    if (error) {
        error |= SENSEDU_ERROR_DMA;
        return error;
    }

    return error;
}