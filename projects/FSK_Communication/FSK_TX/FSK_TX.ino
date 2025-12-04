#include "SensEdu.h"
//#include <bitset>

// We need to create a LUT for the sine wave we want to transmit
const uint16_t sine_lut_size = 64; // sine wave size
static SENSEDU_DAC_BUFFER(sine_lut, sine_lut_size) = {
0x000, 0x00A, 0x029, 0x05B, 0x0A1, 0x0F9, 0x164, 0x1DF, 
0x26A, 0x303, 0x3A9, 0x459, 0x513, 0x5D5, 0x69C, 0x766, 
0x833, 0x8FE, 0x9C7, 0xA8C, 0xB4A, 0xBFF, 0xCAB, 0xD4A, 
0xDDC, 0xE60, 0xED3, 0xF34, 0xF84, 0xFC0, 0xFE8, 0xFFC, 
0xFFC, 0xFE8, 0xFC0, 0xF84, 0xF34, 0xED3, 0xE60, 0xDDC, 
0xD4A, 0xCAB, 0xBFF, 0xB4A, 0xA8C, 0x9C7, 0x8FE, 0x833, 
0x766, 0x69C, 0x5D5, 0x513, 0x459, 0x3A9, 0x303, 0x26A, 
0x1DF, 0x164, 0x0F9, 0x0A1, 0x05B, 0x029, 0x00A, 0x000
};

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;

// Configure the DAC
#define DAC_SINE_FREQ_0    	20000                          // 32kHz
#define DAC_SAMPLE_RATE_0    DAC_SINE_FREQ_0 * sine_lut_size   // 64 samples per one sine cycle 
SensEdu_DAC_Settings dac_settings_1 = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = DAC_SAMPLE_RATE_0,
    .mem_address = (uint16_t*)sine_lut, 
    .mem_size = sine_lut_size, 
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = 10
};

// Configure the DAC
#define DAC_SINE_FREQ_1    	40000                          // 32kHz
#define DAC_SAMPLE_RATE_1    DAC_SINE_FREQ_1 * sine_lut_size   // 64 samples per one sine cycle 

SensEdu_DAC_Settings dac_settings_2 = {
    .dac_channel = DAC_CH2, 
    .sampling_freq = DAC_SAMPLE_RATE_1,
    .mem_address = (uint16_t*)sine_lut, 
    .mem_size = sine_lut_size, 
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = 10
};

// Number that you want to send, and time to each bit
//int number = 8;  // Read the number
//std::bitset<16> binary(number);  // Convert to 16-bit binary
//unsigned long startTime = millis();  // Record start time
//unsigned long duration_ms = 500;     // Run for 500 ms

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");

    //Led in red if there is any problem
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);


}

void loop () {
    SensEdu_DAC_Init(&dac_settings_1);
    SensEdu_DAC_Enable(DAC_CH1);
    while(!SensEdu_DAC_GetBurstCompleteFlag(DAC_CH1));
    SensEdu_DAC_ClearBurstCompleteFlag(DAC_CH1);
    SensEdu_DAC_Disable(DAC_CH1);
    delay(10);
    SensEdu_DAC_Init(&dac_settings_2);
    SensEdu_DAC_Enable(DAC_CH2);
    while(!SensEdu_DAC_GetBurstCompleteFlag(DAC_CH2));
    SensEdu_DAC_ClearBurstCompleteFlag(DAC_CH2);
    SensEdu_DAC_Disable(DAC_CH2);
    delay(10);
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