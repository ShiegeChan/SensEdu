#include "SensEdu.h"

/* errors */
static uint32_t lib_error = 0;
uint8_t error_led = D86;
const int SYNC_PIN = 2;

// Configure the ADC
const uint16_t SAMPLES_PER_BIT = 64;
const uint16_t BIT_PER_BYTE = 8;
const uint16_t MAX_MESSAGE_LENGTH = 64;
const uint16_t mic_data_size = MAX_MESSAGE_LENGTH * BIT_PER_BYTE * SAMPLES_PER_BIT;  //match MAX_LUT_SIZE
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
    while (digitalRead(SYNC_PIN) == LOW) {};  // Wait for sync
    SensEdu_ADC_Start(adc);
    // wait for the data and send it
    while (!SensEdu_ADC_GetTransferStatus(adc));
    SensEdu_ADC_ClearTransferStatus(adc);
    while (digitalRead(SYNC_PIN) == HIGH) {};  // Wait for the signal to stop

    // **NUEVO: Buscar el inicio dinámicamente**
    int dynamic_offset = 0;
    const uint16_t THRESHOLD = 2000;  // Ajusta según tus pruebas
    
    for (int i = 0; i < mic_data_size; i++) {
        if (mic_data[i] < THRESHOLD) {
            dynamic_offset = i;
            break;
        }
    }

    float fs = 32800 * 32;  // Your sample rate
    float f0 = fs/SAMPLES_PER_BIT;       // YOUR low frequency
    float f1 = fs*2/SAMPLES_PER_BIT;       // YOUR high frequency

    for (int byte_idx = 0; byte_idx < MAX_MESSAGE_LENGTH; byte_idx++) {
        uint8_t received_byte = 0;
        float total_power = 0; //Acumulate power

        for (int bit_pos = 7; bit_pos >= 0; bit_pos--) {
            // Calculate position in buffer
            int bit_index = byte_idx * BIT_PER_BYTE + (7 - bit_pos);
            uint16_t* bit_samples = &mic_data[dynamic_offset + bit_index * SAMPLES_PER_BIT];  //maybe add an offset to skip some data int offset = 10 (?)

            // Run Goertzel
            float power_f0 = goertzel(bit_samples, SAMPLES_PER_BIT, f0, fs);
            float power_f1 = goertzel(bit_samples, SAMPLES_PER_BIT, f1, fs);

            total_power += power_f0 + power_f1; //Add powers

            // Serial.print("Bit ");
            // Serial.print(bit_pos);
            // Serial.print(": P0=");
            // Serial.print(power_f0, 0);
            // Serial.print(" P1=");
            // Serial.println(power_f1, 0);

            // Decode bit
            bool bit = (power_f1 > power_f0);
            if (bit) {
                received_byte |= (1 << bit_pos);
            }
        }
    // Si la potencia promedio es muy baja, es silencio
    if (total_power / 8 < 1000000) {  // Ajusta el umbral según tus pruebas
        break;
    }
    // Send decoded character
//    Serial.println();
//    Serial.print("Received text: ");
    Serial.write(received_byte);
//    Serial.println();
//    Serial.println(received_byte);  // Newline for readability
    }
    Serial.println();
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
