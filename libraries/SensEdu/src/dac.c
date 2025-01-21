#include "dac.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static DAC_ERROR error = DAC_ERROR_NO_ERRORS;


/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void dac_init(void);


/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
DAC_ERROR DAC_GetError(void) {
    return error;
}

void DAC_InitPeriph(void) {
    /*
    if (ADC != ADC1 && ADC != ADC2) {
        error = ADC_ERROR_WRONG_ADC; // you can only use ADC1 or ADC2
        return;
    }
    */
}

void DAC_EnablePeriph(void) {

}

void DAC_DisablePeriph(void) {
    
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
void dac_init(void) {

}

void DAC_IRQHandler(void) {
    /*
    if (READ_BIT(ADC1->ISR, ADC_ISR_EOC)) {
        SET_BIT(ADC1->ISR, ADC_ISR_EOC);
        ADC1_Settings.eoc_flag = 1;
    }
    
    if (READ_BIT(ADC2->ISR, ADC_ISR_EOC)) {
        SET_BIT(ADC2->ISR, ADC_ISR_EOC);
        ADC2_Settings.eoc_flag = 1;
    }
    */
}
