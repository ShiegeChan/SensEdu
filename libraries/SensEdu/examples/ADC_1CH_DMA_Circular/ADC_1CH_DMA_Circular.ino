#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Number of half-buffer transfers per host request
// Must match the MATLAB-side configuration
static const uint16_t ITERATIONS_PER_REQUEST = 200; 

// Error indicator LED
static const uint8_t ERROR_LED_PIN = D86;

// DMA buffer size (must be divisible by 2 for ping-pong operation)
static const uint16_t DMA_BUFFER_SIZE = 64;
volatile SENSEDU_DMA_BUFFER(dma_buffer, DMA_BUFFER_SIZE);

static ADC_TypeDef* adc = ADC1;
static const uint8_t ADC_PIN_COUNT = 1;
static uint8_t adc_pins[ADC_PIN_COUNT] = {A1};

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = ADC_PIN_COUNT,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 44100,
    
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

    while (!is_even(DMA_BUFFER_SIZE)) {
        fatal_error(ERROR_LED_PIN);
    }

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
    if (Serial.available() > 0) {
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
        serial_send_array(&dma_buffer[0], DMA_BUFFER_SIZE / 2, 64);
        transfers_remaining--;
    }

    if (transfers_remaining > 0 && SensEdu_ADC_IsDmaTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        serial_send_array(&(dma_buffer[DMA_BUFFER_SIZE / 2]), DMA_BUFFER_SIZE / 2, 64);
        transfers_remaining--;
    }

    if (transfers_remaining == 0) {
        // Send dummy byte for USB to issue the last stuck packet in some edge alignment cases
        Serial.write((uint8_t)0x00);
        recording_active = false;
    }
}

// Ensures buffer size is valid for ping-pong DMA
static bool is_even(uint16_t size) {
    if (size % 2) return false;
    return true;
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

// Send 16-bit buffer over Serial in byte chunks
static void serial_send_array(volatile uint16_t* data, const size_t data_length, const size_t chunk_size_byte) {
    for (size_t i = 0; i < (data_length << 1); i += chunk_size_byte) {
        size_t transfer_size = ((data_length << 1) - i < chunk_size_byte) ? ((data_length << 1) - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}
