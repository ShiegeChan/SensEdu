#include "timer.h"

static const uint32_t freq_reference = 10000000;
static const uint32_t allowed_freqs[] = {10000000, 5000000, 2500000, 2000000, 1250000, 1000000, 625000, 500000,
    312500, 250000, 156250, 125000, 80000, 78125, 62500, 40000, 31250, 20000, 16000, 15625, 
    10000, 8000, 5000, 4000, 3200, 2500, 2000, 1600, 1250, 1000};
static uint32_t sampling_rate = 0;

static TIMER_ERROR error = TIMER_ERROR_NO_ERRORS;
static volatile uint8_t delay_flag = 0;



TIMER_ERROR TIMER_GetError(void) {
    return error;
}

void TIMER_Init(uint32_t trigger_freq) {
    tim2_init();
    tim1_init(trigger_freq);
}

// check if it works as intended with oscilloscope
void TIMER_Delay_us(uint32_t delay_value) {
    delay_flag = 1;

    WRITE_REG(TIM2->CNT, 0U);
    WRITE_REG(TIM2->ARR, delay_value);
    SET_BIT(TIM2->CR1, TIM_CR1_CEN);
    while(delay_flag == 1);
}

void TIMER_ADCtrigger_Enable(void) {
    SET_BIT(TIM1->CR1, TIM_CR1_CEN);
}

// check if it works as intended with oscilloscope
void tim1_init(uint32_t freq) {
    // Enable clock on TIM1 from APB2
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);

    // frequency settings
    WRITE_REG(TIM1->PSC, 12U-1U); // 0.1us step (10MHz as reference)

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

    // update event is trigger output
    MODIFY_REG(TIM1->CR2, TIM_CR2_MMS, 0b010 << TIM_CR2_MMS_Pos);
}

void tim2_init() {
    // Enable clock on TIM2 from APB1
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_TIM2EN);

    // prescaler
    WRITE_REG(TIM2->PSC, 120U-1U); // default arduino PCLK1/PCLK2 = 120MHz

    // timer turns off after one cycle
    SET_BIT(TIM2->CR1, TIM_CR1_OPM);

    // interrupts
    SET_BIT(TIM2->DIER, TIM_DIER_UIE); // update event
    NVIC_SetPriority(TIM2_IRQn, 2);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void) { 
    //Check what type of event occurred
    if (READ_BIT(TIM2->SR, TIM_SR_UIF))
    {
        CLEAR_BIT(TIM2->SR, TIM_SR_UIF);
        delay_flag = 0;
    }
}