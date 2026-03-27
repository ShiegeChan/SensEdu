/**
 * @file timer.c
 * @brief Internal implementation of timer driver.
 *
 * This module provides:
 * - Blocking delays with microsecond and nanosecond resolution
 * - Timer configuration for ADC triggering
 * - Timer configuration for DAC triggering
 * - PWM generation
 *
 * Notes:
 * - Delay functions are blocking (CPU busy-wait)
 * - Nanosecond delays are limited by software overhead (~550 ns minimum)
 * - Timer clock is assumed to be 240 MHz
 */

#include "timer.h"

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

// APB1 and APB2 timer clocks frequency
static const uint32_t TIM_CLK = 240000000UL;

// Precalculated prescaler to achieve 1MHz (1us) frequency
#define PSC_1US (TIM_CLK / 1000000UL)

// Precalculated CPU overhead for one function call like LED blinking
// Used for basic compensation in microsecond delays
#define DELAY_CPU_OVERHEAD_NS (550U)

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

static TIMER_ERROR error = TIMER_ERROR_NO_ERRORS;
static volatile uint8_t delay_done = 0;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static void tim_calculate_freq_settings_16bit(uint32_t freq, uint16_t *PSC, uint16_t *ARR);
static void tim_calculate_freq_settings_32bit(uint32_t freq, uint16_t *PSC, uint32_t *ARR);
static inline uint32_t tim_ns_to_ticks(uint32_t ns);
static void tim_init_adc(TIM_TypeDef* tim);
static void tim2_init_delay(void);
static void tim4_init_dac1(void);
static void tim8_init_pwm(void);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Initializes timer responsible for generic delays.
void SensEdu_TIMER_DelayInit(void) {
    tim2_init_delay();
}

// Triggers a blocking delay for specified time in microseconds.
// CPU is stalled until completion or timeout.
void SensEdu_TIMER_Delay_us(uint32_t delay_us) {
    if (delay_us <= 1000U) {
        SensEdu_TIMER_Delay_ns(delay_us * 1000U);
        return;
    }
    delay_done = 1;
    WRITE_REG(TIM2->PSC, PSC_1US - 1U);
    WRITE_REG(TIM2->ARR, delay_us - 1U);
    WRITE_REG(TIM2->CNT, 0U);
    SET_BIT(TIM2->CR1, TIM_CR1_CEN);

    uint32_t timeout = UINT32_MAX;
    while (delay_done && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        error = TIMER_ERROR_TIMEOUT;
        return;
    }
}

// Triggers a blocking delay for specified time in nanoseconds.
// CPU is stalled until completion or timeout.
//
// Note: Due to software overhead, delays below ~550ns are not possible.
void SensEdu_TIMER_Delay_ns(uint32_t delay_ns) {
    if (delay_ns < 1U) {
        error = TIMER_ERROR_BAD_SET_DELAY;
        return;
    }
    if (delay_ns > 1000000UL) {
        // avoid ticks macro overflow for delays > 1ms
        SensEdu_TIMER_Delay_us(delay_ns / 1000U);
        return;
    }

    // Compensate for software overhead (~550ns) to improve delay accuracy
    if (delay_ns > DELAY_CPU_OVERHEAD_NS) {
        delay_ns -= DELAY_CPU_OVERHEAD_NS;
    } else {
        delay_ns = 1;
    }

    delay_done = 1;
    WRITE_REG(TIM2->PSC, 0U); // Set finest resolution
    uint32_t arr = tim_ns_to_ticks(delay_ns); // Timer tick resolution is ~4.17ns
    if (arr < 2U) {
        arr = 2U;
    }
    WRITE_REG(TIM2->ARR, arr - 1U); // minimum ARR is 1
    WRITE_REG(TIM2->CNT, 0U);
    SET_BIT(TIM2->CR1, TIM_CR1_CEN);

    uint32_t timeout = UINT32_MAX;
    while (delay_done && timeout--) {
        __NOP();
    }
    if (timeout == 0) {
        error = TIMER_ERROR_TIMEOUT;
        return;
    }
}

