#include "SensEdu.h"

uint32_t lib_error = 0;
uint32_t cntr = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 1;
uint8_t adc_pins[adc_pin_num] = {A0};

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t memory4adc_size = 128;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc[memory4adc_size];

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {

    // doesn't boot without opened serial monitor
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }
    Serial.println("Started Initialization...");

    SensEdu_Init(adc, adc_pins, adc_pin_num, SENSEDU_ADC_MODE_CONT, 1000, SENSEDU_ADC_DMA_CONNECT); // continuos mode for ADC
    DMA_ADC1Init((uint16_t*)memory4adc, memory4adc_size); // it shouldn't be here, instead in SensEdu
    SensEdu_ADC_Enable(adc);

    SensEdu_ADC_Start(adc);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    // CPU does something
    cntr += 1;
    Serial.println(cntr);
    
    // DMA in background

    // Print transfered Data if available
    if (SensEdu_DMA_GetADC1TransferStatus()) {
        Serial.println("------");
        for (int i = 0; i < memory4adc_size; i++) {
            Serial.print("ADC value ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(memory4adc[i]);
        };

        // restart ADC
        SensEdu_DMA_ClearADC1TransferStatus();
        SensEdu_ADC_Start(adc);
    }

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
