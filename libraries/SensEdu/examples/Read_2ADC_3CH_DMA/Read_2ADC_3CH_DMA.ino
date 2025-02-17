#include "SensEdu.h"

uint32_t lib_error = 0;
uint32_t cntr = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
const uint8_t adc1_pin_num = 3;
const uint8_t adc2_pin_num = 3;
uint8_t adc1_pins[adc1_pin_num] = {A0, A1, A2}; 
uint8_t adc2_pins[adc2_pin_num] = {A4, A5, A6};

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t memory4adc1_size = 64 * adc1_pin_num;
const uint16_t memory4adc2_size = 64 * adc2_pin_num;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc1[memory4adc1_size];
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc2[memory4adc2_size];

SensEdu_ADC_Settings adc1_settings = {
    .adc = ADC1,
    .pins = adc1_pins,
    .pin_num = adc1_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)memory4adc1,
    .mem_size = memory4adc1_size
};

SensEdu_ADC_Settings adc2_settings = {
    .adc = ADC2,
    .pins = adc2_pins,
    .pin_num = adc2_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)memory4adc2,
    .mem_size = memory4adc2_size
};

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
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(ADC1);
    SensEdu_ADC_Start(ADC1);

    /* ADC 2 */
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(ADC2);
    SensEdu_ADC_Start(ADC2);

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
    if (SensEdu_ADC_GetTransferStatus(ADC1)) {
        Serial.print("---- ");
        Serial.print("ADC1 ");
        Serial.println("----");
        for (int i = 0; i < memory4adc1_size; i+=3) {
            Serial.print("ADC1 value ");
            Serial.print(i/3);
            Serial.print(" for channel 1: ");
            Serial.println(memory4adc1[i]);

            Serial.print("ADC1 value ");
            Serial.print(i/3);
            Serial.print(" for channel 2: ");
            Serial.println(memory4adc1[i+1]);

            Serial.print("ADC1 value ");
            Serial.print(i/3);
            Serial.print(" for channel 3: ");
            Serial.println(memory4adc1[i+2]);
        };

        // restart ADC1
        SensEdu_ADC_ClearTransferStatus(ADC1);
        SensEdu_ADC_Start(ADC1);
    }

    if (SensEdu_ADC_GetTransferStatus(ADC2)) {
        Serial.print("---- ");
        Serial.print("ADC2 ");
        Serial.println("----");
        for (int i = 0; i < memory4adc2_size; i+=3) {
            Serial.print("ADC2 value ");
            Serial.print(i/3);
            Serial.print(" for channel 5: ");
            Serial.println(memory4adc2[i]);

            Serial.print("ADC2 value ");
            Serial.print(i/3);
            Serial.print(" for channel 6: ");
            Serial.println(memory4adc2[i+1]);

            Serial.print("ADC2 value ");
            Serial.print(i/3);
            Serial.print(" for channel 7: ");
            Serial.println(memory4adc2[i+2]);
        };

        // restart ADC2
        SensEdu_ADC_ClearTransferStatus(ADC2);
        SensEdu_ADC_Start(ADC2);
    }

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
