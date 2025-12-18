#include "SensEdu.h"

// Internal library error container
uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 1;
uint8_t adc_pins[adc_pin_num] = {A0};

// If your application doesn't need strict sampling rate - use SENSEDU_ADC_SR_MODE_FREE.
// In case you need to take measurements at specific fixed interval, then prefer SENSEDU_ADC_SR_MODE_FIXED.
// However, use fixed SR mode with care in software polling mode, especially at higher than 1kS/sec frequencies.
// Due to software overhead, data reading loop can perform slower than ADC produces new conversions.
// This results in an event called "overrun" - missing samples and broken SR stability.
// Check OVR flag to see if this happens. If periodic overrun events are not acceptable, use DMA or lower SR.
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
    SensEdu_ADC_EnableOverrunInterrupt(adc);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);

    check_lib_errors();

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

// Serial prints take very long time to execute
// It makes overrun events much more likely to happen
// In actual applications avoid prints in your main loop
void loop() {
    uint16_t data = SensEdu_ADC_ReadConversion(adc);
    Serial.println(data);
    Serial.print("Samples missed: ");
    Serial.println(SensEdu_ADC_GetOverrunCounter(adc));

    check_lib_errors();
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
