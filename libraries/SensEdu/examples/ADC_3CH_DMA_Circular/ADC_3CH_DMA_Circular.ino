#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Error Indicator LED
static const uint8_t ERROR_LED_PIN = D86;

// Half-transfer chunk size (per channel)
// (not multiples of 32/64 to flush the USB chunk)
static const uint16_t CHUNK_SIZE = 75;

// ADC Settings
static ADC_TypeDef* adc = ADC1;
static const uint16_t SAMPLING_RATE_PER_CH = 44100;

static const uint16_t CHANNEL_NUM_PER_ADC = 3;
static uint8_t adc_pins[CHANNEL_NUM_PER_ADC] = {A0, A1, A2};

// DMA Settings
static const uint16_t DMA_BUFFER_SIZE = CHUNK_SIZE * 2 * CHANNEL_NUM_PER_ADC;
volatile SENSEDU_DMA_BUFFER(dma_buffer, DMA_BUFFER_SIZE);

// Config Structure
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = CHANNEL_NUM_PER_ADC,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = SAMPLING_RATE_PER_CH,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_CIRCULAR,
    .mem_address = (uint16_t*)dma_buffer,
    .mem_size = DMA_BUFFER_SIZE
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(2000000);

    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, HIGH);

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);

    check_lib_errors(ERROR_LED_PIN);
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

// Stream control from MATLAB host:
// 'S' = start/resume streaming, 'P' = pause streaming.
static bool stream_enabled = true;

void loop() {
    process_stream_control();

    if (!stream_enabled) {
        if (SensEdu_ADC_IsDmaHalfTransferComplete(adc)) {
            SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        }
        if (SensEdu_ADC_IsDmaTransferComplete(adc)) {
            SensEdu_ADC_ClearDmaTransferComplete(adc);
        }
        return;
    }

    if (SensEdu_ADC_IsDmaHalfTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        if (Serial) {
            transfer_buf(&dma_buffer[0], (DMA_BUFFER_SIZE / 2));
        }
    }

    if (SensEdu_ADC_IsDmaTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        if (Serial) {
            transfer_buf(&dma_buffer[DMA_BUFFER_SIZE / 2], (DMA_BUFFER_SIZE / 2));
        }
    }
}

// Transfers selected buffer in one write.
static void transfer_buf(volatile uint16_t* data, uint16_t data_length) {
    uint8_t* ptr = (uint8_t*)data;
    Serial.write(ptr, data_length * sizeof(uint16_t));
}

// Handles host stream control without blocking the DMA acquisition pipeline.
static void process_stream_control() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'S') {
            stream_enabled = true;
        } else if (c == 'P') {
            stream_enabled = false;
        }
    }
}

// Checks library error state
static void check_lib_errors(uint8_t error_led) {
    uint32_t lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        fatal_error(error_led);
    }
}

// Halts system on fatal error
static void fatal_error(uint8_t error_led) {
    digitalWrite(error_led, !digitalRead(error_led));
    delay(200);
}
