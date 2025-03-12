#include <SensEdu.h>
//IMPORTANT : This FMCW distance measurement example is meant to be used with 2 arduino boards : One transmitting the chirp, one receiving.
//This script should be used for the receiving Arduino.
//Use the Chirp_SawtoothMod script on the transmitting Arduino.
//In this example, DAC1 of the transmitting Arduino has to be connected to the A7 pin on the receiving arduino to acquire the transmitted signal.  


/* -------------------------------------------------------------------------- */
/*                                 Settings                                   */
/* -------------------------------------------------------------------------- */

// ADC Sampling
const uint16_t mic_data_size = 16384; // ADC buffer size, must be a multiple of 16
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc_dac_data[mic_data_size]; // cache aligned
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc_mic_data[mic_data_size]; // cache aligned

// ADC-DMA Hardware Settings
ADC_TypeDef* adc_dac = ADC1;
ADC_TypeDef* adc_mic = ADC2;
const uint8_t mic_num = 1;
uint8_t adc_dac_pins[mic_num] = {A7};
uint8_t adc_mic2_pins[mic_num] = {A4};

SensEdu_ADC_Settings adc1_settings = {
    .adc = adc_dac,
    .pins = adc_dac_pins,
    .pin_num = mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)adc_dac_data,
    .mem_size = mic_data_size
};

SensEdu_ADC_Settings adc2_settings = {
    .adc = adc_mic,
    .pins = adc_mic2_pins,
    .pin_num = mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)adc_mic_data,
    .mem_size = mic_data_size
};


// Error Handling
uint8_t error_led = D86; // Error indicator LED pin
uint32_t lib_error = 0;  // Tracks library errors
bool dac_data_sent = false; // To track whether DAC LUT was sent to MATLAB

/* -------------------------------------------------------------------------- */
/*                              Functions                                     */
/* -------------------------------------------------------------------------- */

// Function to send an array over Serial in 32-byte chunks
void serial_send_array(const uint8_t* data, size_t size) {
    const size_t chunk_size = 32;
    for (size_t i = 0; i < size / chunk_size; i++) {
        Serial.write(data + chunk_size * i, chunk_size);
    }
}

// Function to handle errors
void handle_error() {
    digitalWrite(error_led, LOW); // Turn on error LED
    while (1) {
        // Remain in this state if an error occurs
    }
}

/* -------------------------------------------------------------------------- */
/*                                   Setup                                    */
/* -------------------------------------------------------------------------- */
void setup() {
    // Initialize Serial Communication
    Serial.begin(115200);
    SensEdu_ADC_ShortA4toA9();

    // Initialize ADC
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc_dac);
    SensEdu_ADC_Enable(adc_mic);

    // Setup Error LED
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH); // Turn off (active low)

    // Check for errors
    lib_error = SensEdu_GetError();
    if (lib_error != 0) {
        handle_error();
    }
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    static char serial_buf = 0;

    // Wait for trigger command ('t') from MATLAB
    while (1) {
        while (Serial.available() == 0);
        serial_buf = Serial.read();

        if (serial_buf == 't') { // First trigger detected
            break;
        }
    }
    
    // Start ADC Data Acquisition
    SensEdu_ADC_Start(adc_dac);
    SensEdu_ADC_Start(adc_mic);

    // wait for the data and send it
    while(!SensEdu_ADC_GetTransferStatus(adc_dac));
    SensEdu_ADC_ClearTransferStatus(adc_dac);

    while(!SensEdu_ADC_GetTransferStatus(adc_mic));
    SensEdu_ADC_ClearTransferStatus(adc_mic);


    // Send ADC data (16-bit values, continuously)
    uint32_t adc_byte_length = mic_data_size * 2; // ADC data size in bytes
    Serial.write((uint8_t*)&adc_byte_length, sizeof(adc_byte_length));  // Send size header
    serial_send_array((const uint8_t*)adc_dac_data, adc_byte_length);       // Transmit ADC1 data
    serial_send_array((const uint8_t*)adc_mic_data, adc_byte_length);       // Transmit ADC2 data (Mic2 data)

    // Check for errors during the process
    lib_error = SensEdu_GetError();
    if (lib_error != 0) {
        handle_error();
    }
}
