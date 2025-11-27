#include "SensEdu.h"

static uint32_t lib_error = 0;
uint8_t error_led = D86;

SensEdu_DAC_Settings dac_settings = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = 1000,
    .mem_address = NULL, // TODO
    .mem_size = 10, // TODO
    .wave_mode = SENSEDU_DAC_MODE_SINGLE_WAVE,
    .burst_num = 0
};

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    SensEdu_DAC_Init(&dac_settings);
}

void loop () {
    check_errors();

}

void check_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
        Serial.println(lib_error, HEX);
    }
}