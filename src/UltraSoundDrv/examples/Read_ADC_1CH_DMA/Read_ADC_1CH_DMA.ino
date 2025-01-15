#include "UltraSoundDrv.h"
//#include "src/UltraSoundDrv/src/UltraSoundDrv.h"

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
const uint16_t memory4adc_size = 64;
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

    UltraSoundDrv_Init(adc, adc_pins, adc_pin_num, ULTRASOUND_DRV_ADC_MODE_CONT, 1000); // continuos mode for ADC
    UltraSoundDrv_ADC_Enable(adc);

    UltraSoundDrv_DMA_Init((uint16_t*)memory4adc); 
    UltraSoundDrv_DMA_Enable((uint16_t*)memory4adc, memory4adc_size);

    UltraSoundDrv_ADC_Start(adc);

    lib_error = UltraSoundDrv_GetError();
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
    if (UltraSoundDrv_DMA_GetTransferStatus()) {
        Serial.println("------");
        for (int i = 0; i < memory4adc_size; i++) {
            Serial.print("ADC value ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(memory4adc[i]);
        };

        // restart ADC
        UltraSoundDrv_DMA_ClearTransferStatus();
        UltraSoundDrv_DMA_Enable((uint16_t*)memory4adc, memory4adc_size);
        UltraSoundDrv_ADC_Start(adc);
    }

    // check errors
    lib_error = UltraSoundDrv_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
