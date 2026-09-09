/*
 * Sawtooth chirp generator.
 *
 * Fills the DAC buffer with one linear up-sweep from START_FREQUENCY to
 * END_FREQUENCY, evaluated from a quarter-wave sine LUT.
 *
 * The end frequency is nudged so the sweep holds a whole number of cycles,
 * which keeps the waveform phase-continuous when the LUT repeats.
 */

const uint32_t LUT_RESOLUTION = 5; // Quarter-wave LUT points per degree
const float PI_F = 3.14159; // Arduino already defines PI as a double

void generate_sawtooth_chirp(uint16_t* array) {

    float lut_sine[90 * LUT_RESOLUTION]; // Quarter-wave LUT for sine values
    float chirp_value;
    float phase_deg; // Phase angle in degrees
    float phase_rad; // Phase angle in radians
    float phase_deg_wrapped; // Wrapped phase angle
    int cycle_num = round((START_FREQUENCY + END_FREQUENCY) / 2.0 * CHIRP_DURATION); // Closest integer number of cycles
    int end_frequency_adjusted = 2*cycle_num/CHIRP_DURATION - START_FREQUENCY; // Adjusted end frequency for int number of cycles
    float chirp_rate = (end_frequency_adjusted - START_FREQUENCY) / CHIRP_DURATION;

    // Generate the quarter-wave sine LUT
    for (int i = 0; i < 90 * LUT_RESOLUTION; i++) {
        phase_deg = (float)i/LUT_RESOLUTION;
        phase_rad = phase_deg * PI_F / 180.0;
        lut_sine[i] = sin(phase_rad-PI_F/2); // Store sine value in the LUT
    }

    // Generate the chirp signal
    for (int i = 0; i < samples_int; i++) {
        phase_rad = 2.0 * PI_F * (0.5 * chirp_rate * i / fs + START_FREQUENCY) * i / fs; // Phase angle in radians
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
}
