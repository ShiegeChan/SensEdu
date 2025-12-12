#include "SensEdu.h"

// We need to create a LUT for the sine wave we want to transmit
const uint16_t sine_lut_size = 64; // sine wave size
const SENSEDU_DAC_BUFFER(sine_lut, sine_lut_size) = {
    0x0000,0x000a,0x0027,0x0058,0x009c,0x00f2,0x0159,0x01d1,
    0x0258,0x02ed,0x038e,0x043a,0x04f0,0x05ad,0x0670,0x0737,
    0x0800,0x08c8,0x098f,0x0a52,0x0b0f,0x0bc5,0x0c71,0x0d12,
    0x0da7,0x0e2e,0x0ea6,0x0f0d,0x0f63,0x0fa7,0x0fd8,0x0ff5,
    0x0fff,0x0ff5,0x0fd8,0x0fa7,0x0f63,0x0f0d,0x0ea6,0x0e2e,
    0x0da7,0x0d12,0x0c71,0x0bc5,0x0b0f,0x0a52,0x098f,0x08c8,
    0x0800,0x0737,0x0670,0x05ad,0x04f0,0x043a,0x038e,0x02ed,
    0x0258,0x01d1,0x0159,0x00f2,0x009c,0x0058,0x0027,0x000a
};

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;

// Configure the DAC
#define DAC_SINE_FREQ     	33000                           // 33kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle 

SensEdu_DAC_Settings dac_settings = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut, 
    .mem_size = sine_lut_size, 
    .wave_mode = SENSEDU_DAC_MODE_CONTINUOUS_WAVE,
    .burst_num = 0
};

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");

    //Led in red if there is any problem
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    //Initializing DAC
    SensEdu_DAC_Init(&dac_settings);
    SensEdu_DAC_Enable(DAC_CH1);
}

void loop () {
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