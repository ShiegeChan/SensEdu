#include "SensEdu.h"

static uint32_t lib_error = 0; // lib error container
static uint32_t error = 0x00FF; // error detector (00 to FF instantly)

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
ADC_TypeDef* adc = ADC1;
const uint8_t channel_count = 4;
uint8_t adc_pins[channel_count] = {A0, A2, A11, A7}; 
// must be:
// 1. multiple of 32 bytes to ensure cache coherence
// 2. properly aligned
const uint16_t mem_size = channel_count*128; 
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t emg_data[mem_size];

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = channel_count,

    .conv_mode = SENSEDU_ADC_MODE_CONT,
    .sampling_freq = 0,
    
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

    // Send error "0" and then error code
    // if smth is wrong in a lib
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        Serial.write((const uint8_t*) &error, 4);
        Serial.write((const uint8_t*) &lib_error, 4);
        delay(1000);
    }

    // Send total memory size
    Serial.write((const uint8_t*) &mem_size, 2);

    // Send channel count
    Serial.write((const uint8_t*) &channel_count, 1);
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    if (SensEdu_ADC_GetTransferStatus(adc)) {
        // iterate through all 4 channels
        for (uint8_t i = 0; i < channel_count; i++) {
            // rearrange data properly
            for (uint16_t j = 0; j < mem_size/channel_count; j++) {
                Serial.write((const uint8_t *) &emg_data[i + channel_count*j], 2);
            }
        }

        // restart ADC
        SensEdu_ADC_ClearTransferStatus(adc);
        SensEdu_ADC_Start(adc);
    }

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        Serial.write((const uint8_t*) &error, 4);
        Serial.write((const uint8_t*) &lib_error, 4);
        delay(1000);
    }
}
