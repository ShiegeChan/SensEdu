#include <SensEdu.h>

/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define CHIRP_DURATION          0.04   // Duration of the chirp (in seconds)
#define START_FREQUENCY         30500  // Start frequency (in Hz)
#define END_FREQUENCY           35500  // Stop frequency (in Hz)

/* -------------------------------------------------------------------------- */
/*                                 Settings                                   */
/* -------------------------------------------------------------------------- */

// ADC Sampling
const uint16_t buf_size = 14400; // ADC buffer size
SENSEDU_ADC_BUFFER(tx_data, buf_size);
SENSEDU_ADC_BUFFER(rx_data, buf_size);

// ADC-DMA Hardware Settings
ADC_TypeDef* tx_adc = ADC3;
ADC_TypeDef* rx_adc = ADC1;
const uint8_t tx_pin_num = 1;
const uint8_t rx_pin_num = 1;
uint8_t tx_pins[tx_pin_num] = {A8}; 
uint8_t rx_pins[rx_pin_num] = {A1};

SensEdu_ADC_Settings tx_adc_settings = {
    .adc = tx_adc,
    .pins = tx_pins,
    .pin_num = tx_pin_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)tx_data,
    .mem_size = buf_size
};

SensEdu_ADC_Settings rx_adc_settings = {
    .adc = rx_adc,
    .pins = rx_pins,
    .pin_num = rx_pin_num,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 250000,
    
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*)rx_data,
    .mem_size = buf_size
};

// DAC settings
static uint8_t increment_flag = 1;             // Run time modification flag
const float fs = 10 * END_FREQUENCY;           // Sampling frequency
const float samples = fs * CHIRP_DURATION;     // Number of samples
const uint32_t samples_int = (uint32_t)samples;
static SENSEDU_DAC_BUFFER(lut, samples_int);   // Buffer for the chirp signal

DAC_Channel* dac_ch = DAC_CH2;
SensEdu_DAC_Settings dac_settings = {
    .dac_channel = dac_ch, 
    .sampling_freq = fs,
    .mem_address = (uint16_t*)lut,
    .mem_size = samples_int,
    .wave_mode = SENSEDU_DAC_MODE_CONTINUOUS_WAVE,
    .burst_num = 1
};

// Error Handling
uint8_t error_led = D86;    // Error indicator LED pin
uint32_t lib_error = 0;     // Tracks library errors
bool dac_data_sent = false; // To track whether DAC LUT was sent to MATLAB

/* -------------------------------------------------------------------------- */
/*                                   Setup                                    */
/* -------------------------------------------------------------------------- */

void setup() {
    // Initialize Serial Communication
    Serial.begin(115200);
    while(!Serial);

    // Initialize ADC
    SensEdu_ADC_Init(&tx_adc_settings);
    SensEdu_ADC_Init(&rx_adc_settings);
    SensEdu_ADC_Enable(tx_adc);
    SensEdu_ADC_Enable(rx_adc);

    // Generate the chirp signal
    generateSawtoothChirp(lut);
    
    // Initialize DAC
    SensEdu_DAC_Init(&dac_settings);
    SensEdu_DAC_Enable(DAC_CH2);

    // Setup Error LED
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH); // Turn off (active low)

    // Check for errors
    check_lib_errors();
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

        if (serial_buf == 't') { 
            // First trigger detected
            break;
        }
    }
    
    // Start ADC Data Acquisition
    SensEdu_ADC_Start(tx_adc);
    SensEdu_ADC_Start(rx_adc);

    // wait for the data and send it
    while(!SensEdu_ADC_IsDmaTransferComplete(tx_adc));
    SensEdu_ADC_ClearDmaTransferComplete(tx_adc);

    while(!SensEdu_ADC_IsDmaTransferComplete(rx_adc));
    SensEdu_ADC_ClearDmaTransferComplete(rx_adc);

    // Send ADC data (16-bit values, continuously)
    uint32_t adc_byte_length = buf_size * sizeof(uint16_t); // ADC data size in bytes
    Serial.write((uint8_t*)&adc_byte_length, sizeof(uint32_t));  // Send size header
    serial_send_array(&(tx_data[0]), buf_size, 32);                   
    serial_send_array(&(rx_data[0]), buf_size, 32);                   

    // Check for errors during the process
    check_lib_errors();
}

/* -------------------------------------------------------------------------- */
/*                              Functions                                     */
/* -------------------------------------------------------------------------- */

// Checks if the library has risen any internal errors
// Doesn't print the error code, since Serial is occupied
// Turns on the red LED on Arduino board instead
void check_lib_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
    }
}

void serial_send_array(uint16_t* data, const size_t data_length, const size_t chunk_size_byte) {
    for (size_t i = 0; i < (data_length << 1); i += chunk_size_byte) {
        size_t transfer_size = ((data_length << 1) - i < chunk_size_byte) ? ((data_length << 1) - i) : chunk_size_byte;
        Serial.write((const uint8_t *)data + i, transfer_size);
    }
}
