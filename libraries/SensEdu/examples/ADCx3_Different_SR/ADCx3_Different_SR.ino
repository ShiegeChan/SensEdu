/*
 * ADCx3_Different_SR
 *
 * Runs ADC1, ADC2 and ADC3 in parallel at different sampling rates,
 * each with its own DMA buffer, and sends all three buffers to the host.
 *
 * A measurement is triggered by the character 't' on serial - use the MATLAB
 * script in matlab/ to trigger and plot the data.
 */

#include "SensEdu.h"

// Internal library error container
uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Sampling Rate for each ADC
#define ADC1_SR  250000
#define ADC2_SR  100000
#define ADC3_SR  65000

// For DMA you need to initialize a buffer to store conversion results
// Use the macro SENSEDU_ADC_BUFFER(name, size)
const uint16_t buf_size = 2048;
SENSEDU_ADC_BUFFER(buf1, buf_size);
SENSEDU_ADC_BUFFER(buf2, buf_size);
SENSEDU_ADC_BUFFER(buf3, buf_size);

uint8_t adc_pins[3] = {A0, A1, A6};
ADC_TypeDef* adc1 = ADC1;
SensEdu_ADC_Settings adc1_settings = {
    .adc = adc1,
    .pins = &adc_pins[0],
    .pin_num = 1,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = ADC1_SR,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)buf1,
    .mem_size = buf_size
};

ADC_TypeDef* adc2 = ADC2;
SensEdu_ADC_Settings adc2_settings = {
    .adc = adc2,
    .pins = &adc_pins[1],
    .pin_num = 1,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = ADC2_SR,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)buf2,
    .mem_size = buf_size
};

ADC_TypeDef* adc3 = ADC3;
const uint8_t adc3_pin_num = 1;
uint8_t adc3_pins[adc3_pin_num] = {A6};
SensEdu_ADC_Settings adc3_settings = {
    .adc = adc3,
    .pins = &adc_pins[2],
    .pin_num = 1,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = ADC3_SR,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)buf3,
    .mem_size = buf_size
};

const uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);

    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Init(&adc3_settings);

    SensEdu_ADC_Enable(adc1);
    SensEdu_ADC_Enable(adc2);
    SensEdu_ADC_Enable(adc3);

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    // Wait for the trigger character 't' from the host
    char c;
    while (true) {
        if (Serial.available() > 0) {
            c = Serial.read();
            if (c == 't') {
                break;
            }
        }
        delay(1);
    }

    SensEdu_ADC_Start(adc1);
    SensEdu_ADC_Start(adc2);
    SensEdu_ADC_Start(adc3);
    
    // Wait for the data and send it
    while (!SensEdu_ADC_IsDmaTransferComplete(adc1));
    SensEdu_ADC_ClearDmaTransferComplete(adc1);

    while (!SensEdu_ADC_IsDmaTransferComplete(adc2));
    SensEdu_ADC_ClearDmaTransferComplete(adc2);

    while (!SensEdu_ADC_IsDmaTransferComplete(adc3));
    SensEdu_ADC_ClearDmaTransferComplete(adc3);
    
    serial_send_array(buf1, buf_size, 32);
    serial_send_array(buf2, buf_size, 32);
    serial_send_array(buf3, buf_size, 32);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// Checks if the library has raised any internal errors
// Serial is busy sending measurements, so the error LED is used instead
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
}

// Sends the buffer over serial in fixed-size chunks
void serial_send_array(uint16_t* data, const size_t data_length, const size_t chunk_size_byte) {
    for (size_t i = 0; i < (data_length << 1); i += chunk_size_byte) {
        size_t transfer_size = ((data_length << 1) - i < chunk_size_byte) ? ((data_length << 1) - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}