// Returns TIMER driver's current error state.
TIMER_ERROR TIMER_GetError(void) {
    return error;
}

// Returns a predefined timer for selected ADC.
TIM_TypeDef* TIMER_GetTimerForAdc(ADC_TypeDef* adc) {
    if (adc == ADC1) return TIM1;
    if (adc == ADC2) return TIM3;
    if (adc == ADC3) return TIM6;
    error = TIMER_ERROR_PICKED_WRONG_ADC;
    return NULL;
}

// Initializes predefined timer for selected ADC.
void TIMER_ADCxInit(ADC_TypeDef* adc) {
    TIM_TypeDef* tim = TIMER_GetTimerForAdc(adc);
    if (tim == NULL) {
        error = TIMER_ERROR_PICKED_WRONG_ADC;
        return;
    }
    tim_init_adc(tim);
}

// Enables predefined timer for selected ADC.
void TIMER_ADCxEnable(ADC_TypeDef* adc) {
    TIM_TypeDef* tim = TIMER_GetTimerForAdc(adc);
    if (tim == NULL) {
        return;
    }
    WRITE_REG(tim->CNT, 0U);
    SET_BIT(tim->CR1, TIM_CR1_CEN);
}

// Disables predefined timer for selected ADC.
void TIMER_ADCxDisable(ADC_TypeDef* adc) {
    TIM_TypeDef* tim = TIMER_GetTimerForAdc(adc);
    if (tim == NULL) {
        return;
    }
    CLEAR_BIT(tim->CR1, TIM_CR1_CEN);
}

// Sets the frequency (Hz) of the predefined timer for selected ADC.
void TIMER_ADCxSetFreq(ADC_TypeDef* adc, uint32_t freq) {
    TIM_TypeDef* tim = TIMER_GetTimerForAdc(adc);
    if (tim == NULL) {
        return;
    }
    if (freq > (TIM_CLK/2)) {
        error = TIMER_ERROR_ADC_BAD_SET_FREQUENCY;
        return;
    }
    uint32_t psc, arr;
    tim_calculate_freq_settings_16bit(freq, &psc, &arr);
    WRITE_REG(tim->PSC, psc);
    WRITE_REG(tim->ARR, arr);
}

// Initializes predefined timer for DAC.
void TIMER_DAC1Init(uint32_t freq) {
    tim4_init_dac1();
    TIMER_DAC1SetFreq(freq);
}

// Enables predefined timer for DAC.
void TIMER_DAC1Enable(void) {
    WRITE_REG(TIM4->CNT, 0U);
    SET_BIT(TIM4->CR1, TIM_CR1_CEN);
}

// Disables predefined timer for DAC.
void TIMER_DAC1Disable(void) {
    CLEAR_BIT(TIM4->CR1, TIM_CR1_CEN);
}

// Sets the frequency (Hz) of the predefined timer for the DAC.
void TIMER_DAC1SetFreq(uint32_t freq) {
    if (freq > (TIM_CLK/2)) {
        error = TIMER_ERROR_DAC_BAD_SET_FREQUENCY;
        return;
    }

    uint16_t psc, arr;
    tim_calculate_freq_settings_16bit(freq, &psc, &arr);
    WRITE_REG(TIM4->PSC, psc);
    WRITE_REG(TIM4->ARR, arr);
}

// Initializes predefined timer for PWM.
void TIMER_PWMInit(void) {
    tim8_init_pwm();
}

// Enables predefined timer for PWM.
void TIMER_PWMEnable(void) {
    SET_BIT(TIM8->EGR, TIM_EGR_UG);
    SET_BIT(TIM8->CR1, TIM_CR1_CEN);
}

// Disables predefined timer for PWM.
void TIMER_PWMDisable(void) {
    CLEAR_BIT(TIM8->CR1, TIM_CR1_CEN);
    SET_BIT(TIM8->EGR, TIM_EGR_UG);
}

