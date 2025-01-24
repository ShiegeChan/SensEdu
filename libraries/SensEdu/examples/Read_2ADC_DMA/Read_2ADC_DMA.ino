#include "SensEdu.h"

uint32_t lib_error = 0;
uint32_t cntr = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
ADC_TypeDef* adc1 = ADC1;
ADC_TypeDef* adc3 = ADC2;
const uint8_t adc_pin_num = 2;
uint8_t adc1_pins[adc_pin_num] = {A1, A6}; 
uint8_t adc3_pins[adc_pin_num] = {A5, A10}; 

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t memory4adc_size = 64 * adc_pin_num; // allocate chunks of (4 * 32) * number of channels
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc1[memory4adc_size];
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc3[memory4adc_size];

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

    /* ADC 1 */
    SensEdu_Init(adc1, adc1_pins, adc_pin_num, SENSEDU_ADC_MODE_CONT, 1000, SENSEDU_ADC_DMA_CONNECT); // continuos mode for ADC1
    SensEdu_ADC_Enable(adc1);

    SensEdu_DMA_Init((uint16_t*)memory4adc1, memory4adc_size); 
    SensEdu_DMA_Enable((uint16_t*)memory4adc1, memory4adc_size);

    SensEdu_ADC_Start(adc1);

    /* ADC 3 */
    SensEdu_Init(adc3, adc3_pins, adc_pin_num, SENSEDU_ADC_MODE_CONT, 1000, SENSEDU_ADC_DMA_CONNECT); // continuos mode for ADC1
    SensEdu_ADC_Enable(adc3);

    SensEdu_DMA_Init((uint16_t*)memory4adc3, memory4adc_size, ); 
    SensEdu_DMA_Enable((uint16_t*)memory4adc3, memory4adc_size);

    SensEdu_ADC_Start(adc3);

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
    Serial.print("Counter value: ");
    Serial.println(cntr);
    
    // DMA in background

    // Print transfered Data if available
    if (SensEdu_DMA_GetTransferStatus()) {
        Serial.println("------");
        for (int i = 0; i < memory4adc_size; i+=2) {
            Serial.print("ADC1 value ");
            Serial.print(i/2);
            Serial.print(" for channel 1: ");
            Serial.println(memory4adc1[i]);

            Serial.print("ADC1 value ");
            Serial.print(i/2);
            Serial.print(" for channel 6: ");
            Serial.println(memory4adc1[i+1]);

            Serial.print("ADC3 value ");
            Serial.print(i/2);
            Serial.print(" for channel 5: ");
            Serial.println(memory4adc3[i]);

            Serial.print("ADC3 value ");
            Serial.print(i/2);
            Serial.print(" for channel 9: ");
            Serial.println(memory4adc3[i+1]);
        };

        // restart ADC1
        SensEdu_DMA_ClearTransferStatus();
        SensEdu_DMA_Enable((uint16_t*)memory4adc1, memory4adc_size);
        SensEdu_ADC_Start(adc1);

        // restart ADC3
        SensEdu_DMA_Enable((uint16_t*)memory4adc3, memory4adc_size);
        SensEdu_ADC_Start(adc3);
    }

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
