const uint32_t x = 5; // sin LUT resolution
const float Pi = 3.14159; // pi

// Generate the chirp signal
void generateTriangularChirp(uint16_t* array) {

    float lut_sin[90 * x]; // Quarter-wave LUT for sine values
    float vChirp; // Chirp signal value
    int n = round(samples_int/2);
    float phase_deg; // Phase angle in degrees
    float phase_rad; // Phase angle in radians
    float phase_deg_wrapped; // Wrapped phase angle
    float sK = 2*(END_FREQUENCY - START_FREQUENCY) / CHIRP_DURATION; // Chirp rate

    // Generate the quarter-wave sine LUT
    for (int i = 0; i < 90 * x; i++) {
        phase_deg = (float)i * 90.0 / (90 * x); // Phase angle in degrees
        phase_rad = phase_deg * Pi / 180.0; // Phase angle to radians
        lut_sin[i] = sin(phase_rad-Pi/2); // Store sine value in the LUT
    }

    // Generate the chirp signal
    for (int i = 0; i < n; i++) {
        phase_rad = 2.0 * Pi * (0.5 * sK * (i - 1) / fs + START_FREQUENCY) * (i - 1) / fs; // Phase angle in radians
        phase_deg = phase_rad * 180.0 / Pi; // Phase angle to degrees
        phase_deg_wrapped = fmod(phase_deg, 360.0); // Wrap phase angle to 0-360 degrees

        if (phase_deg_wrapped <= 90) {
            vChirp = lut_sin[(int)(phase_deg_wrapped)* x+1] * 2048 + 2048;
        } else if (phase_deg_wrapped <= 180) {
            vChirp = -lut_sin[(int)(180.0 - phase_deg_wrapped) * x+1] * 2048 + 2048;
        } else if (phase_deg_wrapped <= 270) {
            vChirp = -lut_sin[(int)(phase_deg_wrapped - 180.0) * x+1] * 2048 + 2048;
        } else {
            vChirp = lut_sin[(int)(360.0 - phase_deg_wrapped)* x+1] * 2048 + 2048;
        }
        
        array[i] = (uint16_t)vChirp;
    }

    for (int i = n; i < samples_int; i++) {
        array[i] = array[samples_int - i-1];
    }
}