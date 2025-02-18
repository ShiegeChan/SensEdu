#include <SensEdu.h>

/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define CHIRP_DURATION          0.001   // Duration of the chirp (in seconds)
#define START_FREQUENCY         30300   // Start frequency (in Hz)
#define END_FREQUENCY           35300  // Stop frequency (in Hz)

/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */

static uint32_t lib_error = 0;
static uint8_t increment_flag = 1; // Run time modification flag
const float fs = 42 * END_FREQUENCY; // Sampling frequency
const float samples = fs * CHIRP_DURATION; // Number of samples
const uint32_t samples_int = (uint32_t)samples;
static SENSEDU_DAC_BUFFER(lut, samples_int); // Buffer for the chirp signal

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);
    while(!Serial);

    // Initialize DAC settings
    SensEdu_DAC_Settings dac1_settings = {
        DAC1, fs, (uint16_t*)lut, samples_int,
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 2
    };

    // Generate the chirp signal
    generateChirpSignal(lut);

    SensEdu_DAC_Init(&dac1_settings);
    SensEdu_DAC_Enable(DAC1);

    // Check for errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }

    Serial.println("Setup is successful.");

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

    /* delay(100);
    SensEdu_DAC_Enable(DAC1); */
}