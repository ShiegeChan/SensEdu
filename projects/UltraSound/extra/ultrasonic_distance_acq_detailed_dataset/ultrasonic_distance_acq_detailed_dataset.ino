#include "SensEdu.h"
#include "CMSIS_DSP.h"
#include "SineLUT.h"
#include "FilterTaps.h"
#include "DACWave.h" // contains wave and its size

uint32_t lib_error = 0;
uint8_t error_led = D86;

#define AIR_SPEED               343
#define AIR_SPEED_MM_S          (AIR_SPEED * 1000UL)
#define AIR_SPEED_UM_S          (AIR_SPEED * 1000000UL)
#define HALF_AIR_SPEED_MM_S     (AIR_SPEED_MM_S / 2U)
#define HALF_AIR_SPEED_UM_S     (AIR_SPEED_UM_S / 2U)

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
#define IS_TRANSMIT_DETAILED_DATA   true    // activate full raw, filtered, xcorr data transmission
#define BAN_DISTANCE	            20	    // min distance [cm] - how many self reflections cancelled
#define ACTUAL_SAMPLING_RATE        250000  // You need to measure this value using a wave generator with a fixed e.g. 1kHz Sine
#define STORE_BUF_SIZE              32 * 32 // 2400 for 1 measurement per second. 
                            	        // only multiples of 32!!!!!! (64 chunk size of bytes, so 32 for 16bit)

/* --------------------------------- Filter --------------------------------- */
#define FILTER_BLOCK_LENGTH     32      // how many samples we want to process every time we call the fir process function AT
#define FILTER_TAP_NUM          32      // tap number for the bandpass filter

static float32_t firStateBuffer[FILTER_BLOCK_LENGTH + FILTER_TAP_NUM - 1]; // current filter state buffer
arm_fir_instance_f32 Fir_filt; // creating an object instance

/* ----------------------------------- ADC ---------------------------------- */
ADC_TypeDef* adc1 = ADC1;
ADC_TypeDef* adc2 = ADC2;

const uint8_t adc1_mic_num = 2; // 2 microphones for adc1
const uint8_t adc2_mic_num = 2; // 2 microphones for adc2

uint8_t adc1_pins[adc1_mic_num] = {A5, A10}; // mic1 and mic2
uint8_t adc2_pins[adc2_mic_num] = {A1, A6}; // mic3 and mic4 

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t adc1_data_size = STORE_BUF_SIZE * adc1_mic_num;
const uint16_t adc2_data_size = STORE_BUF_SIZE * adc2_mic_num;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc1_data[adc1_data_size];
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc2_data[adc2_data_size];

SensEdu_ADC_Settings adc1_settings = {
    .adc = adc1,
    .pins = adc1_pins,
    .pin_num = adc1_mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)adc1_data,
    .mem_size = adc1_data_size
};

SensEdu_ADC_Settings adc2_settings = {
    .adc = adc2,
    .pins = adc2_pins,
    .pin_num = adc2_mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)&adc2_data,
    .mem_size = adc2_data_size
};

/* ----------------------------------- DAC ---------------------------------- */
DAC_Channel* dac_channel = DAC_CH1;
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
const uint32_t c_banned_sample_num = ((BAN_DISTANCE*2*ACTUAL_SAMPLING_RATE)/air_speed)/100; 

