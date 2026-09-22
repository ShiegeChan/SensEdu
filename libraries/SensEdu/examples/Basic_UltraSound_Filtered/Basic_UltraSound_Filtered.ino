/*
 * Basic_UltraSound_Filtered
 *
 * Same measurement as Basic_UltraSound, with on-board filtering added: emits a
 * 32 kHz sine burst on the DAC speaker, records one microphone via DMA, then
 * rescales and bandpass filters the echo. Every measurement sends both raw and 
 * filtered buffers for comparison.
 *
 * The FIR filter removes the ADC DC offset, audible-band disturbances and
 * high-frequency noise, leaving the 32 kHz echo.
 *
 * The raw buffer is sent unscaled to halve its transfer size, so the host has
 * to apply exactly the same rescaling used here before filtering, otherwise
 * the comparison between the two traces is not fair.
 *
 * A measurement is triggered by the character 't' on serial - use the MATLAB or 
 * Python script in matlab/ and python/ to trigger and plot the echo.
 */

#include "SensEdu.h"
#include "SineLUT.h"
#include "FilterTaps.h"

// Internal library error container
uint32_t lib_error = 0;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

/* DAC */
// LUT settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     (DAC_SINE_FREQ * sine_lut_size) // 64 samples per one sine cycle

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
const uint16_t mic_data_size = 2048;
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

/* DSP */
// Filter coefficients are in FilterTaps.h
#define FILTER_BLOCK_LENGTH 32      // Samples processed per internal filter call

static SENSEDU_DSP_FIR_STATE_BUFFER(fir_state_buffer, FILTER_TAP_NUM, FILTER_BLOCK_LENGTH);
SensEdu_DSP_FIR fir_filt;

SensEdu_DSP_FIR_Settings fir_settings = {
    .taps = filter_taps,
    .tap_num = FILTER_TAP_NUM,
    .state_buf = fir_state_buffer,
    .state_buf_size = SENSEDU_DSP_FIR_STATE_SIZE(FILTER_TAP_NUM, FILTER_BLOCK_LENGTH),
    .block_size = FILTER_BLOCK_LENGTH
};

static float rescaled_data[mic_data_size];
static float filtered_data[mic_data_size];

const uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);

    SensEdu_DAC_Init(&dac_settings);

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);

    // Filter is re-initialized later, this call is here only for early error checking.
    SensEdu_DSP_FIR_Init(&fir_filt, &fir_settings);

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    // Wait for the trigger character 't' from the host
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

    // Start the DAC -> ADC sequence
    SensEdu_DAC_Enable(dac_ch);
    while (!SensEdu_DAC_GetBurstCompleteFlag(dac_ch));
    SensEdu_DAC_ClearBurstCompleteFlag(dac_ch);
    SensEdu_ADC_Start(adc);
    
    // Wait for the data
    while (!SensEdu_ADC_IsDmaTransferComplete(adc));
    SensEdu_ADC_ClearDmaTransferComplete(adc);

    // Send the raw capture first, then the filtered result
    serial_send_array(&(mic_data[0]), mic_data_size, 32);
    rescale_adc_wave(rescaled_data, &(mic_data[0]), mic_data_size);
    filter_32kHz_wave(rescaled_data, filtered_data, mic_data_size);
    serial_send_float_array(filtered_data, mic_data_size, 32);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// Normalizes raw ADC counts from 0:65535 to -1:1, which also removes the DC offset
void rescale_adc_wave(float* rescaled_wave, uint16_t* adc_wave, const uint16_t data_length) {
    for (uint16_t i = 0; i < data_length; i++) {
        rescaled_wave[i] = (2.0f * adc_wave[i]) / 65535.0f - 1.0f;
    }
}

// Applies the 32 kHz bandpass. Each call starts from a clean history, since every
// measurement is an independent capture rather than a continuation of the previous one
void filter_32kHz_wave(float* input, float* output, const uint16_t data_length) {
    SensEdu_DSP_FIR_Init(&fir_filt, &fir_settings);
    SensEdu_DSP_FIR_Apply(&fir_filt, input, output, data_length);
}

// Checks if the library has raised any internal errors
// Serial is busy sending measurements, so the error LED is used instead
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
}

// Sends the buffer over serial in fixed-size chunks
void serial_send_array(uint16_t* data, const size_t data_length, const size_t chunk_size_byte) {
    const size_t total_byte_length = data_length * sizeof(uint16_t);
    for (size_t i = 0; i < total_byte_length; i += chunk_size_byte) {
        size_t transfer_size = (total_byte_length - i < chunk_size_byte) ? (total_byte_length - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}

// Sends the buffer over serial in fixed-size chunks
void serial_send_float_array(float* data, const size_t data_length, const size_t chunk_size_byte) {
    const size_t total_byte_length = data_length * sizeof(float);
    for (size_t i = 0; i < total_byte_length; i += chunk_size_byte) {
        size_t transfer_size = (total_byte_length - i < chunk_size_byte) ? (total_byte_length - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}
