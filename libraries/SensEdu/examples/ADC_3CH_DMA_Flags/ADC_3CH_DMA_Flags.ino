#include "SensEdu.h"

// Internal library error container
uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// For DMA you need to initialize a buffer to store conversion results
// Use the macro SENSEDU_ADC_BUFFER(name, size)
const uint16_t buf_size = 90;
SENSEDU_ADC_BUFFER(buf_ct, buf_size);   // buffer for complete transfer
SENSEDU_ADC_BUFFER(buf_ht, buf_size);   // buffer for half transfer

ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 3;
uint8_t adc_pins[adc_pin_num] = {A0, A1, A2}; 

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 10000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)buf_ct,
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
    check_lib_errors();

    // 0x0101: 257 in decimal
    memset(buf_ct, 0x01, sizeof(buf_ct));
    memset(buf_ht, 0x01, sizeof(buf_ht));

    // Clean the cache to apply 257 value properly
    SCB_CleanDCache_by_Addr(buf_ct, sizeof(buf_ct));
    SCB_CleanDCache_by_Addr(buf_ht, sizeof(buf_ht));

    Serial.println("Setup is successful.");
    SensEdu_ADC_Start(adc);
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    // Half Transfer
    if (SensEdu_ADC_IsDmaHalfTransferComplete(adc)) {
        for (size_t i = 0; i < buf_size; i++) {
            buf_ht[i] = buf_ct[i];
        }
        SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
    }

    // Complete Transfer
    if (SensEdu_ADC_IsDmaTransferComplete(adc)) {
        Serial.println("Half Transfer Buffer:");
        for (int i = 0; i < buf_size; i+=3) {
            Serial.print("Index ");
            Serial.print(i/3);
            Serial.print(": CH0 = ");
            Serial.print(buf_ht[i]);
            Serial.print(" | CH1 = ");
            Serial.print(buf_ht[i+1]);
            Serial.print(" | CH2 = ");
            Serial.println(buf_ht[i+2]);
        };
        Serial.println("----------------");
        Serial.println("Complete Transfer Buffer:");
        for (int i = 0; i < buf_size; i+=3) {
            Serial.print("Index ");
            Serial.print(i/3);
            Serial.print(": CH0 = ");
            Serial.print(buf_ct[i]);
            Serial.print(" | CH1 = ");
            Serial.print(buf_ct[i+1]);
            Serial.print(" | CH2 = ");
            Serial.println(buf_ct[i+2]);
        };
        SensEdu_ADC_ClearDmaTransferComplete(adc);
    }

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
