#include "SensEdu.h"
/* errors */
static uint32_t lib_error = 0; // Internal library error container
uint8_t error_led = D86; 

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

const uint16_t THRESHOLD = 5000;  // If the signal goes higher, synchro starts
float fs = 33000 * 32;  // Your sample rate
float f0 = fs / 34;       // YOUR low frequency
float f1 = fs / 30;       // YOUR high frequency
const int SYNC_PIN = 2; // Syncronization signal from PIN 2 digital
int current_sample_ptr = 0; 
const int WINDOW_SIZE = 50;  // Tamaño de ventana deslizante
int signal_start_pos = 0;
float total_power = 0;
bool signal_found = false;
const uint16_t SAMPLES_PER_BIT = 272;
const uint16_t BIT_PER_BYTE = 8;
const uint16_t MAX_MESSAGE_LENGTH = 30;
const uint16_t mic_data_size = MAX_MESSAGE_LENGTH * BIT_PER_BYTE * SAMPLES_PER_BIT;  //match MAX_LUT_SIZE
SENSEDU_ADC_BUFFER(mic_data, mic_data_size);

/* ----------------------------------- ADC ---------------------------------- */
ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = { A1 };
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = mic_pins,
    .pin_num = mic_num,
    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = 33000 * 32,
    .adc_mode = SENSEDU_ADC_MODE_DMA_NORMAL,
    .mem_address = (uint16_t*) mic_data,
    .mem_size = mic_data_size
};

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");
    //Led in red if there is any problem
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    pinMode(SYNC_PIN, INPUT);

    //Initializing ADC
    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
}
/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    while (digitalRead(SYNC_PIN) == LOW) {};  // Wait for sync
    SensEdu_ADC_Start(adc);
    // wait for the data and send it
    while (!SensEdu_ADC_IsDmaTransferComplete(adc));
    SensEdu_ADC_ClearDmaTransferComplete(adc);
    while (digitalRead(SYNC_PIN) == HIGH) {};  // Wait for the signal to stop

    for (size_t i = 0; i < mic_data_size - SAMPLES_PER_BIT; i += 5) {
        float p0 = goertzel(&mic_data[i], WINDOW_SIZE, f0, fs);
        float p1 = goertzel(&mic_data[i], WINDOW_SIZE, f1, fs);
    
        if ((p0 + p1) > POWER_THRESHOLD) {
            signal_start_pos = i;
            current_sample_ptr = i + 120;
            signal_found = true;
            break; // You found the signal!
        }
    }

    for (size_t byte_idx = 0; byte_idx < MAX_MESSAGE_LENGTH; byte_idx++) {
        uint8_t received_byte = 0;

        for (int bit_pos = 7; bit_pos >= 0; bit_pos--) { 
            // Calculate position in buffer
            uint16_t* bit_samples = &mic_data[current_sample_ptr+20];

            // Run Goertzel
            float power_f0 = goertzel(bit_samples, 220, f0, fs);
            float power_f1 = goertzel(bit_samples, 220, f1, fs);
            total_power += power_f0 + power_f1; //Add powers

            // Decode bit
            bool bit = (1.1 * power_f1 > power_f0);
            
            if (bit) {
                received_byte |= (1 << bit_pos);
                current_sample_ptr += 240; // The size of bit '1' is 240 samples
            } else {
                current_sample_ptr += 272; // The size of bit '0' is 272 samples
            }
        }
        
        // If average power is too low, then it is silence
        if (total_power / 8 < 1000000) {  // Adjust depending on your signals 
            break;
        }
        // Send decoded character
        Serial.write(received_byte);
        Serial.println();
    }
}

// Checking errors of the library
void check_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
        Serial.println(lib_error, HEX);
    }
}

// Goertzel function: analyses one selectable frequency component from a discrete signal
float goertzel(uint16_t* samples, int N, float targetFreq, float sampleRate) {
    // Implementation of the filter
    float k = (N * targetFreq) / sampleRate;  
    float omega = (2.0 * PI * k) / N;
    float coeff = 2.0 * cos(omega);
    
    //DC offset of our samples
    float sum = 0;
    for (size_t i = 0; i < N; i++) {
        sum += samples[i];
    }
    float dc_center = sum / N;
    float q0 = 0, q1 = 0, q2 = 0;
    for (size_t i = 0; i < N; i++) {
        // Remove DC offset
        float sample = (float) (samples[i] - dc_center) / 1000; 
        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }
    float result =  q1 * q1 + q2 * q2 - q1 * q2 * coeff;
    return result;
}

