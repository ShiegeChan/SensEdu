/*
 * EMG BioInputs
 *
 * Continuous 4-channel surface EMG acquisition on a single ADC with circular DMA, 
 * streamed over USB CDC to a MATLAB host for signal processing and
 * keyboard-press decisions.
 */

#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Per-channel sampling rate (Hz).
static const uint16_t SAMPLING_RATE_PER_CH = 5000;

// EMG decision chunk size (samples per channel).
// Chosen so the USB payload is not a multiple of 64 (USB-FS max packet size),
// which forces a short packet to flush each chunk.
static const uint16_t EMG_CHUNK_SIZE = 75;

/* -------------------------------------------------------------------------- */
/*                                  Globals                                   */
/* -------------------------------------------------------------------------- */

// On-board Arduino LED (active LOW).
static const uint8_t ERROR_LED_PIN = D86;

static ADC_TypeDef* adc = ADC1;
static const uint16_t CHANNEL_NUM_PER_ADC = 4;
static uint8_t adc_pins[CHANNEL_NUM_PER_ADC] = {A5, A4, A10, A11};

// Ping-pong DMA buffer: half-transfer holds one EMG chunk per channel.
static const uint16_t DMA_BUFFER_SIZE = EMG_CHUNK_SIZE * 2 * CHANNEL_NUM_PER_ADC;
volatile SENSEDU_DMA_BUFFER(dma_buffer, DMA_BUFFER_SIZE);

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
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static void transfer_buf(volatile uint16_t* data, uint16_t data_length);
static void check_lib_errors();
static void fatal_error();

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(2000000);  // Baud is cosmetic for USB CDC

    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, HIGH);

    SensEdu_ADC_ShortA4A9();
    SensEdu_ADC_ShortA5A8();

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);

    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    if (SensEdu_ADC_IsDmaHalfTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        transfer_buf(&dma_buffer[0], (DMA_BUFFER_SIZE / 2));
    }

    if (SensEdu_ADC_IsDmaTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        transfer_buf(&dma_buffer[DMA_BUFFER_SIZE / 2], (DMA_BUFFER_SIZE / 2));
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// Streams one DMA half over USB CDC as raw 16-bit samples.
static void transfer_buf(volatile uint16_t* data, uint16_t data_length) {
    uint8_t* ptr = (uint8_t*)data;
    Serial.write(ptr, data_length * sizeof(uint16_t));
}

// Halts on any reported SensEdu library error.
static void check_lib_errors() {
    if (SensEdu_GetError() != 0) {
        fatal_error();
    }
}

// Hard halt: blinks the error LED at 2.5 Hz forever.
static void fatal_error() {
    while (true) {
        digitalWrite(ERROR_LED_PIN, !digitalRead(ERROR_LED_PIN));
        delay(200);
    }
}
