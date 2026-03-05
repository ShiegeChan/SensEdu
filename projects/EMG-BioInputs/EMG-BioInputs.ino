#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Error Indicator LED
static const uint8_t ERROR_LED_PIN = D86;

// EMG Chunk Configuration
static const uint16_t EMG_CHUNK_NUM = 3;
static const uint16_t EMG_CHUNK_SIZE = EMG_CHUNK_NUM * (64 / sizeof(uint16_t));

// ADC Settings
static ADC_TypeDef* adc = ADC1;

static const uint16_t CHANNEL_NUM_PER_ADC = 4;
static uint8_t adc_pins[CHANNEL_NUM_PER_ADC] = {A0, A2, A11, A7};

static const uint16_t SAMPLING_RATE_PER_CH = 5000;
static const uint16_t ADC_SAMPLING_RATE = SAMPLING_RATE_PER_CH * CHANNEL_NUM_PER_ADC;

// DMA Settings
static const uint16_t DMA_BUFFER_SIZE = 64;
volatile SENSEDU_DMA_BUFFER(dma_buffer, DMA_BUFFER_SIZE);
static const uint16_t ITERATIONS_PER_REQUEST = (EMG_CHUNK_SIZE * CHANNEL_NUM_PER_ADC) / (DMA_BUFFER_SIZE / 2);

// Config Structure
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = CHANNEL_NUM_PER_ADC,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = ADC_SAMPLING_RATE,
    
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
    if (!recording_active && Serial.available() > 0) {
        char command = Serial.read();
        if (command == 't') {
            transfers_remaining = ITERATIONS_PER_REQUEST;
            recording_active = true;

            // Clear DMA status flags for synchronization
            SensEdu_ADC_ClearDmaTransferComplete(adc);
            SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        }
    }

    if (!recording_active) return;

    if (transfers_remaining > 0 && SensEdu_ADC_IsDmaHalfTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        transfer_64byte_buf(&dma_buffer[0]);
        transfers_remaining--;
    }

    if (transfers_remaining > 0 && SensEdu_ADC_IsDmaTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        transfer_64byte_buf(&dma_buffer[DMA_BUFFER_SIZE / 2]);
        transfers_remaining--;
    }

    if (transfers_remaining == 0) {
        // Send dummy byte for USB to issue the last stuck packet in some edge alignment cases
        Serial.write((uint8_t)0x00);
        recording_active = false;
    }
}

static void transfer_64byte_buf(volatile uint16_t* data) {
    Serial.write((uint8_t*)data, 64);
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
