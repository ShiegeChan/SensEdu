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

// (comment this line to start up from matlab)
//#define SERIAL_OFF    // disable serial communication to test board by itself

// make this reconfig too
#define LOCAL_XCORR     true    // doing xcorr on the microcontroller
#define XCORR_DEBUG     true   // sending only distance w/o other data

#define BAN_DISTANCE	25		// min distance [cm] - how many self reflections cancelled
#define ACTUAL_SAMPLING_RATE 100000 // You need to measure this value using a wave generator with a fixed e.g. 1kHz Sine
#define STORE_BUF_SIZE  64 * 32    // 2400 for 1 measurement per second. 
                            	// only multiples of 32!!!!!! (64 chunk size of bytes, so 32 for 16bit)

/********************************** ADC ***************************************/

ADC_TypeDef* adc1 = ADC1;
ADC_TypeDef* adc2 = ADC2;

const uint8_t adc1_mic_num = 2; // 2 microphones for adc1
const uint8_t adc2_mic_num = 2; // 2 microphones for adc2
uint8_t adc1_pins[adc1_mic_num] = {A5, A4}; // mic1 and mic2 are on adc1
uint8_t adc2_pins[adc2_mic_num] = {A1, A6}; // mic3 and mic4 are on adc2

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t adc1_data_size = STORE_BUF_SIZE * adc1_mic_num; // 2048 * 2 = 64 * 32 * 2
const uint16_t adc2_data_size = STORE_BUF_SIZE * adc2_mic_num;
const uint16_t adc_data_size = STORE_BUF_SIZE * (adc1_mic_num + adc2_mic_num);
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc1_data[adc1_data_size];
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc2_data[adc2_data_size];
//__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc_data[adc_data_size];


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

/********************************************* DAC **************************************************/
// lut settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

SensEdu_DAC_Settings dac1_settings = {DAC1, DAC_SAMPLE_RATE, (uint16_t*)sine_lut, sine_lut_size, 
    SENSEDU_DAC_MODE_BURST_WAVE, dac_cycle_num}; // specifying burst mode 


/********************************************* FILTER ************************************************/
#define FILTER_KERNEL_LENGTH    33                      // length of the coefficient buffer AT
#define FILTER_BLOCK_LENGTH     (STORE_BUF_SIZE/4)      // how many samples we want to process every time we call the fir process function AT
#define FILTER_TAP_NUM          100                     // tap number for the bandpass filter

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

const uint16_t air_speed = 343; // m/s

// e.g. 25cm ban means 0.25*2/343 time ban, then multiply by sample rate
const uint32_t banned_sample_num = ((BAN_DISTANCE*2*ACTUAL_SAMPLING_RATE)/air_speed)/100; 

float32_t firState[FILTER_BLOCK_LENGTH + FILTER_TAP_NUM - 1];   // current filter state buffer
arm_fir_instance_f32 Fir_filt;  // creating an object instance


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
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

void handle_error() {
    // serial is taken by matlab, use LED as indication
    digitalWrite(error_led, LOW);
}

// initialize main structure
void main_obj_init(SenseduBoard* obj_ptr) {
	obj_ptr->ban_flag = 1;
	obj_ptr->serial_read_buf = '0';
    clear_float_buf(obj_ptr->xcorr_buffer, STORE_BUF_SIZE);
}

// buffer needs to be initialized because it crashes if you access an array element w/o initialization
void clear_float_buf(float array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0.0f;
    }
}

// make sure the buffer is initialized because it crashes if you access an array element w/o initialization
void clear_8bit_buf(uint8_t array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0x00;
    }
}

// sent data size to matlab - how many samples we currently have for 1 measurement
void send_config() {
    // 4bytes for data length
    Serial.write(uint8_t(STORE_BUF_SIZE >> 24));
    Serial.write(uint8_t(STORE_BUF_SIZE >> 16));
    Serial.write(uint8_t(STORE_BUF_SIZE >> 8));
    Serial.write(uint8_t(STORE_BUF_SIZE));
}

