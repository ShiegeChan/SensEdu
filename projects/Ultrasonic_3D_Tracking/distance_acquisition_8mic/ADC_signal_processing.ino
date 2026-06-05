/* -------------------------------------------------------------------------- */
/*                           ADC DATA REARRANGEMENT                           */
/* -------------------------------------------------------------------------- */
void get_channel_data(uint16_t* adc_array, uint16_t* ch_buf, const uint16_t ch_buf_size, const uint16_t total_ch_num, const uint8_t selected_ch) {
    for (uint16_t i = 0; i < ch_buf_size; i++) {
        // If this bottlenecks the execution, use DMA for data rearrangement or move it to MATLAB
        ch_buf[i] = adc_array[i * total_ch_num + selected_ch];
    }
}

/* -------------------------------------------------------------------------- */
/*                         CROSS-CORRELATION FUNCTION                         */
/* -------------------------------------------------------------------------- */
void custom_xcorr(float* xcorr_buf, const uint16_t* dac_wave, uint32_t adc_data_length) {
    // Delay loop
    for (int32_t m = 0; m < adc_data_length; m++) {
        // Sum loop
        float sum = 0;
        for (uint16_t n = 0; n < dac_wave_size; n++) {
            uint32_t idx = n + m;
            if (idx < adc_data_length) {
                sum += dac_wave[n] * xcorr_buf[idx];
            }
        }
        // Indexes never overlap with previous computation -> safe to reuse for memory management
        xcorr_buf[m] = sum;
    }
}

/* -------------------------------------------------------------------------- */
/*                         BANDPASS FILTERING FUNCTION                        */
/* -------------------------------------------------------------------------- */
void filter_32kHz_wave(float* rescaled_adc_wave, uint16_t adc_data_length) {
    static float32_t output_signal[STORE_BUF_SIZE];
    // Initialize this temporal buffer
    clear_float_buf(output_signal, STORE_BUF_SIZE);
    // Need to take block chunks of the input signal
    for (uint16_t i = 0; i < adc_data_length; i += FILTER_BLOCK_LENGTH) {
        // Take care of the last block
        uint32_t block_size = min(FILTER_BLOCK_LENGTH, adc_data_length - i);
        // Perform the filter operation for the current block
        arm_fir_f32(&Fir_filt, &rescaled_adc_wave[i], &output_signal[i], block_size);
    }

    // Copy the filtered signal to the rescaled_adc_wave
    memcpy(rescaled_adc_wave, output_signal, adc_data_length * sizeof(float));
}

/* -------------------------------------------------------------------------- */
/*                             RESCALING FUNCTION                             */
/* -------------------------------------------------------------------------- */
void rescale_adc_wave(float* rescaled_adc_wave, uint16_t* adc_wave, size_t adc_data_length) {
    // Data normalization 0:65535 -> -1:1
    for (uint16_t i = 0; i < adc_data_length; i++) {
        rescaled_adc_wave[i] = (2.0f * adc_wave[i]) / 65535.0f - 1.0f;
    }
}

/* -------------------------------------------------------------------------- */
/*                                BAN COUPLING                                */
/* -------------------------------------------------------------------------- */
void remove_coupling(float* adc_wave, const uint16_t banned_sample_num) {
    for (uint16_t i = 0; i < banned_sample_num; i++) {
        adc_wave[i] = 0;
    }
}

/* -------------------------------------------------------------------------- */
/*                             PEAK PROCESSING                            */
/* -------------------------------------------------------------------------- */

// Comparison function for sorting peaks in descending order
int compare_peaks(const void* a, const void* b) {
    Peak* peak_a = (Peak*)a;
    Peak* peak_b = (Peak*)b;
    if (peak_b->value > peak_a->value) return 1;
    if (peak_b->value < peak_a->value) return -1;
    return 0;
}

/* -------------------------------------------------------------------------- */
/*                             CALCULATE DISTANCES                            */
/* -------------------------------------------------------------------------- */
void calculate_distances(float* echo, uint16_t echo_length, uint32_t sampling_rate, uint32_t* dist_um) {

    // First, we need an envelope for a peak search, otherwise we'll see all the high-frequency peaks of the waveform
    static uint32_t enveloped_signal[STORE_BUF_SIZE];
    uint8_t window_size = 20;
    uint8_t half_window = window_size / 2;
    uint32_t current_max = 0;
    int max_index = -1;

    for (uint16_t i = 0; i < echo_length; i++) {
        uint32_t start = max(0, i - half_window);
        uint32_t end = min(echo_length - 1, i + half_window);
        if (max_index < start) {
            current_max = 0;
            for (int j = start; j <= end; j++) {
                uint32_t abs_val = (uint32_t)fabsf(echo[j]);
                if (abs_val >= current_max) {
                    current_max = abs_val;
                    max_index = j;
                }
            }
        } else {
            uint32_t new_val = (uint32_t)fabsf(echo[end]);
            if (new_val >= current_max) {
                current_max = new_val;
                max_index = end;
            }
        }
        enveloped_signal[i] = current_max;
    }

    // Then, we add a moving average to smooth the envelope and especially to remove flat parts:
    static uint32_t smoothed_buf[STORE_BUF_SIZE];

    if (smoothed_buf != NULL) {
        window_size = 50;
        half_window = window_size / 2;
        double runningSum = 0.0;
        int count = 0;

        for (size_t j = 0; j <= half_window && j < echo_length; j++) {
            runningSum += enveloped_signal[j];
            count++;
        }
        for (size_t i = 0; i < echo_length; i++) {
            smoothed_buf[i] = (uint32_t)(runningSum / count);

            int nextToEnter = i + half_window + 1;
            if (nextToEnter < echo_length) {
                runningSum += enveloped_signal[nextToEnter];
                count++;
            }
            int nextToLeave = i - half_window;
            if (nextToLeave >= 0) {
                runningSum -= enveloped_signal[nextToLeave];
                count--;
            }
        }
        memcpy(enveloped_signal, smoothed_buf, echo_length * sizeof(uint32_t));
    }

    // For the peak search on the envelope, we also consider a threshold relative to the max peak height.
    // We ll olny consider peaks which are X% of the maximum, e.g., 70% 
    uint32_t max_val = 0;
    for (size_t i = 0; i < echo_length; i++) {
        if (enveloped_signal[i] > max_val) {
            max_val = enveloped_signal[i];
        }
    }
    uint32_t threshold = (max_val * 7) / 10;
    static Peak temp_peaks[(STORE_BUF_SIZE / 2)];

    if (temp_peaks != NULL && max_val > 0) {
        int peakCount = 0;

        for (size_t i = 1; i < echo_length - 1; i++) {
            uint32_t current = enveloped_signal[i];

            if (current >= threshold) {
                if (current > enveloped_signal[i - 1] && current >= enveloped_signal[i + 1]) {
                    temp_peaks[peakCount].value = current;
                    temp_peaks[peakCount].location = i;
                    peakCount++;
                }
            }
        }

        if (peakCount > 0) {
            qsort(temp_peaks, peakCount, sizeof(Peak), compare_peaks);
        }

    }

    for (int p = 0; p < MAX_PEAKS; p++) {
        dist_um[p] = (float)temp_peaks[p].location * HALF_AIR_SPEED_UM_S / sampling_rate;
    }

}
