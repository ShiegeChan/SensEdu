#ifndef __SENSEDU_H__
#define __SENSEDU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libs.h"
#include "adc.h"
#include "dac.h"
#include "timer.h"
#include "dma.h"

typedef enum {
    SENSEDU_NO_ERRORS = 0x0000,
    SENSEDU_ERROR_TIMER = 0x1000,
    SENSEDU_ERROR_ADC = 0x2000,
    SENSEDU_ERROR_DMA = 0x3000,
    SENSEDU_ERROR_DAC = 0x4000
} SENSEDU_ERROR;

SENSEDU_ERROR SensEdu_GetError(void);


#ifdef __cplusplus
}
#endif

#endif // __SENSEDU_H__