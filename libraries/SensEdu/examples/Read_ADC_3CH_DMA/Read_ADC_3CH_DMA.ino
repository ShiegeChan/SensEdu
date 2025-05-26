#include "SensEdu.h"

uint32_t lib_error = 0;
uint32_t cntr = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 3;
uint8_t adc_pins[adc_pin_num] = {A0, A1, A2}; 
// must be:
// 1. multiple of 32 bytes to ensure cache coherence
// 2. properly aligned
const uint16_t memory4adc_size = 64 * adc_pin_num; // allocate chunks of (4 * 32) * number of channels
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t memory4adc[memory4adc_size];

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT,
    .sampling_freq = 0,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)memory4adc,
    .mem_size = memory4adc_size
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

    SensEdu_ADC_Init(&adc_settings);
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
    Serial.print("Counter value: ");
    Serial.println(cntr);
    
    // DMA in background

    // Print transfered Data if available
    if (SensEdu_ADC_GetTransferStatus(adc)) {
        Serial.println("------");
        for (int i = 0; i < memory4adc_size; i+=3) {
            Serial.print("ADC value ");
            Serial.print(i/3);
            Serial.print(" for channel 0: ");
            Serial.println(memory4adc[i]);

            Serial.print("ADC value ");
            Serial.print(i/3);
            Serial.print(" for channel 1: ");
            Serial.println(memory4adc[i+1]);

            Serial.print("ADC value ");
            Serial.print(i/3);
            Serial.print(" for channel 2: ");
            Serial.println(memory4adc[i+2]);
        };

        // restart ADC
        SensEdu_ADC_ClearTransferStatus(adc);
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