// "adc_data_length" should be the size of the wave "rescaled_adc_wave"
void custom_xcorr(float* xcorr_buf, const uint16_t* dac_wave, uint32_t adc_data_length) {
    // delay loop
    for (int32_t m = 0; m < adc_data_length; m++) {
        // sum loop
        float sum = 0;
        for (uint16_t n = 0; n < dac_wave_size; n++) {
            uint32_t idx = n + m;
            if (idx < adc_data_length) {
                sum += dac_wave[n]*xcorr_buf[idx]; 
            }
        }
        // indexes never overlap with previous computation -> safe to reuse for memory management
        xcorr_buf[m] = sum; 
    }
}

// rescale samples to normalized -1:1 values around zero
void rescale_adc_wave(float* rescaled_adc_wave, uint16_t* adc_wave, const char* channel, size_t adc_data_length) {
    // 0:65535 -> -1:1
    /* float sum = 0.0f;
    uint32_t cnt_ch1 = 0;
    uint32_t cnt_ch2 = 0;
    char ch = channel[0];
    for (uint32_t i = 0; i < adc_data_length; i++) {
        if(ch == '1' && i%2==0) {
            rescaled_adc_wave[cnt_ch1] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
            sum += rescaled_adc_wave[cnt_ch1];
            cnt_ch1++;
        } 
        else if(ch == '2' && i%2==1) {
            rescaled_adc_wave[cnt_ch2] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
            sum += rescaled_adc_wave[cnt_ch2];
            cnt_ch2++;
        }
        
    }

    //filter_32kHz_wave(rescaled_adc_wave, adc_data_length);

    // also center it around zero (remove it after filtering which would remove DC component)
    float mean = sum/cnt_ch1;
    for (uint32_t i = 0; i < cnt_ch1; i++) {
        rescaled_adc_wave[i] = rescaled_adc_wave[i] - mean;
    }*/
    // 0:65535 -> -1:1
    float sum = 0.0f;
    uint32_t cnt = 0;
    char ch = channel[0];
    clear_float_buf(rescaled_adc_wave, STORE_BUF_SIZE);
    if(ch=='1') {
        for(uint32_t i = 0; i < 2 * STORE_BUF_SIZE; i+=2) {
            rescaled_adc_wave[cnt] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
            sum += rescaled_adc_wave[cnt];
            cnt++;
        }
    }
    else if(ch=='2') {
        for(uint32_t i = 1; i < 2 * STORE_BUF_SIZE; i+=2) {
            rescaled_adc_wave[cnt] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
            sum += rescaled_adc_wave[cnt];
            cnt++;
        }
    }
    float mean = sum/cnt;
    for (uint32_t i = 0; i < STORE_BUF_SIZE; i++) {
        rescaled_adc_wave[i] = rescaled_adc_wave[i] - mean;
    }
}

