#ifndef __DAC_H__
#define __DAC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "SensEdu.h"

typedef enum {
    DAC_ERROR_NO_ERRORS = 0x00
} DAC_ERROR;

DAC_ERROR DAC_GetError(void);
void DAC_InitPeriph(void);
void DAC_EnablePeriph(void);
void DAC_DisablePeriph(void);

#ifdef __cplusplus
}
#endif

#endif // __DAC_H__