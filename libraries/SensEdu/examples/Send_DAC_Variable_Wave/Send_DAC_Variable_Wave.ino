#include <SensEdu.h>

static uint32_t lib_error = 0;
static uint8_t increment_flag = 1; // run time modification flag

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
const size_t lut_size = 4;
static SENSEDU_DAC_BUFFER(lut, lut_size) = {
    0x0000,0x0001,0x0002,0x0003
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);
    // for DAC channel 2, DAC_CH2 as an argument to the function instead of DAC_CH1
    SensEdu_DAC_Settings dac_settings = {DAC_CH1, 64000*16, (uint16_t*)lut, lut_size, 
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0};

    SensEdu_DAC_Init(&dac_settings);
    // for DAC channel 2, DAC_CH2 as an argument to the function instead of DAC_CH1
    SensEdu_DAC_Enable(DAC_CH1);

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
    // modify lut
    for (uint16_t i = 0; i < lut_size; i++) {
        if (increment_flag) {
            lut[i]++;
        } else {
            lut[i]--;
        }
    }

    // out of bounds checks
    if (lut[0] == 0x0000) {
        increment_flag = 1;
    }
    if (lut[lut_size-1] == 0x0FFF) {
        increment_flag = 0;
    }
    
    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}
