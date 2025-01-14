#include "STMSpeeduino.h"
#include "src/UltraSoundDrv/src/UltraSoundDrv.h"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_adc.h"
#include "stm32h747xx.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_rcc.h"

const uint16_t memory4adc_size = 10;
uint16_t memory4adc[memory4adc_size];

uint8_t pins[1] = {A0};
int counter = 0;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {

    // doesn't boot without opened serial monitor
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }

    UltraSoundDrv_Init(ADC1, pins, 1, ULTRASOUND_DRV_ADC_MODE_CONT, 1000);
    UltraSoundDrv_ADC_Enable(ADC1);

    SCB_InvalidateDCache_by_Addr((uint16_t*)memory4adc, memory4adc_size * 2);
    //AttachADC_DMA(ADC1DMA, memory4adc_size, (uint16_t *)memory4adc, DMAS4);
    UltraSoundDrv_DMA_Init((uint16_t*)memory4adc);
    UltraSoundDrv_DMA_Enable();
    Serial.println("Setup is successful.");

    // start ADC conversions
    UltraSoundDrv_ADC_Start(ADC1);
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    // CPU does something

    // DMA in parallel
    //if (UltraSoundDrv_DMA_GetTransferStatus()) {
    if (READ_BIT(DMA1->HISR, DMA_HISR_TCIF4)) {
        Serial.println("we are in");
        //set_statuss(false);
        // enter transfer complete interrupt flag
        //UltraSoundDrv_DMA_SetTransferStatus(0);
        for (int i = 0; i < memory4adc_size; i++) {
            Serial.print("ADC1 value ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(memory4adc[i]);
        };

        //recaptureADCvalues(ADC1DMA);

        UltraSoundDrv_ADC_Start(ADC1);
        UltraSoundDrv_DMA_Enable();
        SCB_InvalidateDCache_by_Addr((uint16_t *)memory4adc, memory4adc_size * 2);
    }


    
    static uint32_t lib_error = UltraSoundDrv_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    //ADC_Start(ADC1);
    //SET_BIT(DMA1->HIFCR, ADC_DMA.AttachedStream.DMA_CHT | ADC_DMA.AttachedStream.DMA_CTC);  //Clear the transfer flags

    //ADC_DMA.AttachedStream.DMASt->CR |= 1 << 0;  //Enable the DMA stream
    //SCB_InvalidateDCache_by_Addr(ADC_DMA.BufferAddress, ADC_DMA.StreamSize * 2);
}
