#include "SensEdu.h"
//#include <bitset>

// We need to create a LUT for the sine wave we want to transmit
const uint16_t sine_lut_size_0 = 68; // sine wave size
static SENSEDU_DAC_BUFFER(sine_lut_0, sine_lut_size_0) = {
0x000, 0x009, 0x024, 0x050, 0x08E, 0x0DD, 0x13C, 0x1AA, 
0x226, 0x2AF, 0x344, 0x3E4, 0x48D, 0x53E, 0x5F5, 0x6B1, 
0x770, 0x82F, 0x8EF, 0x9AC, 0xA66, 0xB1A, 0xBC7, 0xC6C, 
0xD07, 0xD96, 0xE19, 0xE8E, 0xEF5, 0xF4B, 0xF92, 0xFC7, 
0xFEB, 0xFFD, 0xFFD, 0xFEB, 0xFC7, 0xF92, 0xF4B, 0xEF5, 
0xE8E, 0xE19, 0xD96, 0xD07, 0xC6C, 0xBC7, 0xB1A, 0xA66, 
0x9AC, 0x8EF, 0x82F, 0x770, 0x6B1, 0x5F5, 0x53E, 0x48D, 
0x3E4, 0x344, 0x2AF, 0x226, 0x1AA, 0x13C, 0x0DD, 0x08E, 
0x050, 0x024, 0x009, 0x000
};

const uint16_t sine_lut_size_1 = 51; // sine wave size
static SENSEDU_DAC_BUFFER(sine_lut_1, sine_lut_size_1) = {
0x000, 0x010, 0x040, 0x090, 0x0FD, 0x187, 0x22B, 0x2E6, 
0x3B6, 0x498, 0x587, 0x680, 0x77F, 0x880, 0x97F, 0xA78, 
0xB67, 0xC49, 0xD19, 0xDD4, 0xE78, 0xF02, 0xF6F, 0xFBF, 
0xFEF, 0xFFF, 0xFEF, 0xFBF, 0xF6F, 0xF02, 0xE78, 0xDD4, 
0xD19, 0xC49, 0xB67, 0xA78, 0x97F, 0x880, 0x77F, 0x680, 
0x587, 0x498, 0x3B6, 0x2E6, 0x22B, 0x187, 0x0FD, 0x090, 
0x040, 0x010, 0x000
};

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;

const int SYNC_PIN = 2;

// Configure the DAC1
#define DAC_SINE_FREQ    	32000                          // Wave of 20kHz
#define DAC_SAMPLE_RATE    DAC_SINE_FREQ * 64   // 64 samples per one sine cycle 

SensEdu_DAC_Settings dac_settings_1 = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut_0, 
    .mem_size = sine_lut_size_0, 
    .wave_mode = SENSEDU_DAC_MODE_CONTINUOUS_WAVE,
    .burst_num = 0
};

// Configure the DAC2

SensEdu_DAC_Settings dac_settings_2 = {
    .dac_channel = DAC_CH2, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut_1, 
    .mem_size = sine_lut_size_1, 
    .wave_mode = SENSEDU_DAC_MODE_CONTINUOUS_WAVE,
    .burst_num = 0
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

    pinMode(SYNC_PIN, OUTPUT);
    digitalWrite(SYNC_PIN, LOW);
    
    SensEdu_DAC_Init(&dac_settings_1);

    SensEdu_DAC_Init(&dac_settings_2);
}

void loop () {
    digitalWrite(SYNC_PIN, HIGH);  // Signal RX to start
    SensEdu_DAC_Enable(DAC_CH1);
    delayMicroseconds(1000);
    SensEdu_DAC_Disable(DAC_CH1);
    digitalWrite(SYNC_PIN, LOW);

    delayMicroseconds(5000);

    digitalWrite(SYNC_PIN, HIGH);  // Signal RX to start
    SensEdu_DAC_Enable(DAC_CH2);
    delayMicroseconds(1000);
    SensEdu_DAC_Disable(DAC_CH2);
    digitalWrite(SYNC_PIN, LOW);

    delayMicroseconds(1000);

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