// Sets the frequency (Hz) of the predefined timer for the PWM.
void TIMER_PWMSetFreq(uint32_t freq) {
    uint16_t psc, arr;
    tim_calculate_freq_settings_16bit(freq, &psc, &arr);

    WRITE_REG(TIM8->PSC, psc);
    WRITE_REG(TIM8->ARR, arr);
}

// Sets the duty cycle (%) of the predefined timer for the PWM.
void TIMER_PWMSetDutyCycle(uint8_t channel, uint8_t duty_cycle) {

    if (duty_cycle > 100) {
        error = TIMER_ERROR_BAD_SET_DUTY_CYCLE;
        return;
    }

    uint32_t arr = READ_REG(TIM8->ARR) + 1;
    uint32_t ccr = (arr*(100 - duty_cycle))/100;
    switch(channel) {
        case 1:
            WRITE_REG(TIM8->CCR1, ccr);
            break;
        case 2:
            WRITE_REG(TIM8->CCR2, ccr);
            break;
        case 3:
            WRITE_REG(TIM8->CCR3, ccr);
            break;
        case 4:
            WRITE_REG(TIM8->CCR4, ccr);
            break;
        default:
            error = TIMER_ERROR_TIM8_WRONG_DUTY_CHANNEL;
            break;
    }
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

static void tim_calculate_freq_settings_16bit(uint32_t freq, uint16_t *PSC, uint16_t *ARR) {
    uint32_t arr = 0;
    uint16_t psc = 0;

    if (freq == 0U) {
        error = TIMER_ERROR_BAD_SET_FREQUENCY;
        return;
    }

    // try PSC = 0
    arr = TIM_CLK/freq - 1;
    if (arr > 0 && arr <= UINT16_MAX) {
        *PSC = 0U;
        *ARR = (uint16_t)arr;
        return;
    }
    if (arr < 1) {
        error = TIMER_ERROR_CRITICAL_FREQ_CALCULATION_BUG;
        return;
    }

    // compute minimal PSC
    psc = TIM_CLK/(freq*(UINT16_MAX + 1)) - 1;
    if (psc > UINT16_MAX) {
        error = TIMER_ERROR_CRITICAL_FREQ_CALCULATION_BUG;
        return;
    }
    arr = TIM_CLK/(freq * (psc + 1)) - 1;
    if (arr > UINT16_MAX) {
        psc += 1; // fix for rounding errors
        arr = TIM_CLK/(freq * (psc + 1)) - 1;
        if (arr > UINT16_MAX) {
            error = TIMER_ERROR_CRITICAL_FREQ_CALCULATION_BUG;
            return;
        }
    }
    *PSC = psc;
    *ARR = (uint16_t)arr;
}

static void tim_calculate_freq_settings_32bit(uint32_t freq, uint16_t *PSC, uint32_t *ARR) {
    uint32_t arr = 0;
    uint16_t psc = 0;

    if (freq == 0U) {
        error = TIMER_ERROR_BAD_SET_FREQUENCY;
        return;
    }

    // PSC = 0 will always succeed,
    // since 32bit ARR with 240MHz TIM_CLK allows up to ~0.06Hz frequencies
    arr = TIM_CLK/freq - 1;
    if (arr > 0) {
        *PSC = 0U;
        *ARR = arr;
        return;
    }
    if (arr < 1) {
        error = TIMER_ERROR_CRITICAL_FREQ_CALCULATION_BUG;
        return;
    }
}

// Convert nanoseconds to ticks due to step 1ns not being possible.
// Finest resolution is ~4.17ns, which tick is a multiple of.
static inline uint32_t tim_ns_to_ticks(uint32_t ns) {
    return ((ns * PSC_1US) / 1000);
}

static void tim_init_adc(TIM_TypeDef* tim) {
    if (tim == TIM1) {
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);
    } else if (tim == TIM3) {
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_TIM3EN);
    } else if (tim == TIM6) {
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_TIM6EN);
    } else {
        error = TIMER_ERROR_UNDEFINED_TIMER_REQUESTED;
        return;
    }
    // Frequency settings (default)
    WRITE_REG(tim->PSC, 1U - 1U);
    WRITE_REG(tim->ARR, 120U - 1U);

    // Update event is trigger output
    MODIFY_REG(tim->CR2, TIM_CR2_MMS, 0b010 << TIM_CR2_MMS_Pos);
}

