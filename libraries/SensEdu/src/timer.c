#include "timer.h"

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

#define TIM_FREQ        240000000
#define DAC_PRESC_FREQ  120000000       // Desired freq after prescaler for DAC

static const uint32_t freq_reference = 10000000;
static const uint32_t allowed_freqs[] = {10000000, 5000000, 2500000, 2000000, 1250000, 1000000, 625000, 500000,
    312500, 250000, 156250, 125000, 80000, 78125, 62500, 40000, 31250, 20000, 16000, 15625, 
    10000, 8000, 5000, 4000, 3200, 2500, 2000, 1600, 1250, 1000};
static uint32_t sampling_rate = 0;

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
    int32_t freq_diff = 2147483647; // max int32 value
    uint8_t closest_freq_index = 0;
    for (uint8_t i = 0; i < sizeof(allowed_freqs)/sizeof(allowed_freqs[0]); i++) {
        int32_t temp_diff = freq - allowed_freqs[i];
        temp_diff = abs(temp_diff);
        if (temp_diff < freq_diff) {
            freq_diff = temp_diff;
            closest_freq_index = i;
        }
    }

    sampling_rate = allowed_freqs[closest_freq_index];
    WRITE_REG(TIM1->ARR, (freq_reference/sampling_rate));
}

void TIMER_DAC1SetFreq(uint32_t freq) {
    if (freq < 0 || freq > (DAC_PRESC_FREQ/2)) {
        // minimum ARR is 1
        error = TIMER_ERROR_TIM4_BAD_SET_FREQUENCY;
        return;
    }
    float periodf = (float)DAC_PRESC_FREQ/freq;
    uint32_t period = (uint32_t)lroundf(periodf);
    WRITE_REG(TIM4->ARR, period - 1U);
}


/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
void tim1_adc1_init(void) {
    // Clock
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);

    // Frequency settings
    WRITE_REG(TIM1->PSC, 24U-1U); // 0.1us step (10MHz as reference)
    TIMER_ADC1SetFreq(allowed_freqs[0]); // default max freq

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
    WRITE_REG(TIM4->PSC, (TIM_FREQ/DAC_PRESC_FREQ) - 1U);
    WRITE_REG(TIM4->ARR, 120U - 1U);

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