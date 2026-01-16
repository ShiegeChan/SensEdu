#include "SensEdu.h"

// Internal library error container
uint32_t lib_error = 0;


/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 3;
uint8_t adc_pins[adc_pin_num] = {A0, A1, A2};

// Continuous multi-channel scan is meant to be used with DMA only.
// Due to race conditions between EOC, EOS flags, and the automatic start of the next conversion,
// reliable multi-channel polling is not guaranteed, occasional desynchronization may occur, breaking channel alignment.
// Higher sampling rate increases the risk of misalignment even further.
//
// Only use this example for experimentation, testing, or low-speed scenarios.
// For projects requiring continuous multi-channel acquisition, always use DMA.
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 1000,
    
    .adc_mode = SENSEDU_ADC_MODE_POLLING_CONT,
    .mem_address = 0x0000,
    .mem_size = 0
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    // Stuck in the loop if Serial Monitor is not opened
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("Started Initialization...");
    
    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);

    check_lib_errors();

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    uint16_t* data = SensEdu_ADC_ReadSequence(adc);
    check_lib_errors();

    Serial.println("-------");
    for (uint8_t i = 0; i < adc_pin_num; i++) {
        Serial.print("CH");
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(data[i]);
    }
}

// Checks if the library has risen any internal errors
// Prints the error code in Serial Monitor
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
