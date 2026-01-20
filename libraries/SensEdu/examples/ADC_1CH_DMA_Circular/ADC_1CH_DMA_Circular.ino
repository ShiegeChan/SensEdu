#include "SensEdu.h"

// Internal library error container
uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

const uint16_t buf_size = 2048;
SENSEDU_DMA_BUFFER(buf, buf_size);

ADC_TypeDef* adc = ADC1;
const uint8_t adc_pin_num = 1;
uint8_t adc_pins[adc_pin_num] = {A0};

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = adc_pin_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 44100,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_CIRCULAR,
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

    SCB_DisableDCache();

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

uint32_t capture_remaining = 0;
bool capture_active = false;

void loop() {
    char c;
    if (Serial.available() > 0) {
        c = Serial.read();
        if (c == 't') {
            capture_remaining = 40;
            capture_active = true;
            SensEdu_ADC_ClearDmaTransferComplete(adc);
            SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        }
    }

    if (!capture_active) return;

    if (capture_remaining && SensEdu_ADC_IsDmaHalfTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        serial_send_array(&buf[0], buf_size/2, 32);
        capture_remaining--;
    }

    if (capture_remaining && SensEdu_ADC_IsDmaTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        serial_send_array(&(buf[buf_size/2]), buf_size/2, 32);
        capture_remaining--;
    }

    if (capture_remaining == 0) {
        capture_active = false;
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

void serial_send_array(uint16_t* data, const size_t data_length, const size_t chunk_size_byte) {
    for (size_t i = 0; i < (data_length << 1); i += chunk_size_byte) {
        size_t transfer_size = ((data_length << 1) - i < chunk_size_byte) ? ((data_length << 1) - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}
