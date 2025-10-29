#include "SensEdu.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
#define ADC1_SR  250000     // ADC1 sampling rate
#define ADC2_SR  100000     // ADC2 sampling rate
#define ADC3_SR  65000      // ADC3 sampling rate

/* ADC */
const uint16_t buf_size = 16*128; // must be multiple of 16 for 16bit
SENSEDU_ADC_BUFFER(buf1, buf_size);
SENSEDU_ADC_BUFFER(buf2, buf_size);
SENSEDU_ADC_BUFFER(buf3, buf_size);

ADC_TypeDef* adc1 = ADC1;
const uint8_t adc1_pin_num = 1;
uint8_t adc1_pins[adc1_pin_num] = {A0};
SensEdu_ADC_Settings adc1_settings = {
    .adc = adc1,
    .pins = adc1_pins,
    .pin_num = adc1_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = ADC1_SR,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)buf1,
    .mem_size = buf_size
};

ADC_TypeDef* adc2 = ADC2;
const uint8_t adc2_pin_num = 1;
uint8_t adc2_pins[adc2_pin_num] = {A1};
SensEdu_ADC_Settings adc2_settings = {
    .adc = adc2,
    .pins = adc2_pins,
    .pin_num = adc2_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = ADC2_SR,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)buf2,
    .mem_size = buf_size
};

ADC_TypeDef* adc3 = ADC3;
const uint8_t adc3_pin_num = 1;
uint8_t adc3_pins[adc3_pin_num] = {A6};
SensEdu_ADC_Settings adc3_settings = {
    .adc = adc3,
    .pins = adc3_pins,
    .pin_num = adc3_pin_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = ADC3_SR,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)buf3,
    .mem_size = buf_size
};

/* errors */
uint32_t lib_error = 0;
uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);

    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(adc1);
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc2);
    SensEdu_ADC_Init(&adc3_settings);
    SensEdu_ADC_Enable(adc3);

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }

}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
  
    // Measurement is initiated by the signal from computing device
    static char serial_buf = 0;
    
    while (1) {
        while (Serial.available() == 0); // Wait for a signal
        serial_buf = Serial.read();

        if (serial_buf == 't') {
            // expected 't' symbol (trigger)
            break;
        }
    }

    SensEdu_ADC_Start(adc1);
    SensEdu_ADC_Start(adc2);
    SensEdu_ADC_Start(adc3);
    
    // wait for the data and send it
    while(!SensEdu_ADC_GetTransferStatus(adc1));
    SensEdu_ADC_ClearTransferStatus(adc1);

    while(!SensEdu_ADC_GetTransferStatus(adc2));
    SensEdu_ADC_ClearTransferStatus(adc2);

    while(!SensEdu_ADC_GetTransferStatus(adc3));
    SensEdu_ADC_ClearTransferStatus(adc3);
    
    serial_send_array((const uint8_t *) & buf1, buf_size << 1);
    serial_send_array((const uint8_t *) & buf2, buf_size << 1);
    serial_send_array((const uint8_t *) & buf3, buf_size << 1);

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */
void handle_error() {
    // serial is taken by matlab, use LED as indication
    digitalWrite(error_led, LOW);
}

// send serial data in 32 byte chunks
void serial_send_array(const uint8_t* data, size_t size) {
    const size_t chunk_size = 32;
	for (uint32_t i = 0; i < size/chunk_size; i++) {
		Serial.write(data + chunk_size * i, chunk_size);
	}
}