/* -------------------------------------------------------------------------- */
/*                              Global Structure                              */
/* -------------------------------------------------------------------------- */
typedef struct {
	uint8_t ban_flag; // activate self reflections ban
	char serial_read_buf;
    float processing_buffer[STORE_BUF_SIZE]; // reusable float buffer for signal processing
    uint16_t channel_buffer[STORE_BUF_SIZE]; // data rearrangement
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

    Serial.begin(115200);

    /* DAC */
    SensEdu_DAC_Init(&dac_settings);
    
    /* ADC 1 */
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(adc1);

    /* ADC 2 */
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc2);

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
    while (1) {
        while (Serial.available() == 0); // Wait for a signal
        serial_buf = Serial.read();
        if (serial_buf == 't') {
            break; 
        }
    }
    
    // Start dac->adc sequence
    SensEdu_DAC_Enable(dac_channel);
    while (!SensEdu_DAC_GetBurstCompleteFlag(dac_channel)); // wait for dac to finish sending the burst
    SensEdu_DAC_ClearBurstCompleteFlag(dac_channel); 
    
    // Start ADCs
    SensEdu_ADC_Start(adc1);
    SensEdu_ADC_Start(adc2);

    // Wait for the data from ADC1
    while (!SensEdu_ADC_IsDmaTransferComplete(adc1));
    SensEdu_ADC_ClearDmaTransferComplete(adc1);

    // Wait for the data from ADC2
    while (!SensEdu_ADC_IsDmaTransferComplete(adc2));
    SensEdu_ADC_ClearDmaTransferComplete(adc2);

    // Calculating distance for each microphone
    static uint32_t distance[adc1_mic_num + adc2_mic_num];
    for (uint8_t i = 0; i < adc1_mic_num; i++) {
        get_channel_data(adc1_data, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, adc1_mic_num, i);
        process_and_transmit_data(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, main_obj_ptr->ban_flag, IS_TRANSMIT_DETAILED_DATA);
        distance[i] = calculate_distance(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, ACTUAL_SAMPLING_RATE);
    }
    for (uint8_t i = 0; i < adc2_mic_num; i++) {
        get_channel_data(adc2_data, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, adc2_mic_num, i);
        process_and_transmit_data(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, main_obj_ptr->ban_flag, IS_TRANSMIT_DETAILED_DATA);
        distance[adc1_mic_num + i] = calculate_distance(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, ACTUAL_SAMPLING_RATE);
    }

    // Sending the distance measurements
    for (uint8_t i = 0; i < (adc1_mic_num + adc2_mic_num); i++) {
        Serial.write((const uint8_t *) &distance[i], 4);
    }

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

void process_and_transmit_data(float* buf, const uint16_t buf_size, uint16_t* ch_array, const uint16_t ch_array_size, uint8_t ban_flag, uint8_t is_detailed_transmission) {

    /* -------------------------------- RAW DATA -------------------------------- */
    if (is_detailed_transmission)
        transfer_serial_data(ch_array, ch_array_size, 32);
    
    /* --------------------- RESCALED, FILTERED, NO COUPLED --------------------- */
    // Rescale from [0, (2^16-1)] to [-1, 1] and filter around 32 kHz
    clear_float_buf(buf, buf_size);
    rescale_adc_wave(buf, ch_array, ch_array_size);
    filter_32kHz_wave(buf, buf_size);
    if (ban_flag == 1) {
        remove_coupling(buf, c_banned_sample_num);
    } 
    if (is_detailed_transmission)
        transfer_serial_data_float(buf, buf_size, 32);

    /* ---------------------------------- XCORR --------------------------------- */
	custom_xcorr(buf, dac_wave, buf_size);
    if (is_detailed_transmission)
	    transfer_serial_data_float(buf, buf_size, 32);
    
}

void transfer_serial_data(uint16_t* data, const uint16_t data_length, const uint16_t chunk_size_byte) {
    for (uint16_t i = 0; i < (data_length*2); i += chunk_size_byte) {
        uint16_t transfer_size = ((data_length*2) - i < chunk_size_byte) ? (data_length*2 - i) : chunk_size_byte;
        Serial.write((const uint8_t *) data + i, transfer_size);
    }
}

void transfer_serial_data_float(float* data, const uint16_t data_length, const uint16_t chunk_size_byte) {
    for (uint16_t i = 0; i < (data_length*4); i += chunk_size_byte) {
        uint16_t transfer_size = ((data_length*4) - i < chunk_size_byte) ? (data_length*4 - i) : chunk_size_byte;
        Serial.write((const uint8_t *) data + i, transfer_size);
    }
}

void handle_error() {
    // serial is taken by matlab, use LED as indication
    digitalWrite(error_led, LOW);
}