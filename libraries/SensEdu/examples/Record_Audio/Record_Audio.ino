#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

// Internal library error container
uint32_t lib_error = 0;

// Error indication pin
const uint8_t error_led = D86;

// Flag to indicate the recording start
static bool is_recording_started = false;

// Counter for MATLAB synchronization (must match with receiver script)
static const uint16_t LOOP_COUNT = 500;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

const uint16_t mic_data_size = 2048;
SENSEDU_ADC_BUFFER(mic_data, mic_data_size);

ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = {A1};
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = mic_pins,
    .pin_num = mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 44100,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)mic_data,
    .mem_size = mic_data_size
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {

    Serial.begin(115200);

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    // Recording is initiated by the signal from computing device
    static char serial_buf = 0;
    while (!is_recording_started) {
        while (Serial.available() == 0);
        serial_buf = Serial.read();

        if (serial_buf == 't') {
            is_recording_started = true;
            break;
        }
    }

    // Recording loop
    for (uint16_t i = 0; i < LOOP_COUNT; i++) {
        SensEdu_ADC_Start(adc);
        // Wait for the data and send it
        while (!SensEdu_ADC_IsDmaTransferComplete(adc));
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        serial_send_array(&(mic_data[0]), mic_data_size, 32);
    }
    is_recording_started = false;

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// Checks if the library has risen any internal errors
// Doesn't print the error code, since Serial is occupied
// Turns on the red LED on Arduino board instead
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
}

void serial_send_array(uint16_t* data, const size_t data_length, const size_t chunk_size_byte) {
    for (size_t i = 0; i < (data_length << 1); i += chunk_size_byte) {
        size_t transfer_size = ((data_length << 1) - i < chunk_size_byte) ? ((data_length << 1) - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}
