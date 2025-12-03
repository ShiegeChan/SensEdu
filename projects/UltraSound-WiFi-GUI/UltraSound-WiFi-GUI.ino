#include "SensEdu.h"
#include "CMSIS_DSP.h"
#include "SineLUT.h"
#include "FilterTaps.h"
#include "DACWave.h" // contains wave and its size
#include <WiFi.h>

uint32_t lib_error = 0;
uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
#define WIFI_SSID   "TestWiFi"
#define WIFI_PASS   "TestWiFi"
#define WIFI_PORT   80

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

// HARDCODED FOR ONE CHANNEL OPERATION
// LOOK INTO /projects/UltraSound/extra/ultrasonic_distance_acq_detailed_dataset/
// FOR FLEXIBLE (BUT SLOWER) IMPLEMENTATION
const uint8_t adc1_mic_num = 1;
uint8_t adc1_pins[adc1_mic_num] = {A5}; // mic1

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t adc1_data_size = STORE_BUF_SIZE * adc1_mic_num;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc1_data[adc1_data_size];

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

/* ----------------------------------- DAC ---------------------------------- */
// lut settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

DAC_Channel* dac_ch = DAC_CH1;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_ch, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)sine_lut,
    .mem_size = sine_lut_size,
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = dac_cycle_num
};

/* ---------------------------------- WiFi ---------------------------------- */
int status = WL_IDLE_STATUS;
WiFiServer server(WIFI_PORT);

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

    check_lib_errors();

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(WIFI_SSID);

        // connect to WPA/WPA2 network (change this if youre using open / WEP network)
        status = WiFi.begin(WIFI_SSID, WIFI_PASS);

        // wait 10 seconds for connection:
        delay(10000);
    }
    server.begin();
    print_wifi_status();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    check_lib_errors();

	SenseduBoard* main_obj_ptr = &SenseduBoardObj;

    WiFiClient client = server.available();
    if (!client) {
        return;
    }
    Serial.println("Client connected!");

    // Measurement is initiated by the signal from computing device (matlab script)
    static char buf = 0;

    while(client.connected()) {
        if (!client.available()) {
            continue;
        }

        buf = client.read();
        if (buf != 't') { // trigger not detected
            continue;
        }

        // Start dac->adc sequence
        SensEdu_DAC_Enable(dac_ch);
        while(!SensEdu_DAC_GetBurstCompleteFlag(dac_ch)); // wait for dac to finish sending the burst
        SensEdu_DAC_ClearBurstCompleteFlag(dac_ch);

        // Start ADC
        SensEdu_ADC_Start(adc1);

        // Wait for the data from ADC1
        while(!SensEdu_ADC_GetTransferStatus(adc1));
        SensEdu_ADC_ClearTransferStatus(adc1);

        // Calculating distance for each microphone
        static uint32_t distance[adc1_mic_num];
        for(uint8_t i = 0; i < adc1_mic_num; i++) {
            process_and_transmit_data(client, main_obj_ptr->processing_buffer, STORE_BUF_SIZE, adc1_data, STORE_BUF_SIZE, main_obj_ptr->ban_flag, IS_TRANSMIT_DETAILED_DATA);
            distance[i] = calculate_distance(main_obj_ptr->processing_buffer);
        }

        // Sending the distance measurements
        for (uint8_t i = 0; i < (adc1_mic_num); i++) {
            client.write((const uint8_t *) &distance[i], 4);
        }

        check_lib_errors();
    }
}

void process_and_transmit_data(WiFiClient& client, float* buf, const uint16_t buf_size, uint16_t* ch_array, const uint16_t ch_array_size, uint8_t ban_flag, uint8_t is_detailed_transmission) {

    /* -------------------------------- RAW DATA -------------------------------- */
    if (is_detailed_transmission)
        transfer_wifi_data(client, ch_array, ch_array_size);
    
    /* --------------------- RESCALED, FILTERED, NO COUPLED --------------------- */
    // Rescale from [0, (2^16-1)] to [-1, 1] and filter around 32 kHz
    clear_float_buf(buf, buf_size);
    rescale_adc_wave(buf, ch_array, ch_array_size);
    filter_32kHz_wave(buf, buf_size);
    if (ban_flag == 1) {
        remove_coupling(buf, c_banned_sample_num);
    } 
    if (is_detailed_transmission)
        transfer_wifi_data_float(client, buf, buf_size);

    /* ---------------------------------- XCORR --------------------------------- */
	custom_xcorr(buf, dac_wave, buf_size);
    if (is_detailed_transmission)
	    transfer_wifi_data_float(client, buf, buf_size);
    
}

void transfer_wifi_data(WiFiClient& client, uint16_t* data, const uint16_t data_length) {
    client.write((const uint8_t*)data, data_length * sizeof(uint16_t));
}

void transfer_wifi_data_float(WiFiClient& client, float* data, const uint16_t data_length) {
    client.write((const uint8_t*)data, data_length * sizeof(float));
}

void check_lib_errors(void) {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

void handle_error(void) {
    digitalWrite(error_led, LOW);
    Serial.print("Error: 0x");
    Serial.println(lib_error, HEX);
    delay(1000);
}

void print_wifi_status(void) {
    // print the SSID of the network youre connected to
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your boards local IP address
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received WiFi signal strength
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}