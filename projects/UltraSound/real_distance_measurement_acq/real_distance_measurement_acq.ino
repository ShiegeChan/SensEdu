// Works on new versions of libraries:
// 1. Arduino Giga 4.2.1
#include "SensEdu.h"
#include "CMSIS_DSP.h"
#include "SineLUT.h"
#include "FilterTaps.h"
#include "DACWave.h" // contains wave and its size

uint32_t lib_error = 0;
uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
//#define SERIAL_OFF

#define BAN_DISTANCE    20	    // min distance [cm] - how many self reflections cancelled
#define ACTUAL_SAMPLING_RATE 250000 // You need to measure this value using a wave generator with a fixed e.g. 1kHz Sine
#define STORE_BUF_SIZE  32 * 32     // 2400 for 1 measurement per second. 
                            	    // only multiples of 32!!!!!! (64 chunk size of bytes, so 32 for 16bit)


/*************************FILTER *****************/
#define FILTER_BLOCK_LENGTH     32      // how many samples we want to process every time we call the fir process function AT
#define FILTER_TAP_NUM          32      // tap number for the bandpass filter

static float32_t firStateBuffer[FILTER_BLOCK_LENGTH + FILTER_TAP_NUM - 1];   // current filter state buffer
arm_fir_instance_f32 Fir_filt;  // creating an object instance

/********************************** ADC **************/

ADC_TypeDef* adc1 = ADC1;
ADC_TypeDef* adc2 = ADC2;
ADC_TypeDef* adc3 = ADC3;

const uint8_t adc1_mic_num = 3; // 3 microphones for adc1
const uint8_t adc2_mic_num = 2; // 3 microphones for adc2
const uint8_t adc3_mic_num = 3; // 2 microphones for adc3

uint8_t adc1_pins[adc1_mic_num] = {A1, A3, A4}; // mic1, mic2, mic3 are on adc1
uint8_t adc2_pins[adc2_mic_num] = {A5, A10}; // mic4 and mic8 are on adc2
uint8_t adc3_pins[adc3_mic_num] = {A6, A8, A9}; // mic6, mic5, and mic7 are on adc3

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t adc1_data_size = STORE_BUF_SIZE * adc1_mic_num; // 2048 * 2 = 64 * 32 * 2
const uint16_t adc2_data_size = STORE_BUF_SIZE * adc2_mic_num;
const uint16_t adc3_data_size = STORE_BUF_SIZE * adc3_mic_num;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc1_data[adc1_data_size];
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc2_data[adc2_data_size];
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc3_data[adc3_data_size];


SensEdu_ADC_Settings adc1_settings = {
    .adc = adc1,
    .pins = adc1_pins,
    .pin_num = adc1_mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)adc1_data,
    .mem_size = adc1_data_size
};

SensEdu_ADC_Settings adc2_settings = {
    .adc = adc2,
    .pins = adc2_pins,
    .pin_num = adc2_mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)&adc2_data,
    .mem_size = adc2_data_size
};

SensEdu_ADC_Settings adc3_settings = {
    .adc = adc3,
    .pins = adc3_pins,
    .pin_num = adc3_mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)&adc3_data,
    .mem_size = adc3_data_size
};

/********************* DAC ****************/
DAC_Channel* dac_channel = DAC_CH2;
// lut settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

SensEdu_DAC_Settings dac_settings = {dac_channel, DAC_SAMPLE_RATE, (uint16_t*)sine_lut, sine_lut_size, 
    SENSEDU_DAC_MODE_BURST_WAVE, dac_cycle_num}; // specifying burst mode 

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

const uint16_t air_speed = 343; // m/s

// e.g. 25cm ban means 0.25*2/343 time ban, then multiply by sample rate
const uint32_t banned_sample_num = ((BAN_DISTANCE*2*ACTUAL_SAMPLING_RATE)/air_speed)/100; 

