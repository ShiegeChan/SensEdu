#include "SensEdu.h"
#include "CMSIS_DSP.h"
#include "SineLUT.h"
#include "FilterTaps.h"
#include "DACWave.h"
#include "Peaks.h"

#include <vector>
#include <array>


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

#define BAN_DISTANCE	            25	        // Min distance [cm] - how many self reflections cancelled
#define SAMPLING_RATE               250000      // ADC sampling rate
#define STORE_BUF_SIZE              2048        // 2400 for 1 measurement per second 
                            	            

/* --------------------------------- Filter --------------------------------- */

#define FILTER_BLOCK_LENGTH     32      // How many samples we want to process every time we call the fir process function AT
#define FILTER_TAP_NUM          32      // Tap number for the bandpass filter

static float32_t firStateBuffer[FILTER_BLOCK_LENGTH + FILTER_TAP_NUM - 1]; // Current filter state buffer
arm_fir_instance_f32 Fir_filt; // Creating an object instance

/* ----------------------------------- ADC ---------------------------------- */

const uint16_t adc13_data_size = STORE_BUF_SIZE * 3;
const uint16_t adc2_data_size = STORE_BUF_SIZE * 2;

SENSEDU_ADC_BUFFER(mic123_data, adc13_data_size);
SENSEDU_ADC_BUFFER(mic48_data, adc2_data_size);
SENSEDU_ADC_BUFFER(mic567_data, adc13_data_size);


ADC_TypeDef* adc1 = ADC1;
ADC_TypeDef* adc2 = ADC2;
ADC_TypeDef* adc3 = ADC3;

const uint8_t adc1_mic_num = 3;
const uint8_t adc2_mic_num = 2;
const uint8_t adc3_mic_num = 3;

uint8_t mic123_pins[adc1_mic_num] = {A1, A3, A4};
uint8_t mic48_pins[adc2_mic_num] = {A5, A10};
uint8_t mic567_pins[adc3_mic_num] = {A6, A8, A9};


SensEdu_ADC_Settings adc1_settings = {
    .adc = adc1,
    .pins = mic123_pins,
    .pin_num = adc1_mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)mic123_data,
    .mem_size = adc13_data_size
};

SensEdu_ADC_Settings adc2_settings = {
    .adc = adc2,
    .pins = mic48_pins,
    .pin_num = adc2_mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)mic48_data,
    .mem_size = adc2_data_size
};

SensEdu_ADC_Settings adc3_settings = {
    .adc = adc3,
    .pins = mic567_pins,
    .pin_num = adc3_mic_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)mic567_data,
    .mem_size = adc13_data_size
};

/* ----------------------------------- DAC ---------------------------------- */

#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

DAC_Channel* dac_channel = DAC_CH1;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_channel, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut,
    .mem_size = sine_lut_size,
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = dac_cycle_num
};

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

const uint16_t air_speed = 343; // m/s

// e.g. 25cm ban means 0.25*2/343 time ban, then multiply by sample rate
const uint32_t c_banned_sample_num = ((BAN_DISTANCE*2*SAMPLING_RATE)/air_speed)/100; 


/* -------------------------------------------------------------------------- */
/*                              Global Structure                              */
/* -------------------------------------------------------------------------- */

typedef struct {
	uint8_t ban_flag; // Activate self reflections ban
	char serial_read_buf;
    float processing_buffer[STORE_BUF_SIZE]; // Reusable float buffer for signal processing
    uint16_t channel_buffer[STORE_BUF_SIZE]; // Data rearrangement
} SenseduBoard;

