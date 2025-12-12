#include "SensEdu.h"
// We need to create a LUT for the sine wave we want to transmit
const uint16_t sine_lut_size_0 = 34; // sine wave size
static uint16_t array_bit0[sine_lut_size_0] = {
    0x000, 0x025, 0x093, 0x145, 0x236, 0x35C, 0x4AD, 0x61D, 
    0x79E, 0x923, 0xA9D, 0xBFF, 0xD3C, 0xE49, 0xF1B, 0xFAC, 
    0xFF6, 0xFF6, 0xFAC, 0xF1B, 0xE49, 0xD3C, 0xBFF, 0xA9D, 
    0x923, 0x79E, 0x61D, 0x4AD, 0x35C, 0x236, 0x145, 0x093, 
    0x025, 0x000
};

const uint16_t sine_lut_size_1 = 29; // sine wave size
static uint16_t array_bit1[sine_lut_size_1] = {
    0x000, 0x033, 0x0CB, 0x1BF, 0x303, 0x487, 0x638, 0x800, 
    0x9C7, 0xB78, 0xCFC, 0xE40, 0xF34, 0xFCC, 0xFFF, 0xFCC, 
    0xF34, 0xE40, 0xCFC, 0xB78, 0x9C7, 0x7FF, 0x638, 0x487, 
    0x303, 0x1BF, 0x0CB, 0x033, 0x000
};

// Big buffer for the entire LUT
const uint16_t MAX_LUT_SIZE = 8*34; // All bits ''1'' -> 1088 values
static SENSEDU_DAC_BUFFER(lut , MAX_LUT_SIZE);
uint8_t serial_buf;

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;
const int SYNC_PIN = 2;


// Configure the DAC1
#define DAC_SINE_FREQ    	16000 // too low, 28-30 should be ok
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * 64   // 64 samples per one sine cycle 

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
    delay(100);
    // Opcion 2: fixed bit
    serial_buf = 'U'; // U: 01010101

    digitalWrite(SYNC_PIN, HIGH);
    delay(100);  // Give RX time to detect rising edge
    sendByte(serial_buf);    // Send while HIGH
    digitalWrite(SYNC_PIN, LOW);

    delay(100);
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

    // Clear the entire LUT first (fill with DC level, e.g., 0x8000)
    for (size_t i = 0; i < MAX_LUT_SIZE; i++) {
        lut[i] = 0x0000;  // Mid-level (silence)
    }

    // Build the waveform
    // extremely weird dont do it ike this please, no negative indicies
    for (int bit_pos = 7; bit_pos >= 0; bit_pos--) {
        bool bit = (data >> bit_pos) & 1; // do you really need this & 1
        
        if (bit) {
            for (int i = 0; i < sine_lut_size_1; i++) {
                lut[position++] = array_bit1[i];
            }
        } else {
            for (int i = 0; i < sine_lut_size_0; i++) {
                lut[position++] = array_bit0[i];
            }
        }
    }
    // Rest of byte_lut stays at 0x800 (silence padding)
}

void sendByte(uint8_t data) {
    buildByteLUT(data);  // Update byte_lut contents
    
    // DAC settings already point to byte_lut, just trigger
    SensEdu_DAC_Enable(DAC_CH1);
    while (!SensEdu_DAC_GetBurstCompleteFlag(DAC_CH1));
    SensEdu_DAC_ClearBurstCompleteFlag(DAC_CH1);
    //SensEdu_DAC_Disable(DAC_CH1); not needed because burst mode
}