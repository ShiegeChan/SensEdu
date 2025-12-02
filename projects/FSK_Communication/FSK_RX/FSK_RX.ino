#include "SensEdu.h"

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;

// Configure the ADC 
const uint16_t mic_data_size = 2048;
SENSEDU_ADC_BUFFER(mic_data, mic_data_size);

ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = {A1};
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = mic_pins,
    .pin_num = mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)mic_data,
    .mem_size = mic_data_size
};

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");

    //Led in red if there is any problem
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    //Initializing DAC
    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
}

void loop () {
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

    SensEdu_ADC_Start(adc);
    // wait for the data and send it
    while(!SensEdu_ADC_GetTransferStatus(adc));
    SensEdu_ADC_ClearTransferStatus(adc);
    serial_send_array((const uint8_t *) &mic_data, mic_data_size << 1);

    check_errors();
}

// Checking errors of the library
void check_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
        Serial.println(lib_error, HEX);
    }
}

// send serial data in 32 byte chunks
void serial_send_array(const uint8_t* data, size_t size) {
    const size_t chunk_size = 32;
	for (uint32_t i = 0; i < size/chunk_size; i++) {
		Serial.write(data + chunk_size * i, chunk_size);
	}
}