/* -------------------------------------------------------------------------- */
/*                              Global Structure                              */
/* -------------------------------------------------------------------------- */
typedef struct {
	uint8_t ban_flag; // activate self reflections ban
	char serial_read_buf;
    float xcorr_buffer[STORE_BUF_SIZE]; // use same buffer for several functions for memory saving
    
} SenseduBoard;

static SenseduBoard SenseduBoardObj;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
	main_obj_init(main_obj_ptr);

    // initializing the filter
    arm_fir_init_f32(&Fir_filt, FILTER_TAP_NUM, filter_taps, firStateBuffer, FILTER_BLOCK_LENGTH); 
    
    //SensEdu_ADC_ShortA4toA9(); // in order to use ADC1 for mic2

    Serial.begin(115200); // 14400 bytes/sec -> 7200 samples/sec -> 2400 samples/sec for 1 mic

    /* DAC */
    SensEdu_DAC_Init(&dac_settings);
    
    /* ADC 1 */
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(adc1);

    /* ADC 2 */
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc2);

    /* ADC 3 */
    SensEdu_ADC_Init(&adc3_settings);
    SensEdu_ADC_Enable(adc3);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
    // Measurement is initiated by the signal from computing device (matlab script)
    static char serial_buf = 0;
    
    // Measurement is initiated by signal from computing device
	#ifndef SERIAL_OFF
		while (1) {
			while (Serial.available() == 0);    // Wait for a signal
			serial_buf = Serial.read();
			if (serial_buf == 't') {
                break; 
			}
		}
	#endif
    
    // Start dac->adc sequence
    SensEdu_DAC_Enable(dac_channel);
    while(!SensEdu_DAC_GetBurstCompleteFlag(dac_channel)); // wait for dac to finish sending the burst
    SensEdu_DAC_ClearBurstCompleteFlag(dac_channel); 
    
    // Start ADCs
    SensEdu_ADC_Start(adc1);
    SensEdu_ADC_Start(adc2);
    SensEdu_ADC_Start(adc3);

    // Wait for the data from ADC1
    while(!SensEdu_ADC_GetTransferStatus(adc1));
    SensEdu_ADC_ClearTransferStatus(adc1);

    // Wait for the data from ADC2
    while(!SensEdu_ADC_GetTransferStatus(adc2));
    SensEdu_ADC_ClearTransferStatus(adc2);

    // Wait for the data from ADC2
    while(!SensEdu_ADC_GetTransferStatus(adc3));
    SensEdu_ADC_ClearTransferStatus(adc3);

    // Calculating distance for each microphone
    static uint32_t distance[6];
	distance[0] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc1_data, sizeof(adc1_data), "1", 3, main_obj_ptr->ban_flag);
    distance[1] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc1_data, sizeof(adc1_data), "2", 3, main_obj_ptr->ban_flag);
    distance[2] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc1_data, sizeof(adc1_data), "3", 3, main_obj_ptr->ban_flag);

    distance[3] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc2_data, sizeof(adc2_data), "1", 2, main_obj_ptr->ban_flag);
	distance[7] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc2_data, sizeof(adc2_data), "2", 2, main_obj_ptr->ban_flag);

    distance[5] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc3_data, sizeof(adc3_data), "1", 3, main_obj_ptr->ban_flag);
    distance[4] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc3_data, sizeof(adc3_data), "2", 3, main_obj_ptr->ban_flag);
    distance[6] = get_distance_measurement(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc3_data, sizeof(adc3_data), "3", 3, main_obj_ptr->ban_flag);

    // Sending the distance measurements
    
    Serial.write((const uint8_t *) &distance[0], 4);
    Serial.write((const uint8_t *) &distance[1], 4);
    Serial.write((const uint8_t *) &distance[2], 4);
    Serial.write((const uint8_t *) &distance[3], 4);
    Serial.write((const uint8_t *) &distance[7], 4);
    Serial.write((const uint8_t *) &distance[5], 4);
    Serial.write((const uint8_t *) &distance[4], 4);
    Serial.write((const uint8_t *) &distance[6], 4);

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}