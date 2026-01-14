#include "SensEdu.h"

// We need to create a LUT for the sine wave we want to transmit
const uint16_t sine_lut_size_0 = 64; // sine wave size
static uint16_t array_bit0[sine_lut_size_0] = {
0x000, 0x00A, 0x029, 0x05B, 0x0A1, 0x0F9, 0x164, 0x1DF, 
0x26A, 0x303, 0x3A9, 0x459, 0x513, 0x5D5, 0x69C, 0x766, 
0x833, 0x8FE, 0x9C7, 0xA8C, 0xB4A, 0xBFF, 0xCAB, 0xD4A, 
0xDDC, 0xE60, 0xED3, 0xF34, 0xF84, 0xFC0, 0xFE8, 0xFFC, 
0xFFC, 0xFE8, 0xFC0, 0xF84, 0xF34, 0xED3, 0xE60, 0xDDC, 
0xD4A, 0xCAB, 0xBFF, 0xB4A, 0xA8C, 0x9C7, 0x8FE, 0x833, 
0x766, 0x69C, 0x5D5, 0x513, 0x459, 0x3A9, 0x303, 0x26A, 
0x1DF, 0x164, 0x0F9, 0x0A1, 0x05B, 0x029, 0x00A, 0x000
};

const uint16_t sine_lut_size_1 = 64; // sine wave size
static uint16_t array_bit1[sine_lut_size_1] = {
0x000, 0x029, 0x0A1, 0x164, 0x26A, 0x3A9, 0x513, 0x69C, 
0x833, 0x9C7, 0xB4A, 0xCAB, 0xDDC, 0xED3, 0xF84, 0xFE8, 
0xFFC, 0xFC0, 0xF34, 0xE60, 0xD4A, 0xBFF, 0xA8C, 0x8FE, 
0x766, 0x5D5, 0x459, 0x303, 0x1DF, 0x0F9, 0x05B, 0x00A, 
0x00A, 0x05B, 0x0F9, 0x1DF, 0x303, 0x459, 0x5D5, 0x766, 
0x8FE, 0xA8C, 0xBFF, 0xD4A, 0xE60, 0xF34, 0xFC0, 0xFFC, 
0xFE8, 0xF84, 0xED3, 0xDDC, 0xCAB, 0xB4A, 0x9C7, 0x833, 
0x69C, 0x513, 0x3A9, 0x26A, 0x164, 0x0A1, 0x029, 0x000
};

// Big buffer for the entire LUT
const uint16_t MAX_LUT_SIZE = 8*64; // All bits ''0'' 
static SENSEDU_DAC_BUFFER(lut , MAX_LUT_SIZE);
uint8_t serial_buf;

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;
const int SYNC_PIN = 2;


// Configure the DAC1
#define DAC_SINE_FREQ    	32800 
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * 32   // samples per one sine cycle 

SensEdu_DAC_Settings dac_settings_1 = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)lut, 
    .mem_size = MAX_LUT_SIZE, 
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = 1
};

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");

    //Led in red if there is any problem
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);
    pinMode(SYNC_PIN, OUTPUT);
    digitalWrite(SYNC_PIN, LOW);
    
    SensEdu_DAC_Init(&dac_settings_1);
}

void loop () {
    // Opcion 1: esperar al serial input
    //while (Serial.available() == 0); 
    // Lee el primer byte
    //serial_buf = Serial.read(); 
    delay(5000);
    // Opcion 2: fixed bit
    serial_buf = 'U'; // U: 01010101

    digitalWrite(SYNC_PIN, HIGH);
    sendByte(serial_buf);    // Send while HIGH
    digitalWrite(SYNC_PIN, LOW);

    check_errors();
}

// Checking errors of the library
void check_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
        Serial.println(lib_error, HEX);
    }
}

void buildByteLUT(uint8_t data) {
    uint16_t position = 0;
    const uint16_t SAMPLES_PER_BIT = 64; 

    // Clear the entire LUT first (fill with DC level, e.g., 0x800)
    for (size_t i = 0; i < MAX_LUT_SIZE; i++) {
        lut[i] = 0x800;  // Mid-level (silence)
    }

    // Build the waveform
    for (int bit_pos = 7; bit_pos >= 0; bit_pos--) {
        bool bit = (data >> bit_pos) & 1; 
        
        // Guardamos el inicio de este bloque de bit
        uint16_t bit_start_index = (7 - bit_pos) * SAMPLES_PER_BIT;

        if (bit) {
            for (int i = 0; i < sine_lut_size_1; i++) {
                lut[bit_start_index + i] = array_bit1[i];
            }
        } else {
            for (int i = 0; i < sine_lut_size_0; i++) {
                lut[bit_start_index + i] = array_bit0[i];
            }
        }
    }
    // Rest of byte_lut stays at 0x800 (silence padding)
}

void sendByte(uint8_t data) {
    buildByteLUT(data);  // Update byte_lut contents
    
    SensEdu_DAC_Enable(DAC_CH1);
    while (!SensEdu_DAC_GetBurstCompleteFlag(DAC_CH1));
    SensEdu_DAC_ClearBurstCompleteFlag(DAC_CH1);
    SensEdu_DAC_Disable(DAC_CH1);  // Clean shutdown
}