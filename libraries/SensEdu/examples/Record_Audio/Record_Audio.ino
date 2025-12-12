#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

// Library error container
static uint32_t lib_error = 0;

// Error indication pin
static uint8_t error_led = D86;

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

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 44100,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
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

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
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
        while(!SensEdu_ADC_GetTransferStatus(adc));
        SensEdu_ADC_ClearTransferStatus(adc);
        serial_send_array((const uint8_t *)&mic_data, mic_data_size << 1);
    }
    is_recording_started = false;

    // Check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// send serial data in 32 byte chunks
void serial_send_array(const uint8_t* data, size_t size) {
    const size_t chunk_size = 32;
	for (uint32_t i = 0; i < size/chunk_size; i++) {
		Serial.write(data + chunk_size * i, chunk_size);
	}
}
