#include <SensEdu.h>


/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define CHIRP_DURATION          0.002    // Duration of the chirp (in seconds)
#define START_FREQUENCY         30300    // Start frequency (in Hz)
#define END_FREQUENCY           35300   // Stop frequency (in Hz)

/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */

static uint32_t lib_error = 0;
uint8_t error_led = D86;
static uint8_t increment_flag = 1; // Run time modification flag
const float fs = 42 * END_FREQUENCY; // Sampling frequency of LUT
const float samples = fs * CHIRP_DURATION; // Number of samples
const uint32_t samples_int = (uint32_t)samples;
static SENSEDU_DAC_BUFFER(lut, samples_int*2); // Buffer for the chirp signal

// Initialize DAC settings
SensEdu_DAC_Settings dac1_settings = {
    DAC1, fs, (uint16_t*)lut, samples_int*2,
    SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0
};

const uint16_t mic_data_size = 64*32; // must be multiple of 64 for 16bit
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t mic_data[mic_data_size]; // cache aligned

ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = {A4};
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

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);
    
    // Generate the chirp signal
    generateChirpSignal(lut);

    SensEdu_DAC_Init(&dac1_settings);
    SensEdu_DAC_Enable(DAC1);

    SensEdu_ADC_ShortA4toA9();
    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);

    // Check for errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }

    Serial.println("Setup is successful.");
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
    SensEdu_ADC_Start(adc);
    
    // wait for the data and send it
    while(!SensEdu_DMA_GetADCTransferStatus(ADC1));
    SensEdu_DMA_ClearADCTransferStatus(ADC1);
    serial_send_array((const uint8_t *) & mic_data, mic_data_size << 1);


    // Check for errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
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