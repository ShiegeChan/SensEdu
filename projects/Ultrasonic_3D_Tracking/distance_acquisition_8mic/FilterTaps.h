/*
 * FIR bandpass taps for the ultrasonic band, covering the 32 kHz transmit tone
 * and the ~32.8 kHz transducer resonance.
 *
 * Rejects the DC offset, audible-band disturbances and high-frequency noise
 * before cross-correlation.
 *
 * Measured response at Fs = 250 kHz: peak at 32.8 kHz, -3 dB band 28.7-36.9 kHz,
 * DC attenuated by about 30x.
 *
 * Scaled for unity gain at the peak, so the filtered signal keeps the same
 * amplitude scale as the input.
 */

#define FILTER_TAP_NUM 64

static float filter_taps[FILTER_TAP_NUM] = {
    -0.00091879f, -0.01806528f, -0.00259733f, 0.00032985f,
    0.00346258f, 0.00368990f, 0.00151953f, -0.00030350f,
    0.00039716f, 0.00249704f, 0.00220099f, -0.00308520f,
    -0.01076676f, -0.01360451f, -0.00537449f, 0.01213373f,
    0.02752208f, 0.02681140f, 0.00484648f, -0.02737571f,
    -0.04761017f, -0.03762265f, 0.00148293f, 0.04642679f,
    0.06566478f, 0.04192995f, -0.01315996f, -0.06435443f,
    -0.07609776f, -0.03785534f, 0.02688516f, 0.07556039f,
    0.07556039f, 0.02688516f, -0.03785534f, -0.07609776f,
    -0.06435443f, -0.01315996f, 0.04192995f, 0.06566478f,
    0.04642679f, 0.00148293f, -0.03762265f, -0.04761017f,
    -0.02737571f, 0.00484648f, 0.02681140f, 0.02752208f,
    0.01213373f, -0.00537449f, -0.01360451f, -0.01076676f,
    -0.00308520f, 0.00220099f, 0.00249704f, 0.00039716f,
    -0.00030350f, 0.00151953f, 0.00368990f, 0.00346258f,
    0.00032985f, -0.00259733f, -0.01806528f, -0.00091879f
};
