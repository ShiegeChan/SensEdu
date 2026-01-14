#include "SensEdu.h"

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;
const int offset = 5; //Wait for the signal to arrive, change if needed
const int SYNC_PIN = 2;

// Configure the ADC
const uint16_t mic_data_size = 64 * 8 + offset;  //match MAX_LUT_SIZE
SENSEDU_ADC_BUFFER(mic_data, mic_data_size);

ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = { A1 };
SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = mic_pins,
    .pin_num = mic_num,
    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 32800 * 32,
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)mic_data,
    .mem_size = mic_data_size
};

void setup() {
    Serial.begin(115200);
    Serial.println("Started Initialization...");

    //Led in red if there is any problem
    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    pinMode(SYNC_PIN, INPUT);

    //Initializing DAC
    SensEdu_ADC_Init(&adc_settings);
    SensEdu_ADC_Enable(adc);
}

void loop() {
    // Measurement is initiated by the signal from computing device
    static char serial_buf = 0;

    while (digitalRead(SYNC_PIN) == LOW) {};  // Wait for sync
    SensEdu_ADC_Start(adc);
    // wait for the data and send it
    while (!SensEdu_ADC_GetTransferStatus(adc));
    SensEdu_ADC_ClearTransferStatus(adc);
    while (digitalRead(SYNC_PIN) == HIGH) {};  // Wait for the signal to stop

    // //Print the values 
    // for (int i = 0; i < mic_data_size; i++) {
    //     Serial.println(mic_data[i]);
    // }

    // Decode byte
    uint8_t received_byte = 0;
    int samples_per_bit = 64;  // 560 / 8 = 1024

    float fs = 32800 * 32;  // Your sample rate
    float f0 = fs/samples_per_bit;       // YOUR low frequency
    float f1 = fs*2/samples_per_bit;       // YOUR high frequency

    for (int bit_pos = 7; bit_pos >= 0; bit_pos--) {
        // Get samples for this bit
        uint16_t* bit_samples = &mic_data[offset + (7-bit_pos) * samples_per_bit];  //maybe add an offset to skip some data int offset = 10 (?)

        // Run Goertzel
        float power_f0 = goertzel(bit_samples, samples_per_bit, f0, fs);
        float power_f1 = goertzel(bit_samples, samples_per_bit, f1, fs);

        Serial.print("Bit ");
        Serial.print(bit_pos);
        Serial.print(": P0=");
        Serial.print(power_f0, 0);
        Serial.print(" P1=");
        Serial.println(power_f1, 0);

        // Decode bit
        bool bit = (power_f1 > power_f0 * 1.3);
        if (bit) {
            received_byte |= (1 << bit_pos);
        }
  }

    // Send decoded character
    Serial.println();
    Serial.print("Received text: ");
    Serial.write(received_byte);
    Serial.println();
    //Serial.println(received_byte);  // Newline for readability
}

// Checking errors of the library
void check_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
        Serial.println(lib_error, HEX);
    }
}

// Goertzel function
float goertzel(uint16_t* samples, int N, float targetFreq, float sampleRate) {
    float k = round((N * targetFreq) / sampleRate);  
    float omega = (2.0 * PI * k) / N;
    float coeff = 2.0 * cos(omega);

    //DC offset of our samples
    float sum = 0;
    for(int i = 0; i < N; i++) {
        sum += samples[i];
    }
    float dc_center = sum / N;

    float q0 = 0, q1 = 0, q2 = 0;

    for (int i = 0; i < N; i++) {
        // Remove DC offset
        float sample = (float)(samples[i] -dc_center); 
        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }
    float result =  q1 * q1 + q2 * q2 - q1 * q2 * coeff;
    return result;
}
