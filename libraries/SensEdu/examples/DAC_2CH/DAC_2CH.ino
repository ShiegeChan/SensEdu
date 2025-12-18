#include <SensEdu.h>

uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// How many LUT repeats for one DAC transfer
const uint16_t dac_cycle_num = 10;

// DAC transfered symbols
const size_t lut1_size = 64;
const SENSEDU_DAC_BUFFER(lut1, lut1_size) = {
    0x0000,0x000a,0x0027,0x0058,0x009c,0x00f2,0x0159,0x01d1,0x0258,0x02ed,0x038e,0x043a,0x04f0,0x05ad,0x0670,0x0737,
	0x0800,0x08c8,0x098f,0x0a52,0x0b0f,0x0bc5,0x0c71,0x0d12,0x0da7,0x0e2e,0x0ea6,0x0f0d,0x0f63,0x0fa7,0x0fd8,0x0ff5,
	0x0fff,0x0ff5,0x0fd8,0x0fa7,0x0f63,0x0f0d,0x0ea6,0x0e2e,0x0da7,0x0d12,0x0c71,0x0bc5,0x0b0f,0x0a52,0x098f,0x08c8,
	0x0800,0x0737,0x0670,0x05ad,0x04f0,0x043a,0x038e,0x02ed,0x0258,0x01d1,0x0159,0x00f2,0x009c,0x0058,0x0027,0x000a
};

const size_t lut2_size = 64;
const SENSEDU_DAC_BUFFER(lut2, lut2_size) = {
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,
    0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0,0x0ff0
};

#define DAC_SAMPLE_RATE     (1000*64)   // 64kHz

DAC_Channel* dac_ch1 = DAC_CH1;
DAC_Channel* dac_ch2 = DAC_CH2;

SensEdu_DAC_Settings dac1_settings = {
    .dac_channel = dac_ch1, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)lut1,
    .mem_size = lut1_size,
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = dac_cycle_num
};

SensEdu_DAC_Settings dac2_settings = {
    .dac_channel = dac_ch2, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)lut2,
    .mem_size = lut2_size,
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = dac_cycle_num
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    // Stuck in the loop if Serial Monitor is not opened
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("Started Initialization...");

    SensEdu_DAC_Init(&dac1_settings);
    SensEdu_DAC_Init(&dac2_settings);

    check_lib_errors();

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    check_lib_errors();

    SensEdu_DAC_Enable(dac_ch1);
    SensEdu_DAC_Enable(dac_ch2);
    
    delay(100);
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
