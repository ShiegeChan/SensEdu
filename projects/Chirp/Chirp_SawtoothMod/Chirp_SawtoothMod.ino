#include <SensEdu.h>

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
const float fs =  100 * END_FREQUENCY; // Sampling frequency
const float samples = fs * CHIRP_DURATION; // Number of samples
const uint32_t samples_int = (uint32_t)samples;
static SENSEDU_DAC_BUFFER(lut, samples_int); // Buffer for the chirp signal

// Initialize DAC settings
    SensEdu_DAC_Settings dac1_settings = {
        DAC_CH2, fs, (uint16_t*)lut, samples_int,
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 1
    };

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);
    while(!Serial);

    // Check if array size exceeds 7688 or fs exceeds 15 MHz
    if (samples_int > 7688 || fs > 15000000) {
        Serial.println("Error: samples_int exceeds 7688 or fs exceeds 15 MHz!");
        while (true);
      }

    // Generate the chirp signal
    generateSawtoothChirp(lut);

    SensEdu_DAC_Init(&dac1_settings);
    SensEdu_DAC_Enable(DAC_CH2);

    // Check for errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    Serial.println("Setup is successful.");

    // Print the chirp signal LUT
    Serial.println("start of the Chirp LUT");
    for (int i = 0 ; i < samples_int; i++) { // loop for the LUT size
        Serial.print("value ");
        Serial.print(i+1);
        Serial.print(" of the Chirp LUT: ");
        Serial.println(lut[i]);
    }
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    // Check for errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}