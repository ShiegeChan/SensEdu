#ifndef __TIMER_H__
#define __TIMER_H__


#include "SensEdu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TIMER_ERROR_NO_ERRORS = 0x00,
    TIMER_ERROR_BAD_SET_DELAY = 0x01,
    TIMER_ERROR_PICKED_WRONG_ADC = 0x02,
    TIMER_ERROR_ADC_TIM_BAD_SET_FREQUENCY = 0x03,
    TIMER_ERROR_TIM4_BAD_SET_FREQUENCY = 0x04,
    TIMER_ERROR_TIM8_INIT_WHILE_RUNNING = 0x05,
    TIMER_ERROR_TIM8_WRONG_DUTY_CHANNEL = 0x06,
    TIMER_ERROR_TIM3_BAD_SET_FREQUENCY = 0x07,
    TIMER_ERROR_TIM6_BAD_SET_FREQUENCY = 0x08,

    TIMER_ERROR_CRITICAL_FREQ_CALCULATION_BUG = 0xA0
} TIMER_ERROR;

void SensEdu_TIMER_DelayInit(void);
void SensEdu_TIMER_Delay_us(uint32_t delay_us);
void SensEdu_TIMER_Delay_ns(uint32_t delay_ns);

TIMER_ERROR TIMER_GetError(void);

void TIMER_ADC1Init(void);
void TIMER_DAC1Init(uint32_t freq);
void TIMER_ADCxEnable(ADC_TypeDef* adc);
void TIMER_DAC1Enable(void);
void TIMER_ADCxDisable(ADC_TypeDef* adc);
void TIMER_DAC1Disable(void);
void TIMER_ADCSetFreq(ADC_TypeDef* adc, uint32_t freq);
void TIMER_DAC1SetFreq(uint32_t freq);

void TIMER_PWMInit(void);
void TIMER_PWMEnable(void);
void TIMER_PWMDisable(void);
void TIMER_PWMSetFreq(uint32_t freq);
void TIMER_PWMSetDutyCycle(uint8_t channel, uint8_t duty_cycle);


#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
