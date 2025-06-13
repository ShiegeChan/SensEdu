#include "SensEdu.h"

static uint16_t lib_error = 0x0000;     // lib error container
static uint32_t error = 0x0000FFFF;     // error detector (0000 to FFFF instantly)

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
ADC_TypeDef* adc = ADC1;
const uint16_t channel_count = 4;
uint8_t adc_pins[channel_count] = {A0, A2, A11, A7};
const uint16_t sampling_rate = 25600;
// must be:
// 1. multiple of 32 bytes to ensure cache coherence
// 2. properly aligned
const uint16_t mem_size = 16 * channel_count * 64; // multiple of 16 for 2 byte values
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t emg_data[mem_size];

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = channel_count,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = sampling_rate,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)emg_data,
    .mem_size = mem_size
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    // doesn't boot without opened serial monitor
    Serial.begin(115200);
    while (!Serial) {
        delay(1);
    }

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    // loop is triggered with character sent from PC
    static char serial_buf = 0;
    while (1) {
        while (Serial.available() == 0); // Wait for a signal
        serial_buf = Serial.read();
        if (serial_buf == 'c') {
            // config character
            send_config(&mem_size, &channel_count, &sampling_rate);
            return;
        }
        if (serial_buf == 'm') {
            // measurement character
            break;
        }
    }

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        // Send error marker and then error code
        Serial.write((const uint8_t*) &error, 4);
        for (uint16_t i = 0; i < (mem_size - 2); i++) {
            Serial.write((const uint8_t*) &lib_error, 2);
        }
        delay(1000);
    }

    // wait till ADC is ready
    while(!SensEdu_ADC_GetTransferStatus(adc));

    // send data
    transfer_serial_data(&(emg_data[0]), mem_size, 64);

    // restart ADC
    SensEdu_ADC_ClearTransferStatus(adc);
    SensEdu_ADC_Start(adc);
    
}

void send_config(const uint16_t* mem_size, const uint16_t* channel_count, const uint16_t* sampling_rate) {
    Serial.write((const uint8_t*) mem_size, 2);
    Serial.write((const uint8_t*) channel_count, 2);
    Serial.write((const uint8_t*) sampling_rate, 2);
}

void transfer_serial_data(uint16_t* data, const uint16_t data_length, const uint16_t chunk_size_byte) {
    for (uint16_t i = 0; i < (data_length*2); i += chunk_size_byte) {
        uint16_t transfer_size = ((data_length*2) - i < chunk_size_byte) ? (data_length*2 - i) : chunk_size_byte;
        Serial.write((const uint8_t *) data + i, transfer_size);
    }
}
