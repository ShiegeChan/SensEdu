#include "SensEdu.h"

// Internal library error container
uint32_t lib_error = 0;

// Counter to show that CPU can do something else while DMA transfer is running
uint32_t cntr = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// For DMA you need to initialize a buffer to store conversion results
// Use the macro SENSEDU_ADC_BUFFER(name, size)
const uint16_t buf_size = 128;
SENSEDU_ADC_BUFFER(buf, buf_size);

ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 1;
uint8_t adc_pins[adc_pin_num] = {A0};

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 10000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)buf,
    .mem_size = buf_size
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
    // CPU does something
    cntr += 1;
    Serial.println(cntr);
    check_lib_errors();
    
    // DMA in background
    if (SensEdu_ADC_IsDmaTransferComplete(adc)) {
        Serial.println("------");
        for (int i = 0; i < buf_size; i++) {
            Serial.print("ADC value ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(buf[i]);
        };

        // Restart ADC
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        SensEdu_ADC_Start(adc);
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
