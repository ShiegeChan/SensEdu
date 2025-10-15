#include "SensEdu.h"

uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// ADC1 Settings (Timer 1)
ADC_TypeDef* adc1 = ADC1;
const uint8_t adc1_pin_num = 1;
uint8_t adc1_pins[adc1_pin_num] = {A0};  // ADC1 input is A0 pin
SensEdu_ADC_Settings adc1_settings = {
    .adc = adc1,
    .pins = adc1_pins,
    .pin_num = adc1_pin_num,
    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,  // 250 kHz sampling rate
    .dma_mode = SENSEDU_ADC_DMA_DISCONNECT,
    .mem_address = 0x0000,
    .mem_size = 0
};

// ADC2 Settings (Timer 3)
ADC_TypeDef* adc2 = ADC2;
const uint8_t adc2_pin_num = 1;
uint8_t adc2_pins[adc2_pin_num] = {A1};  // ADC2 input is A1 pin
SensEdu_ADC_Settings adc2_settings = {
    .adc = adc2,
    .pins = adc2_pins,
    .pin_num = adc2_pin_num,
    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 125000,  // 125 kHz sampling rate
    .dma_mode = SENSEDU_ADC_DMA_DISCONNECT,
    .mem_address = 0x0000,
    .mem_size = 0
};

// ADC3 Settings (Timer 6)
ADC_TypeDef* adc3 = ADC3;
const uint8_t adc3_pin_num = 1;
uint8_t adc3_pins[adc3_pin_num] = {A6};  // ADC3 input is A6 pin
SensEdu_ADC_Settings adc3_settings = {
    .adc = adc3,
    .pins = adc3_pins,
    .pin_num = adc3_pin_num,
    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 62500,  // 62.5 kHz sampling rate
    .dma_mode = SENSEDU_ADC_DMA_DISCONNECT,
    .mem_address = 0x0000,
    .mem_size = 0
};

uint8_t led = D2; // Test LED output

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    // Initialize Serial
    Serial.begin(115200);
    while (!Serial) {
        delay(1);  // Wait for serial monitor to open (if required)
    }
    Serial.println("Starting ADC Configuration...");

    // Initialize ADC1
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(adc1);
    SensEdu_ADC_Start(adc1);
    lib_error = SensEdu_GetError();
    if (lib_error != 0) {
        Serial.print("ADC1 Error: 0x");
        Serial.println(lib_error, HEX);
    }

    // Initialize ADC2
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc2);
    SensEdu_ADC_Start(adc2);
    lib_error = SensEdu_GetError();
    if (lib_error != 0) {
        Serial.print("ADC2 Error: 0x");
        Serial.println(lib_error, HEX);
    }

    // Initialize ADC3
    SensEdu_ADC_Init(&adc3_settings);
    SensEdu_ADC_Enable(adc3);
    SensEdu_ADC_Start(adc3);
    lib_error = SensEdu_GetError();
    if (lib_error != 0) {
        Serial.print("ADC3 Error: 0x");
        Serial.println(lib_error, HEX);
    }

    // Configure LED Output
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);

    Serial.println("Setup is successful. Running test...");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    // Read ADC1 data
    uint16_t data_adc1 = SensEdu_ADC_ReadConversion(adc1);  // Single-channel ADC1 data
    Serial.print("ADC1 (250 kHz): ");
    Serial.println(data_adc1);

    // Read ADC2 data
    uint16_t data_adc2 = SensEdu_ADC_ReadConversion(adc2);  // Single-channel ADC2 data
    Serial.print("ADC2 (125 kHz): ");
    Serial.println(data_adc2);

    // Read ADC3 data
    uint16_t data_adc3 = SensEdu_ADC_ReadConversion(adc3);  // Single-channel ADC3 data
    Serial.print("ADC3 (62.5 kHz): ");
    Serial.println(data_adc3);

    // Toggle LED for visual feedback
    digitalWrite(led, !digitalRead(led));

    // Add a short delay (adjust as required for testing serial performance)
    delay(100);
}
