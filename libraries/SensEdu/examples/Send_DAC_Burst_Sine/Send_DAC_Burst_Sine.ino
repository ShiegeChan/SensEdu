#include <SensEdu.h>

uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// how many LUT repeats for one DAC transfer
const uint16_t dac_cycle_num = 10;

// DAC transfered symbols
const size_t sine_lut_size =64;
const SENSEDU_DAC_BUFFER(sine_lut, sine_lut_size) = {
   0x800, 0x831, 0x861, 0x890, 0x8BE, 0x8EA, 0x914, 0x93B, 0x95F, 0x980, 0x99D, 0x9B6, 0x9CB, 0x9DB, 0x9E7, 0x9EE, 
0x9F0, 0x9EE, 0x9E7, 0x9DB, 0x9CB, 0x9B6, 0x99D, 0x980, 0x95F, 0x93B, 0x914, 0x8EA, 0x8BE, 0x890, 0x861, 0x831, 
0x800, 0x7CF, 0x79F, 0x770, 0x742, 0x716, 0x6EC, 0x6C5, 0x6A1, 0x680, 0x663, 0x64A, 0x635, 0x625, 0x619, 0x612, 
0x610, 0x612, 0x619, 0x625, 0x635, 0x64A, 0x663, 0x680, 0x6A1, 0x6C5, 0x6EC, 0x716, 0x742, 0x770, 0x79F, 0x7CF};

#define DAC_SINE_FREQ       32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * 64   // 64 samples per one sine cycle

DAC_Channel* dac_ch = DAC_CH1;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_ch, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut,
    .mem_size = sine_lut_size,
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = dac_cycle_num
};
/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);

    SensEdu_DAC_Init(&dac_settings);

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
    SensEdu_DAC_Enable(dac_ch);
    
    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    delay(100);
}
