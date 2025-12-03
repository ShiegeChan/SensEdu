/* -------------------------------------------------------------------------- */
/*                           ADC DATA REARRANGEMENT                           */
/* -------------------------------------------------------------------------- */
void get_channel_data(uint16_t* adc_array, uint16_t* ch_buf, const uint16_t ch_buf_size, const uint16_t total_ch_num, const uint8_t selected_ch) {
    for(uint16_t i = 0; i < ch_buf_size; i++) {
        // if this bottlenecks the execution, use DMA for data rearrangement or move it to MATLAB
        ch_buf[i] = adc_array[i*total_ch_num + selected_ch];
    }
}

/*------------------------------------------------------------------*/
/*                     CROSS-CORRELATION FUNCTION                   */
/*------------------------------------------------------------------*/
void custom_xcorr(float* xcorr_buf, const uint16_t* dac_wave, uint32_t adc_data_length) {
    // delay loop
    for (int32_t m = 0; m < adc_data_length; m++) {
        // sum loop
        float sum = 0;
        for (uint16_t n = 0; n < dac_wave_size; n++) {
            uint32_t idx = n + m;
            if (idx < adc_data_length) {
                sum += dac_wave[n]*xcorr_buf[idx]; 
            }
        }
        // indexes never overlap with previous computation -> safe to reuse for memory management
        xcorr_buf[m] = sum; 
    }
}

/*------------------------------------------------------------------*/
/*                     BANDPASS FILTERING FUNCTION                  */
/*------------------------------------------------------------------*/
void filter_32kHz_wave(float* rescaled_adc_wave, uint16_t adc_data_length) {
    static float32_t output_signal[STORE_BUF_SIZE];
    // initialize this temporal buffer
    clear_float_buf(output_signal, STORE_BUF_SIZE);
    // need to take block chunks of the input signal
    for(uint16_t i = 0; i < adc_data_length; i += FILTER_BLOCK_LENGTH) {
        // take care of the last block
        size_t block_size = min(FILTER_BLOCK_LENGTH, adc_data_length - i);
        // perform the filter operation for the current block
        arm_fir_f32(&Fir_filt, &rescaled_adc_wave[i], &output_signal[i], block_size);
    }

    // copy the filtered signal to the rescaled_adc_wave
    memcpy(rescaled_adc_wave, output_signal, adc_data_length * sizeof(float));
}

/*------------------------------------------------------------------*/
/*                     RESCALING FUNCTION                           */
/*------------------------------------------------------------------*/
void rescale_adc_wave(float* rescaled_adc_wave, uint16_t* adc_wave, size_t adc_data_length) {
    // 0:65535 -> -1:1
    for(uint16_t i = 0; i < adc_data_length; i++) {
        rescaled_adc_wave[i] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
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

/*------------------------------------------------------------------*/
/*                     CALCULATE DISTANCES                          */
/*------------------------------------------------------------------*/
float calculate_distance(float *xcorr_buf) {
    uint32_t peak_index = 0, pocni = 0, cnt = 0;
	float biggest = 0.0f;
    uint16_t sr = ACTUAL_SAMPLING_RATE/1000; // kS/sec  sample rate
    uint32_t distance = 0;
    while(cnt!=STORE_BUF_SIZE) {
        for (uint32_t i = pocni; i < STORE_BUF_SIZE; i++) {
            if (xcorr_buf[i] > biggest) {
                biggest = xcorr_buf[i];
                peak_index = i;
            }
        }
        
        // (lag_samples * sample_time) * air_speed / 2
        // peak index is in kilosamples. This math manover makes the samples come in micrometers 
        distance = ((peak_index * 1000 * air_speed) / sr) >> 1; // in micrometers
        if (abs((distance / 1000000) - BAN_DISTANCE/100) > 0.0000001) {
            return distance;
        }
        pocni = peak_index + 10; // offset it so it can get a real size
        cnt++;
    }
    return distance;
}