#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "SensEdu.h"

typedef enum {
    TIMER_ERROR_NO_ERRORS = 0x00,
    TIMER_ERROR_TIM3_BAD_SET_FREQUENCY = 0x01
} TIMER_ERROR;

TIMER_ERROR TIMER_GetError(void);
void TIMER_Init(void);
void TIMER_Delay_us(uint32_t delay_value);
void TIMER_ADCtrigger_SetFreq(uint32_t freq);
void TIMER_ADCtrigger_Enable(void);
void TIMER_DACtrigger_Enable(void);
void TIMER_DACtrigger_Disable(void);
void TIMER_DACtrigger_SetFreq(uint32_t freq);


#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__