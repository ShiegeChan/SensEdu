#include "SensEdu.h"

// We need to create two LUTs for the sine wave we want to transmit
const uint16_t sine_lut_size_0 = 17 * 8; // sine wave size bit '0' 31000 Hz
static uint16_t array_bit0[sine_lut_size_0] = {
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000, 
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000, 
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000, 
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000, 
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000, 
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000, 
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000, 
0x000, 0x09C, 0x258, 0x4F0, 0x800, 0xB0F, 0xDA7, 0xF63, 
0xFFF, 0xF63, 0xDA7, 0xB0F, 0x800, 0x4F0, 0x258, 0x09C, 0x000
};

const uint16_t sine_lut_size_1 = 15 * 8; // sine wave size bit '1' 35000 Hz
static uint16_t array_bit1[sine_lut_size_1] = {
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000, 
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000, 
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000, 
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000, 
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000, 
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000, 
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000, 
0x000, 0x0CB, 0x303, 0x638, 0x9C7, 0xCFC, 0xF34, 0xFFF, 
0xF34, 0xCFC, 0x9C7, 0x638, 0x303, 0x0CB, 0x000 
};

/* errors */
static uint32_t lib_error = 0; // Internal library error container
uint8_t error_led = D86; 

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Dynamic buffer configuration
const uint16_t SAMPLES_PER_BIT = 136; 
const uint16_t BIT_PER_BYTE = 8;
const uint16_t MAX_MESSAGE_LENGTH = 30; // Maximum allowed by number of samples
const uint16_t MAX_LUT_SIZE = MAX_MESSAGE_LENGTH * BIT_PER_BYTE * SAMPLES_PER_BIT; 
static SENSEDU_DAC_BUFFER(lut , MAX_LUT_SIZE); 
uint8_t message[MAX_MESSAGE_LENGTH];
uint8_t length = 0;

const int SYNC_PIN = 2; // Syncronization signal from PIN 2 digital

/* ----------------------------------- DAC ---------------------------------- */
#define DAC_SINE_FREQ    	33000
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * 16   // samples per one sine cycle  

SensEdu_DAC_Settings dac_settings_1 = {
    .dac_channel = DAC_CH1, 
    .sampling_freq = DAC_SAMPLE_RATE,
    .mem_address = (uint16_t*)lut, 
    .mem_size = MAX_LUT_SIZE, 
    .wave_mode = SENSEDU_DAC_MODE_BURST_WAVE,
    .burst_num = 1
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
    pinMode(SYNC_PIN, OUTPUT);
    digitalWrite(SYNC_PIN, LOW);
    
    SensEdu_DAC_Init(&dac_settings_1);

    Serial.println("Enter message:");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop () {
    delay(1000);

    // Wait for the message on the serial post and send it
    if (Serial.available() > 0) {
        length = 0;
        
        while (Serial.available() > 0 && length < MAX_MESSAGE_LENGTH) {
            message[length] = Serial.read();
            length++;
        }
        
        while (length > 0 && (message[length - 1] == '\n' || message[length - 1] == '\r')) {
            length--;
        }
        
        if (length > 0) {
            digitalWrite(SYNC_PIN, HIGH);
            sendMessage(message, length); 
            digitalWrite(SYNC_PIN, LOW);
        }
    }
    check_errors();
}

// Checking errors of the library
void check_errors() {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        digitalWrite(error_led, LOW);
        Serial.println(lib_error, HEX);
    }
}

// Build the LUT to send
void buildMessageLUT (uint8_t* data, uint8_t num_bytes) {
    uint16_t position = 0;

    // Clear the entire LUT first (fill with DC level, e.g., 0x800)
    for (size_t i = 0; i < MAX_LUT_SIZE; i++) {
        lut[i] = 0x800;  // Mid-level (silence)
    }
    for (size_t i = 0; i < sine_lut_size_1; i++) {
        lut[position + i] = array_bit1[i];  // Mid-level (silence)
    }
    position += sine_lut_size_1; // Update the starting position

    // Limit to maximum message length
    if (num_bytes > MAX_MESSAGE_LENGTH) {
        num_bytes = MAX_MESSAGE_LENGTH;
        Serial.println("Warning: Message truncated to MAX_MESSAGE_LENGTH");
    }

    // Build waveform for each byte in the message
    for (size_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        uint8_t current_byte = data[byte_idx];
    
        for (int32_t bit_pos = 7; bit_pos >= 0; bit_pos--) { 
            bool bit = (current_byte >> bit_pos) & 1; 

            if (bit) { // Asign bit '1' to high frequency LUT 
                for (size_t i = 0; i < sine_lut_size_1; i++) {
                    lut[position + i] = array_bit1[i];
                }
                position += sine_lut_size_1;
            } else { //Asign bit '0' to low frequency LUT
                for (size_t i = 0; i < sine_lut_size_0; i++) {
                    lut[position + i] = array_bit0[i];
                }
                position += sine_lut_size_0;
            }
        }
    // Rest of byte_lut stays at 0x800 (silence padding)
    }
}

void sendMessage(uint8_t* data, uint8_t num_bytes) {
    buildMessageLUT(data, num_bytes);  // Update byte_lut contents
    
    SensEdu_DAC_Enable(DAC_CH1);
    while (!SensEdu_DAC_GetBurstCompleteFlag(DAC_CH1));
    SensEdu_DAC_ClearBurstCompleteFlag(DAC_CH1);
    SensEdu_DAC_Disable(DAC_CH1);  // Clean shutdown
}
