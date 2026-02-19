#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

#define TWO_PI  (6.28318530718f)

#define FREQ0   (16000.0f)
#define FREQ1   (32000.0f)

#define SAMPLE_RATE (33000 * 16)

const float PHASE_INC0 = (TWO_PI * FREQ0) / SAMPLE_RATE;
const float PHASE_INC1 = (TWO_PI * FREQ1) / SAMPLE_RATE;

// Error indicator LED
static const uint8_t ERROR_LED_PIN = D86;

// Maximum number of characters to send between separate messages
const uint16_t MAX_MESSAGE_LENGTH = 30;

// ASCII standard 1 byte per letter
const uint16_t BIT_PER_CHARACTER = 8;

// Arbitrary chosen number to send ~7-9 cycles per bit
// 35kHz ~15 samples per cycle
// 31kHz ~17 samples per cycle
const uint16_t SAMPLES_PER_BIT = 200;
const uint16_t SAMPLES_PER_CHARACTER = SAMPLES_PER_BIT * BIT_PER_CHARACTER;

// x4 extra characters reserved for the preamble
const uint16_t PREAMBLE_LENGTH = 4;

// DMA buffer size
const uint16_t MAX_LUT_SIZE = (MAX_MESSAGE_LENGTH + PREAMBLE_LENGTH) * SAMPLES_PER_CHARACTER; 

volatile SENSEDU_DAC_BUFFER(dma_buffer, MAX_LUT_SIZE);
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = SAMPLE_RATE,
    .mem_address = (uint16_t*)dma_buffer,
    .mem_size = MAX_LUT_SIZE, 
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = 1
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("Started Initialization...");

    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, HIGH);
    
    SensEdu_DAC_Init(&dac_settings);

    check_lib_errors(ERROR_LED_PIN);
    Serial.println("Setup is successful.");

    Serial.println("Please enter the message to transmit (30 characters max).");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

uint8_t message[MAX_MESSAGE_LENGTH];
size_t length = 0;

void loop () {
    check_lib_errors(ERROR_LED_PIN);

    if (Serial.available() > 0) {
        length = 0;
        while (Serial.available() > 0 && length < MAX_MESSAGE_LENGTH) {
            message[length] = Serial.read();
            length++;
        }
        
        while (length > 0 && (message[length - 1] == '\n' || message[length - 1] == '\r')) {
            length--;
        }
        
        if (length > 0) {
            Serial.println("Transmitted message: ");
            Serial.write(message, length);
            Serial.println("");
            send_message(message, length); 
        }
    }
}

void send_message(uint8_t* data, uint8_t num_bytes) {
    construct_buffer(data, num_bytes);
    SensEdu_DAC_Enable(DAC_CH1);
    while (!SensEdu_DAC_GetBurstCompleteFlag(DAC_CH1));
    SensEdu_DAC_ClearBurstCompleteFlag(DAC_CH1);
    SensEdu_DAC_Disable(DAC_CH1); // Clean shutdown
}

// Phase resets between messages
void construct_buffer(uint8_t* data, uint8_t num_bytes) {

    // Position in a LUT buffer
    uint16_t position = 0;

    // Current phase of the output sine wave
    float phase = 0.0f;

    // Clear the entire LUT first with DC level
    for (size_t i = 0; i < MAX_LUT_SIZE; i++) {
        dma_buffer[i] = 0x000;
    }

    // Preamble 10101010 x PREAMBLE_LENGTH times
    for (size_t i = 0; i < PREAMBLE_LENGTH * BIT_PER_CHARACTER; i++) {
        //send_bit(i % 2, &phase, &position);
    }

    // Payload
    for (size_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        uint8_t cur_byte = data[byte_idx];
        for (size_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            bool bit = (cur_byte >> (7 - bit_idx)) & 1;
            send_bit(bit, &phase, &position);
        }
    }
}

void send_bit(bool bit, float* phase, uint16_t* buf_pos) {
    float phase_inc = bit ? PHASE_INC1 : PHASE_INC0;
    for (size_t i = 0; i < SAMPLES_PER_BIT; i++) {
        *phase += phase_inc;
        if (*phase > TWO_PI) {
            *phase -= TWO_PI;
        }

        float sample = sinf(*phase);
        dma_buffer[*buf_pos] = (uint16_t)((sample + 1.0f) * 2047.5f);
        (*buf_pos)++;
    }
}

// Check library error state
static void check_lib_errors(uint8_t error_led) {
    uint32_t lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        fatal_error(error_led);
    }
}

// Halt system on fatal error
static void fatal_error(uint8_t error_led) {
    digitalWrite(error_led, !digitalRead(error_led));
    delay(200);
}
