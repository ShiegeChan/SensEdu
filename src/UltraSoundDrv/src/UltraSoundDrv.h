#ifndef __ULTRASOUNDDRV_H__
#define __ULTRASOUNDDRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>

#include "stm32h747xx.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_pwr.h"

// redefine pure analog pins
#define A8 (86u)
#define A9 (87u)
#define A10 (88u)
#define A11 (89u)

typedef enum {
    ULTRASOUND_DRV_NO_ERRORS = 0x0000,
    ULTRASOUND_DRV_ERROR_TIMER = 0x1000,
    ULTRASOUND_DRV_ERROR_ADC = 0x2000,
    ULTRASOUND_DRV_ERROR_DMA = 0x3000
} ULTRASOUND_DRV_ERROR;


ULTRASOUND_DRV_ERROR UltraSoundDrv_GetError(void);
void UltraSoundDrv_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, uint8_t tim_trigger, uint32_t trigger_freq);

void UltraSoundDrv_TIMER_Init();
void UltraSoundDrv_Delay_us(uint32_t delay_value);

void UltraSoundDrv_ADC_Init(ADC_TypeDef* ADC, uint8_t* adc_pins, uint8_t adc_pin_num, uint8_t tim_trigger, uint32_t trigger_freq);
void UltraSoundDrv_ADC_Enable(ADC_TypeDef* ADC);
void UltraSoundDrv_ADC_Disable(ADC_TypeDef* ADC);
void UltraSoundDrv_ADC_Start(ADC_TypeDef* ADC);
uint16_t* UltraSoundDrv_ADC_Read(ADC_TypeDef* ADC);
uint8_t get_msg();

void UltraSoundDrv_DMA_Init(uint32_t* memory0_address);
void UltraSoundDrv_DMA_Enable(void);
int8_t UltraSoundDrv_DMA_GetTransferStatus(void);
void UltraSoundDrv_DMA_SetTransferStatus(uint8_t new_status);

#ifdef __cplusplus
}
#endif

#endif // __ULTRASOUNDDRV_H__