#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "SensEdu.h"

typedef enum {
    TIMER_ERROR_NO_ERRORS = 0x00,
    TIMER_ERROR_TIM4_BAD_SET_FREQUENCY = 0x01
} TIMER_ERROR;

void SensEdu_TIMER_DelayInit(void);
void SensEdu_TIMER_Delay_us(uint32_t delay_us);

TIMER_ERROR TIMER_GetError(void);
void TIMER_ADC1Init(void);
void TIMER_DAC1Init(uint32_t freq);
void TIMER_ADC1Enable(void);
void TIMER_DAC1Enable(void);
void TIMER_ADC1Disable(void);
void TIMER_DAC1Disable(void);
void TIMER_ADC1SetFreq(uint32_t freq);
void TIMER_DAC1SetFreq(uint32_t freq);


#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__