const uint32_t x = 5; // cos LUT resolution
const float Pi = 3.14159; // pi

// Generate the chirp signal
void generateChirpSignal(uint16_t* array) {

    float lut_cos[90 * x]; // Quarter-wave LUT for cosine values
    float vChirp[samples_int]; // Chirp signal LUT

    // Generate the quarter-wave cosine LUT
    for (int i = 0; i < 90 * x; i++) {
        float phase_deg = (float)i * 90.0 / (90 * x); // Phase angle in degrees
        float phase_rad = phase_deg * Pi / 180.0; // Phase angle to radians
        lut_cos[i] = cos(phase_rad); // Store cosine value in the LUT
    }

    // Generate the chirp signal
    float sT = CHIRP_DURATION; // Time duration of the chirp signal
    float sK = (END_FREQUENCY - START_FREQUENCY) / sT; // Chirp rate

    for (int i = 1; i < samples_int + 1; i++) {
        float phase_rad = 2.0 * Pi * (0.5 * sK * (i - 1) / fs + START_FREQUENCY) * (i - 1) / fs; // Phase angle in radians
        float phase_deg = phase_rad * 180.0 / Pi; // Phase angle to degrees
        float phase_deg_wrapped = fmod(phase_deg, 360.0); // Wrap phase angle to 0-360 degrees

        if (phase_deg_wrapped <= 90) {
            vChirp[i - 1] = lut_cos[(int)(phase_deg_wrapped / 90.0 * (90 * x - 1))] * 2047.5 + 2048;
        } else if (phase_deg_wrapped <= 180) {
            vChirp[i - 1] = -lut_cos[(int)((180.0 - phase_deg_wrapped) / 90.0 * (90 * x - 1))] * 2047.5 + 2048;
        } else if (phase_deg_wrapped <= 270) {
            vChirp[i - 1] = -lut_cos[(int)((phase_deg_wrapped - 180.0) / 90.0 * (90 * x - 1))] * 2047.5 + 2048;
        } else {
            vChirp[i - 1] = lut_cos[(int)((360.0 - phase_deg_wrapped) / 90.0 * (90 * x - 1))] * 2047.5 + 2048;
        }
    }

     // Copy the chirp signal to the DAC buffer
    for (int i = 0; i < samples_int; i++) {
        array[i] = (uint16_t)vChirp[i]; 
    }
}