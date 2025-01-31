#include "SensEdu.h"
#include "SineLUT.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

/* DAC */
// lut settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle
#define STORE_BUF_SIZE      64*32
SensEdu_DAC_Settings dac1_settings = {DAC1, DAC_SAMPLE_RATE, (uint16_t*)sine_lut, sine_lut_size, 
    SENSEDU_DAC_MODE_BURST_WAVE, dac_cycle_num};


/* ADC */
const uint16_t mic_data_size = 64*32*2; // must be multiple of 64 for 16bit
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t mic_data[mic_data_size]; // cache aligned

ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 2;
uint8_t mic_pins[mic_num] = {A1, A6};
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

/* errors */
uint32_t lib_error = 0;
uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {

    Serial.begin(115200);

    SensEdu_DAC_Init(&dac1_settings);

    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);

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

    // start dac->adc sequence
    SensEdu_DAC_Enable(DAC1);
    while(!SensEdu_DAC_GetBurstCompleteFlag());
    SensEdu_DAC_ClearBurstCompleteFlag();
    SensEdu_ADC_Start(adc);
    
    // wait for the data and send it
    while(!SensEdu_DMA_GetADCTransferStatus(ADC1));
    SensEdu_DMA_ClearADCTransferStatus(ADC1);
    serial_send_array((const uint8_t *)mic_data, sizeof(mic_data), "1");
    serial_send_array((const uint8_t *)mic_data, sizeof(mic_data), "2");
    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
    delay(100);
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */
void handle_error() {
    // serial is taken by matlab, use LED as indication
    digitalWrite(error_led, LOW);
}

// make sure the buffer is initialized because it crashes if you access an array element w/o initialization
void clear_8bit_buf(uint8_t array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0x00;
    }
}

// send serial data in 32 byte chunks
void serial_send_array(const uint8_t* data, size_t size, const char* channel) {
    const size_t chunk_size = 32; // buffer is 32 bytes, but 32 for 2400 data samples
    if (channel[0] == '1') {
        // first extract the data 
        uint8_t ch1[2*STORE_BUF_SIZE]; 
        // initialize the buffer
        clear_8bit_buf(ch1, 2*STORE_BUF_SIZE);
        uint16_t cnt = 0;
        for(uint16_t i = 0; i < size; i+=4) {
            ch1[cnt++] = data[i];
            ch1[cnt++] = data[i+1];
        }
        // send the data in chunks of 32
        size /= 2;
        for (uint16_t i = 0; i < size/chunk_size; i++) {
            Serial.write(ch1 + chunk_size * i, chunk_size);
        }
        return;
    }
    else if(channel[0] == '2') {
        // first extract the data 
        uint16_t cnt = 0;
        uint8_t ch2[2*STORE_BUF_SIZE]; 
        clear_8bit_buf(ch2, 2*STORE_BUF_SIZE);
        for(uint16_t i = 0; i < size; i+=4) {
            ch2[cnt++] = data[i+2];
            ch2[cnt++] = data[i+3];
        }
        // send the data in chunks of 32
        size /= 2;
        for (uint16_t i = 0; i < size/chunk_size; i++) {
            Serial.write(ch2 + chunk_size * i, chunk_size);
        }
        return;
    }
    else {
        // normal data send
        for (uint16_t i = 0; i < size/chunk_size; i++) {
            Serial.write(data + chunk_size * i, chunk_size);
        }
        return;
    }   
}
