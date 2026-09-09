/*
 * Triangular chirp generator.
 *
 * Fills the first half of the DAC buffer with a linear up-sweep from
 * START_FREQUENCY to END_FREQUENCY, then mirrors it into the second half to
 * form the down-sweep.
 *
 * The sweep is evaluated from a quarter-wave sine LUT. Because the up-sweep
 * only gets half the period, the chirp rate is doubled.
 */

const uint32_t LUT_RESOLUTION = 5; // Quarter-wave LUT points per degree
const float PI_F = 3.14159; // Arduino already defines PI as a double

void generate_triangular_chirp(uint16_t* array) {

    float lut_sine[90 * LUT_RESOLUTION]; // Quarter-wave LUT for sine values
    float chirp_value;
    int half_samples = round(samples_int/2);
    float phase_deg; // Phase angle in degrees
    float phase_rad; // Phase angle in radians
    float phase_deg_wrapped; // Wrapped phase angle
    float chirp_rate = 2*(END_FREQUENCY - START_FREQUENCY) / CHIRP_DURATION;

    // Generate the quarter-wave sine LUT
    for (int i = 0; i < 90 * LUT_RESOLUTION; i++) {
        phase_deg = (float)i * 90.0 / (90 * LUT_RESOLUTION); // Phase angle in degrees
        phase_rad = phase_deg * PI_F / 180.0; // Phase angle to radians
        lut_sine[i] = sin(phase_rad-PI_F/2); // Store sine value in the LUT
    }

    // Generate the up-sweep
    for (int i = 0; i < half_samples; i++) {
        phase_rad = 2.0 * PI_F * (0.5 * chirp_rate * (i - 1) / fs + START_FREQUENCY) * (i - 1) / fs; // Phase angle in radians
        phase_deg = phase_rad * 180.0 / PI_F; // Phase angle to degrees
        phase_deg_wrapped = fmod(phase_deg, 360.0); // Wrap phase angle to 0-360 degrees

        // Mirror the quarter wave into the matching quadrant, then scale to 12-bit
        if (phase_deg_wrapped <= 90) {
            chirp_value = lut_sine[(int)(phase_deg_wrapped)* LUT_RESOLUTION+1] * 2048 + 2048;
        } else if (phase_deg_wrapped <= 180) {
            chirp_value = -lut_sine[(int)(180.0 - phase_deg_wrapped) * LUT_RESOLUTION+1] * 2048 + 2048;
        } else if (phase_deg_wrapped <= 270) {
            chirp_value = -lut_sine[(int)(phase_deg_wrapped - 180.0) * LUT_RESOLUTION+1] * 2048 + 2048;
        } else {
            chirp_value = lut_sine[(int)(360.0 - phase_deg_wrapped)* LUT_RESOLUTION+1] * 2048 + 2048;
        }
        
        array[i] = (uint16_t)chirp_value;
    }

    // Mirror the up-sweep to form the down-sweep
    for (int i = half_samples; i < samples_int; i++) {
        array[i] = array[samples_int - i-1];
    }
}