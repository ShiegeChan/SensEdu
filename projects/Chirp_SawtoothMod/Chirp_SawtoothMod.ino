#include <SensEdu.h>


/* -------------------------------------------------------------------------- */
/*                                User Settings                               */
/* -------------------------------------------------------------------------- */

#define CHIRP_DURATION          0.0009    // Duration of the chirp (in seconds)
#define START_FREQUENCY         30300    // Start frequency (in Hz)
#define END_FREQUENCY           35300   // Stop frequency (in Hz)

/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */

static uint32_t lib_error = 0;
static uint8_t increment_flag = 1; // Run time modification flag
const float fs = 100 * END_FREQUENCY; // Sampling frequency of LUT
const float samples = fs * CHIRP_DURATION; // Number of samples
const uint32_t samples_int = (uint32_t)samples;
const float chirp_nb = 1/CHIRP_DURATION; //Number of chirp cycles per second
static SENSEDU_DAC_BUFFER(lut, samples_int); // Buffer for the chirp signal

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);

    // Initialize DAC settings
    SensEdu_DAC_Settings dac1_settings = {
        DAC1, chirp_nb*samples_int, (uint16_t*)lut, samples_int,
        SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0
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