static void tim2_init_delay(void) {
    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_TIM2EN);

    // Frequency settings (1 us resolution)
    WRITE_REG(TIM2->PSC, 240U-1U);

    // Enable one-pulse mode
    // Timer stops automatically after update event
    SET_BIT(TIM2->CR1, TIM_CR1_OPM);

    // Interrupts
    SET_BIT(TIM2->DIER, TIM_DIER_UIE); // update event
    NVIC_SetPriority(TIM2_IRQn, 2);
    NVIC_EnableIRQ(TIM2_IRQn);
}

static void tim4_init_dac1(void) {
    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_TIM4EN);

    // Frequency settings (default)
    WRITE_REG(TIM4->PSC, 1U - 1U);
    WRITE_REG(TIM4->ARR, 120U - 1U);

    // Use update event as trigger output
    MODIFY_REG(TIM4->CR2, TIM_CR2_MMS, 0b010 << TIM_CR2_MMS_Pos);
}

static void tim8_init_pwm(void) {
    if (READ_BIT(TIM8->CR1, TIM_CR1_CEN)) {
        error = TIMER_ERROR_TIM8_INIT_WHILE_RUNNING;
        return;
    }

    // Clock
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM8EN);

    // Select PWM mode
    MODIFY_REG(TIM8->CCMR1, TIM_CCMR1_OC1M, 0b0111 << TIM_CCMR1_OC1M_Pos);
    MODIFY_REG(TIM8->CCMR1, TIM_CCMR1_OC2M, 0b0111 << TIM_CCMR1_OC2M_Pos);
    MODIFY_REG(TIM8->CCMR2, TIM_CCMR2_OC3M, 0b0111 << TIM_CCMR2_OC3M_Pos);
    MODIFY_REG(TIM8->CCMR2, TIM_CCMR2_OC4M, 0b0111 << TIM_CCMR2_OC4M_Pos);

    // Preload updates
    SET_BIT(TIM8->CCMR1, TIM_CCMR1_OC1PE);
    SET_BIT(TIM8->CCMR1, TIM_CCMR1_OC2PE);
    SET_BIT(TIM8->CCMR2, TIM_CCMR2_OC3PE);
    SET_BIT(TIM8->CCMR2, TIM_CCMR2_OC4PE);
    SET_BIT(TIM8->CR1, TIM_CR1_ARPE);

    // Default frequency settings
    WRITE_REG(TIM8->PSC, 1U - 1U);
    WRITE_REG(TIM8->ARR, 1000U - 1U);

    // Default duty cycle settings
    WRITE_REG(TIM8->CCR1, 1000U - 1U);
    WRITE_REG(TIM8->CCR2, 750U - 1U);
    WRITE_REG(TIM8->CCR3, 500U - 1U);
    WRITE_REG(TIM8->CCR4, 250U - 1U);

    // Clear Counter
    SET_BIT(TIM8->EGR, TIM_EGR_UG);
    WRITE_REG(TIM8->CNT, 0U);

    // Enable Capture/Compare
    SET_BIT(TIM8->CCER, TIM_CCER_CC1E);
    SET_BIT(TIM8->CCER, TIM_CCER_CC2E);
    SET_BIT(TIM8->CCER, TIM_CCER_CC3E);
    SET_BIT(TIM8->CCER, TIM_CCER_CC4E);

    // Main Output Enable
    SET_BIT(TIM8->BDTR, TIM_BDTR_MOE);
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */
void TIM2_IRQHandler(void) { 
    if (READ_BIT(TIM2->SR, TIM_SR_UIF)) {
        CLEAR_BIT(TIM2->SR, TIM_SR_UIF);
        delay_done = 0;
    }
}
