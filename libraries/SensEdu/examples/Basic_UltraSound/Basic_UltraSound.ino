#include "SensEdu.h"
#include "SineLUT.h"

// Internal library error container
uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

/* DAC */
// LUT settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

DAC_Channel* dac_ch = DAC_CH1;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_ch, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut,
    .mem_size = sine_lut_size,
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = dac_cycle_num
};

/* ADC */
const uint16_t mic_data_size = 5142;
SENSEDU_ADC_BUFFER(mic_data, mic_data_size);

ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = {A1};
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = mic_pins,
    .pin_num = mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)mic_data,
    .mem_size = mic_data_size
};

const uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);

    SensEdu_DAC_Init(&dac_settings);

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    // Wait for trigger character 't' from computing device
    char c;
    while (true) {
        if (Serial.available() > 0) {
            c = Serial.read();
            if (c == 't') {
                break;
            }
        }
        delay(1);
    }

    // Start dac->adc sequence
    SensEdu_DAC_Enable(dac_ch);
    while (!SensEdu_DAC_GetBurstCompleteFlag(dac_ch));
    SensEdu_DAC_ClearBurstCompleteFlag(dac_ch);
    SensEdu_ADC_Start(adc);
    
    // Wait for the data and send it
    while (!SensEdu_ADC_IsDmaTransferComplete(adc));
    SensEdu_ADC_ClearDmaTransferComplete(adc);
    serial_send_array(&(mic_data[0]), mic_data_size, 32);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// Checks if the library has risen any internal errors
// Doesn't print the error code, since Serial is occupied
// Turns on the red LED on Arduino board instead
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
}

void serial_send_array(uint16_t* data, const size_t data_length, const size_t chunk_size_byte) {
    for (size_t i = 0; i < (data_length << 1); i += chunk_size_byte) {
        size_t transfer_size = ((data_length << 1) - i < chunk_size_byte) ? ((data_length << 1) - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}
