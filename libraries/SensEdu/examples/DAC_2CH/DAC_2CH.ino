#include <SensEdu.h>

uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// how many LUT repeats for one DAC transfer
const uint16_t dac_cycle_num = 10;

// DAC transfered symbols
const size_t sine_lut_size = 64;
const SENSEDU_DAC_BUFFER(sine_lut, sine_lut_size) = {
    0x0000,0x000a,0x0027,0x0058,0x009c,0x00f2,0x0159,0x01d1,0x0258,0x02ed,0x038e,0x043a,0x04f0,0x05ad,0x0670,0x0737,
	0x0800,0x08c8,0x098f,0x0a52,0x0b0f,0x0bc5,0x0c71,0x0d12,0x0da7,0x0e2e,0x0ea6,0x0f0d,0x0f63,0x0fa7,0x0fd8,0x0ff5,
	0x0fff,0x0ff5,0x0fd8,0x0fa7,0x0f63,0x0f0d,0x0ea6,0x0e2e,0x0da7,0x0d12,0x0c71,0x0bc5,0x0b0f,0x0a52,0x098f,0x08c8,
	0x0800,0x0737,0x0670,0x05ad,0x04f0,0x043a,0x038e,0x02ed,0x0258,0x01d1,0x0159,0x00f2,0x009c,0x0058,0x0027,0x000a
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);

    // TODO: make two channels work simultaniously

    SensEdu_DAC_Settings dac_ch1_settings = {DAC_CH1, 32000*64, (uint16_t*)sine_lut, sine_lut_size, 
        SENSEDU_DAC_MODE_BURST_WAVE, dac_cycle_num};

    SensEdu_DAC_Init(&dac_ch1_settings);

    SensEdu_DAC_Settings dac_ch2_settings = {DAC_CH2, 32000*64, (uint16_t*)sine_lut, sine_lut_size, 
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, dac_cycle_num};

    SensEdu_DAC_Init(&dac_ch2_settings);

    
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    SensEdu_DAC_Enable(DAC_CH1);
    SensEdu_DAC_Enable(DAC_CH2);
    
    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    delay(100);
}