static SenseduBoard SenseduBoardObj;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
	main_obj_init(main_obj_ptr);

    // Initializing the filter
    arm_fir_init_f32(&Fir_filt, FILTER_TAP_NUM, filter_taps, firStateBuffer, FILTER_BLOCK_LENGTH); 

    Serial.begin(115200);

    // DAC
    SensEdu_DAC_Init(&dac_settings);
    
    // ADC1
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(adc1);

    // ADC2
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc2);

    // ADC3
    SensEdu_ADC_Init(&adc3_settings);
    SensEdu_ADC_Enable(adc3);
    
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);
    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
    // Wait for trigger character 't' from computing device
    char c;
    while (true) {
        if (Serial.available() > 0) {
            c = Serial.read();
            if (c == 't') {
                break;
            }
        }
        delay(1);
    }
    
    // Start dac->adc sequence
    SensEdu_DAC_Enable(dac_channel);
    while (!SensEdu_DAC_GetBurstCompleteFlag(dac_channel)); // Wait for dac to finish sending the burst
    SensEdu_DAC_ClearBurstCompleteFlag(dac_channel); 
    
    // Start ADCs
    SensEdu_ADC_Start(adc1);
    SensEdu_ADC_Start(adc2);
    SensEdu_ADC_Start(adc3);

    while (!SensEdu_ADC_IsDmaTransferComplete(adc1));
    SensEdu_ADC_ClearDmaTransferComplete(adc1);

    while (!SensEdu_ADC_IsDmaTransferComplete(adc2));
    SensEdu_ADC_ClearDmaTransferComplete(adc2);

    while (!SensEdu_ADC_IsDmaTransferComplete(adc3));
    SensEdu_ADC_ClearDmaTransferComplete(adc3);

    // Calculating distance for each microphone
    static uint32_t distance[adc1_mic_num + adc2_mic_num];
    uint32_t test_3_dist[3];
    static std::vector<uint32_t> test_dist;
    test_dist.reserve((adc1_mic_num + adc2_mic_num + adc3_mic_num)*MAX_PEAKS);

    for (uint8_t i = 0; i < adc1_mic_num; i++) {
        get_channel_data(mic123_data, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, adc1_mic_num, i);
        process_data(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, main_obj_ptr->ban_flag);
        // distance[i] = calculate_distance(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, SAMPLING_RATE);
        calculate_distances(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, SAMPLING_RATE, test_3_dist);
        for (uint8_t k = 0; k < MAX_PEAKS; k++) {
            test_dist.push_back(test_3_dist[k]);
        }
    }
    for (uint8_t i = 0; i < adc2_mic_num; i++) {
        get_channel_data(mic48_data, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, adc2_mic_num, i);
        process_data(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, main_obj_ptr->ban_flag);
        calculate_distances(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, SAMPLING_RATE, test_3_dist);
        for (uint8_t k = 0; k < MAX_PEAKS; k++) {
            test_dist.push_back(test_3_dist[k]);
        }
    }
    for (uint8_t i = 0; i < adc3_mic_num; i++) {
        get_channel_data(mic567_data, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, adc3_mic_num, i);
        process_data(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, main_obj_ptr->channel_buffer, STORE_BUF_SIZE, main_obj_ptr->ban_flag);
        calculate_distances(main_obj_ptr->processing_buffer, STORE_BUF_SIZE, SAMPLING_RATE, test_3_dist);
        for (uint8_t k = 0; k < MAX_PEAKS; k++) {
            test_dist.push_back(test_3_dist[k]);
        }
    }

    // Sending the distance measurements
    for (uint8_t i = 0; i < (adc1_mic_num + adc2_mic_num + adc3_mic_num)*MAX_PEAKS; i++) {
        Serial.write((const uint8_t *) &test_dist[i], 4);
    }

    check_lib_errors();
    test_dist.clear();
}

void process_data(float* buf, const uint16_t buf_size, uint16_t* ch_array, const uint16_t ch_array_size, uint8_t ban_flag) {

    /* --------------------- RESCALED, FILTERED, NO COUPLED --------------------- */
    // Rescale from [0, (2^16-1)] to [-1, 1] and filter around 32 kHz
    clear_float_buf(buf, buf_size);
    rescale_adc_wave(buf, ch_array, ch_array_size);
    filter_32kHz_wave(buf, buf_size);
    if (ban_flag == 1) {
        remove_coupling(buf, c_banned_sample_num);
    } 
    /* ---------------------------------- XCORR --------------------------------- */
	custom_xcorr(buf, dac_wave, buf_size);
    
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

void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
}

