#include <SensEdu.h>

// usage example
// data is a pointer
// if it is not, use & symbol before the array

// serial_send_array((const uint8_t *)data, sizeof(data)); // second argument in bytes

// for this poor implementation, make sure data is multiple of 32

// send serial data in 32 byte chunks
void serial_send_array(const uint8_t* data, size_t size) {
    const size_t chunk_size = 32;
    size_t full_chunks = size / chunk_size;  // Calculate how many full 32-byte chunks there are
    size_t remainder = size % chunk_size;  // Calculate the remaining bytes after full chunks

    // Send all full chunks
    for (size_t i = 0; i < full_chunks; i++) {
        Serial.write(data + chunk_size * i, chunk_size);
    }

    // Send the remaining bytes, if any
    if (remainder > 0) {
        Serial.write(data + chunk_size * full_chunks, remainder);
    }
}

/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define CHIRP_DURATION          0.001   // Duration of the chirp (in seconds)
#define START_FREQUENCY         1000   // Start frequency (in Hz)
#define END_FREQUENCY           30000  // Stop frequency (in Hz)

/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */

static uint32_t lib_error = 0;
static uint8_t increment_flag = 1; // Run time modification flag
const float fs = 50 * END_FREQUENCY; // Sampling frequency
const float samples = fs * CHIRP_DURATION; // Number of samples
const uint32_t samples_int = (uint32_t)samples;
static SENSEDU_DAC_BUFFER(lut, samples_int); // Buffer for the chirp signal

    // Initialize DAC settings
    SensEdu_DAC_Settings dac1_settings = {
        DAC_CH2, fs, (uint16_t*)lut, samples_int,
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 2
    };

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);
    while(!Serial);

    // Check if array size exceeds 7688 or fs exceeds 15 MHz
    if (samples_int > 7688 || fs > 15000000) {
        //Serial.println("Error: samples_int exceeds 7688 or fs exceeds 15 MHz!");
        while (true);
      }

    // Generate the chirp signal
    generateChirpSignal(lut);

    SensEdu_DAC_Init(&dac1_settings);
    SensEdu_DAC_Enable(DAC_CH2);

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

    uint32_t length_in_bytes = samples_int * 2; // Total number of bytes (16-bit values)
    Serial.write((uint8_t*)&length_in_bytes, sizeof(length_in_bytes)); // Send the length (4 bytes)
    
    serial_send_array((const uint8_t *) & lut, samples_int * 2);
}
