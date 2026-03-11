#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Error Indicator LED
static const uint8_t ERROR_LED_PIN = D86;

// EMG Decision Chunk Size
// (not multiples of 32/64 to flush the USB chunk)
static const uint16_t EMG_CHUNK_SIZE = 75;

// ADC Settings
static ADC_TypeDef* adc = ADC1;

static const uint16_t CHANNEL_NUM_PER_ADC = 4;
static uint8_t adc_pins[CHANNEL_NUM_PER_ADC] = {A0, A2, A11, A7};

static const uint16_t SAMPLING_RATE_PER_CH = 5000;
static const uint16_t ADC_SAMPLING_RATE = SAMPLING_RATE_PER_CH * CHANNEL_NUM_PER_ADC;

// DMA Settings
static const uint16_t DMA_BUFFER_SIZE = EMG_CHUNK_SIZE * 2 * CHANNEL_NUM_PER_ADC;
volatile SENSEDU_DMA_BUFFER(dma_buffer, DMA_BUFFER_SIZE);

// Config Structure
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = CHANNEL_NUM_PER_ADC,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 5000,
    
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

uint32_t transfers_remaining = 0;
bool recording_active = false;

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

static void transfer_buf(volatile uint16_t* data, uint16_t data_length) {
    uint8_t* ptr = (uint8_t*)data;
    Serial.write(ptr, data_length * 2);
}

// Check library error state
static void check_lib_errors(uint8_t error_led) {
    uint32_t lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        fatal_error(error_led);
    }
}

// Halt system on fatal error
static void fatal_error(uint8_t error_led) {
    digitalWrite(error_led, !digitalRead(error_led));
    delay(200);
}
