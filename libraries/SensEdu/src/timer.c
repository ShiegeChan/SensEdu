#include "timer.h"

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

#define TIM_FREQ        240000000
#define DAC_PRESC_FREQ  120000000       // Desired freq after prescaler for DAC
#define ADC_PRESC_FREQ  60000000        // Desired freq after prescaler for ADC


static TIMER_ERROR error = TIMER_ERROR_NO_ERRORS;
static volatile uint8_t delay_flag = 0;


/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
void tim1_adc1_init(void);
void tim2_delay_init(void);
void tim4_dac1_init(void);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

void SensEdu_TIMER_DelayInit(void) {
    tim2_delay_init();
}

void SensEdu_TIMER_Delay_us(uint32_t delay_us) {
    delay_flag = 1;
    WRITE_REG(TIM2->ARR, delay_us);
    WRITE_REG(TIM2->CNT, 0U);
    SET_BIT(TIM2->CR1, TIM_CR1_CEN);
    while(delay_flag == 1);
}

TIMER_ERROR TIMER_GetError(void) {
    return error;
}

void TIMER_ADC1Init(void) {
    tim1_adc1_init();
}

void TIMER_DAC1Init(uint32_t freq) {
    tim4_dac1_init();
    TIMER_DAC1SetFreq(freq);
}

void TIMER_ADC1Enable(void) {
    WRITE_REG(TIM1->CNT, 0U);
    SET_BIT(TIM1->CR1, TIM_CR1_CEN);
}

void TIMER_DAC1Enable(void) {
    WRITE_REG(TIM4->CNT, 0U);
    SET_BIT(TIM4->CR1, TIM_CR1_CEN);
}

void TIMER_ADC1Disable(void) {
    CLEAR_BIT(TIM1->CR1, TIM_CR1_CEN);
}

void TIMER_DAC1Disable(void) {
    CLEAR_BIT(TIM4->CR1, TIM_CR1_CEN);
}

void TIMER_ADC1SetFreq(uint32_t freq) {
    if (freq < 0 || freq > (TIM_FREQ/2)) {
        //error = TIMER_ERROR_TIM1_BAD_SET_FREQUENCY;
        return;
    }
    float periodf = (float)ADC_PRESC_FREQ/freq;
    uint32_t period = (uint32_t)lroundf(periodf); // period = ARR + 1
    uint32_t *psc, *arr;
    calculate_timer_values(freq, *psc, *arr);
    WRITE_REG(TIM1->PSC, &psc);
    WRITE_REG(TIM1->ARR, &arr);
}

void TIMER_DAC1SetFreq(uint32_t freq) {
    if (freq < 0 || freq > (DAC_PRESC_FREQ/2)) {
        // minimum ARR is 1
        error = TIMER_ERROR_TIM4_BAD_SET_FREQUENCY;
        return;
    }
    float periodf = (float)DAC_PRESC_FREQ/freq;
    uint32_t period = (uint32_t)lroundf(periodf); // period = ARR + 1
    WRITE_REG(TIM4->ARR, period - 1U);
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */


float calculate_freq_error(uint32_t PSC, uint32_t ARR, uint32_t target_freq) {
    float calculated_freq = TIM_FREQ / ((PSC + 1) * (ARR + 1));
    return (target_freq - calculated_freq);
}

void calculate_timer_values(uint32_t freq, uint32_t *PSC, uint32_t *ARR) {
    // uint32_t PSC_MAX = 65535; // 2^16-1
    // uint32_t ARR_MAX = 65535; 
    float threshold = 1;
    if(freq <= 9999) {
        threshold = 50.0;
    }
    else if (freq <= 99999) {
        threshold = 500.0;
    }
    else if (freq <= 999999) {
        threshold = 50000.0;
    }
    else if (freq <= 9999999) {
        threshold = 500000.0;
    }
    else if (freq <= 99999999) {
        threshold = 5000000.0;
    }
    else if (freq <= 999999999) {
        threshold = 50000000.0;
    }
    else {
        threshold = 100;
    }

    for (uint32_t i = 1; i < 65535; i++) {
        for(int32_t j = 1; j < 65535; j++) {
            if(calculate_freq_error(i, j, freq) < threshold) {
                PSC = i;
                ARR = j;
                break;
            }
        }
    }
}

void tim1_adc1_init(void) {
    // Clock
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);

    // Frequency settings
    WRITE_REG(TIM1->PSC, 2U - 1U); // default
    WRITE_REG(TIM1->ARR, 60U - 1U); // default

    // update event is trigger output
    MODIFY_REG(TIM1->CR2, TIM_CR2_MMS, 0b010 << TIM_CR2_MMS_Pos);
}

void tim2_delay_init() {
    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_TIM2EN);

    // Frequency settings
    WRITE_REG(TIM2->PSC, 240U-1U); // timer clock = 240MHz

    // timer turns off after one cycle
    SET_BIT(TIM2->CR1, TIM_CR1_OPM);

    // interrupts
    SET_BIT(TIM2->DIER, TIM_DIER_UIE); // update event
    NVIC_SetPriority(TIM2_IRQn, 4);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void tim4_dac1_init() {
    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_TIM4EN);

    // Frequency settings
    WRITE_REG(TIM4->PSC, (TIM_FREQ/DAC_PRESC_FREQ) - 1U); // default
    WRITE_REG(TIM4->ARR, 120U - 1U); // default

    // update event is trigger output
    MODIFY_REG(TIM4->CR2, TIM_CR2_MMS, 0b010 << TIM_CR2_MMS_Pos);
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */

void TIM2_IRQHandler(void) { 
    if (READ_BIT(TIM2->SR, TIM_SR_UIF)) {
        CLEAR_BIT(TIM2->SR, TIM_SR_UIF);
        delay_flag = 0;
    }
}