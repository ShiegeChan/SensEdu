const uint32_t x = 5; // sine LUT resolution
const float Pi = 3.14159; // pi

// Generate chirp signal with a sawtooth modulation
void generateSawtoothChirp(uint16_t* array) {

    float lut_sine[90 * x]; // Quarter-wave LUT for sine values
    float vChirp; // Chirp signal value
    float phase_deg; // Phase angle in degrees
    float phase_rad; // Phase angle in radians
    float phase_deg_wrapped; // Wrapped phase angle
    int N = round((START_FREQUENCY + END_FREQUENCY) / 2.0 * CHIRP_DURATION); // Closest integer number of cycles
    int END_FREQUENCY_ADJUSTED = 2*N/CHIRP_DURATION - START_FREQUENCY; // Adjusted end frequency for int number of cycles
    float sK = (END_FREQUENCY_ADJUSTED - START_FREQUENCY) / CHIRP_DURATION; // Adjusted chirp rate
    
    // Generate the quarter-wave sine LUT
    for (int i = 0; i < 90 * x; i++) {
        phase_deg = (float)i/x;
        phase_rad = phase_deg * Pi / 180.0;
        lut_sine[i] = sin(phase_rad-Pi/2); // Store sine value in the LUT
    }

    // Generate the chirp signal
    for (int i = 0; i < samples_int; i++) {
        phase_rad = 2.0 * Pi * (0.5 * sK * i / fs + START_FREQUENCY) * i / fs; // Phase angle in radians
        phase_deg = phase_rad * 180.0 / Pi; // Phase angle to degrees
        phase_deg_wrapped = fmod(phase_deg, 360.0); // Wrap phase angle to 0-360 degrees

        if (phase_deg_wrapped <= 90) {
            vChirp = lut_sine[(int)(phase_deg_wrapped)* x+1] * 2048 + 2048;
        } else if (phase_deg_wrapped <= 180) {
            vChirp = -lut_sine[(int)(180.0 - phase_deg_wrapped) * x+1] * 2048 + 2048;
        } else if (phase_deg_wrapped <= 270) {
            vChirp = -lut_sine[(int)(phase_deg_wrapped - 180.0) * x+1] * 2048 + 2048;
        } else {
            vChirp = lut_sine[(int)(360.0 - phase_deg_wrapped)* x+1] * 2048 + 2048;
        }

        array[i] = (uint16_t)vChirp;
    }
}