// filter the wave around 32kHz
void filter_32kHz_wave(float* rescaled_adc_wave, uint16_t adc_data_length) {
    float32_t output_signal[adc_data_length];
    // need to take block chunks of the input signal
    for(uint16_t i = 0; i < adc_data_length; i += FILTER_BLOCK_LENGTH) {
        // take care of the last block
        size_t block_size = min(FILTER_BLOCK_LENGTH, adc_data_length - i);
        // perform the filter operation for the current block
        arm_fir_f32(&Fir_filt, &rescaled_adc_wave[i], &output_signal[i], block_size);
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

// clean it up in the future, especially with variables
uint32_t xcorr(float* xcorr_buf, size_t xcorr_buf_size, uint16_t* mic_array, size_t mic_array_size, const char* channel, uint8_t ban_flag) {
	rescale_adc_wave(xcorr_buf, mic_array, channel, mic_array_size);
    if (XCORR_DEBUG)
        serial_send_array((const uint8_t*)mic_array, mic_array_size, channel);

	// remove self reflections from a dataset
	if (ban_flag == 1) {
		for (uint32_t i = 0; i < banned_sample_num; i++) {
			xcorr_buf[i] = 0;
		}
	}

    if (XCORR_DEBUG)
        serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size, "b");

    custom_xcorr(xcorr_buf, dac_wave, STORE_BUF_SIZE);
    if (XCORR_DEBUG)
	    serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size, "b");

	uint32_t peak_index = 0;
	float biggest = 0.0f;

	for (uint32_t i = 0; i < STORE_BUF_SIZE; i++) {
		if (xcorr_buf[i] > biggest) {
			biggest = xcorr_buf[i];
			peak_index = i;
		}
	}
	uint16_t sr = 370; // kS/sec  sample rate
	uint16_t c = 343; // speed in air
	// (lag_samples * sample_time) * air_speed / 2
	uint32_t distance = ((peak_index * 1000 * c) / sr) >> 1; // in micrometers
    //uint32_t distance = 1000000;
    return distance;
}

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
	main_obj_init(main_obj_ptr);
    
    SensEdu_ADC_ShortA4toA9(); // in order to use ADC1 for mic2

    Serial.begin(115200); // 14400 bytes/sec -> 7200 samples/sec -> 2400 samples/sec for 1 mic

    /* DAC */
    SensEdu_DAC_Init(&dac1_settings);
    
    /* ADC 1 */
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(adc1);

    /* ADC 2 */
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc2);

    // initializing the filter
    arm_fir_init_f32(&Fir_filt, FILTER_TAP_NUM, (float32_t *)&filter_taps[0], &firState[0], FILTER_BLOCK_LENGTH); 

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
    // Measurement is initiated by the signal from computing device
    static char serial_buf = 0;
    
    // Measurement is initiated by signal from computing device
	#ifndef SERIAL_OFF
		while (1) {
			while (Serial.available() == 0);    // Wait for a signal
			serial_buf = Serial.read();
			// Send Configuration Data to computing device
			if (serial_buf == 't') {
				//send_config();
                break; 
			}
		}
	#endif
    
    // start dac->adc sequence
    SensEdu_DAC_Enable(DAC1);
    while(!SensEdu_DAC_GetBurstCompleteFlag()); // wait for dac to finish sending the burst
    SensEdu_DAC_ClearBurstCompleteFlag(); 
    SensEdu_ADC_Start(adc1);
    SensEdu_ADC_Start(adc2);

    // wait for the data
    while(!SensEdu_DMA_GetADCTransferStatus(ADC1));
    SensEdu_DMA_ClearADCTransferStatus(ADC1);

    while(!SensEdu_DMA_GetADCTransferStatus(ADC2));
    SensEdu_DMA_ClearADCTransferStatus(ADC2);

    if(!LOCAL_XCORR) {
        // just send the data bunch of bits first both channels from adc1 and then both channels from adc2
        serial_send_array((const uint8_t *)adc1_data, sizeof(adc1_data), "a");
        serial_send_array((const uint8_t *)adc2_data, sizeof(adc2_data), "a");
        return;
    }

    // otherwise caluculate the distance here using local xcorr
    static uint32_t distance[4];
	distance[0] = xcorr(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc1_data, sizeof(adc1_data), "1", main_obj_ptr->ban_flag);
    distance[1] = xcorr(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc1_data, sizeof(adc1_data), "2", main_obj_ptr->ban_flag);
    distance[2] = xcorr(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc2_data, sizeof(adc2_data), "1", main_obj_ptr->ban_flag);
    distance[3] = xcorr(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc2_data, sizeof(adc2_data), "2", main_obj_ptr->ban_flag);

    Serial.write((const uint8_t *) &distance[0], 4);
    Serial.write((const uint8_t *) &distance[1], 4);
    Serial.write((const uint8_t *) &distance[2], 4);
    Serial.write((const uint8_t *) &distance[3], 4);

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}