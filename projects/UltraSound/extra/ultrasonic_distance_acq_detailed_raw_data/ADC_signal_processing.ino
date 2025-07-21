
/*------------------------------------------------------------------*/
/*                     MAIN MEASUREMENT FUNCTION                    */
/*------------------------------------------------------------------*/


uint32_t get_distance_measurement(float* xcorr_buf, size_t xcorr_buf_size, uint16_t* mic_array, size_t mic_array_size, const char* channel, uint8_t adc_ch_num, uint8_t ban_flag, float* modified_xcorr_buf, uint8_t trust) {
	rescale_adc_wave(xcorr_buf, mic_array, channel, mic_array_size, adc_ch_num);
    if (XCORR_DEBUG)
        serial_send_array((const uint8_t*)mic_array, mic_array_size, channel, adc_ch_num);

	// remove self reflections from a dataset
	if (ban_flag == 1) {
		for (uint32_t i = 0; i < banned_sample_num; i++) {
			xcorr_buf[i] = 0;
		}
	}
    if (XCORR_DEBUG)
        serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size, "b", 1);

    custom_xcorr(xcorr_buf, dac_wave, STORE_BUF_SIZE);
    if (XCORR_DEBUG)
	    serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size, "b", 1);
	
    // new logic
    /*float calculated_distance = calculate_distance(xcorr_buf);

    xcorr_modification_logic(xcorr_buf, modified_xcorr_buf);
    float assumed_distance = calculate_distance(modified_xcorr_buf);

    // compare
    if(compare_distances(calculated_distance, assumed_distance)) {
        trust++; 
        return assumed_distance;
    }
    else {
        trust = 0;
        return calculated_distance;
    }*/
    float calculated_distance = calculate_distance(xcorr_buf);
    return calculated_distance;
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
void rescale_adc_wave(float* rescaled_adc_wave, uint16_t* adc_wave, const char* channel, size_t adc_data_length, uint8_t adc_channel_num) {
    // 0:65535 -> -1:1
    char ch = channel[0];
    uint32_t cnt = 0;
    clear_float_buf(rescaled_adc_wave, STORE_BUF_SIZE);

    switch(adc_channel_num) {
        case 2:
            if(ch=='1') {
                for(uint32_t i = 0; i < 2 * STORE_BUF_SIZE; i+=2) {
                    rescaled_adc_wave[cnt] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
                    cnt++;
                }
            }
            else if(ch=='2') {
                for(uint32_t i = 1; i < 2 * STORE_BUF_SIZE; i+=2) {
                    rescaled_adc_wave[cnt] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
                    cnt++;
                }
            }
            break;
        case 3:
            if(ch=='1') {
                for(uint32_t i = 0; i < 3 * STORE_BUF_SIZE; i+=3) {
                    rescaled_adc_wave[cnt] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
                    cnt++;
                }
            }
            else if(ch=='2') {
                for(uint32_t i = 1; i < 3 * STORE_BUF_SIZE; i+=3) {
                    rescaled_adc_wave[cnt] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
                    cnt++;
                }
            }
            else if(ch=='3') {
                for(uint32_t i = 2; i < 3 * STORE_BUF_SIZE; i+=3) {
                    rescaled_adc_wave[cnt] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
                    cnt++;
                }
            }
            break;
        default:
            break;
    }
    filter_32kHz_wave(rescaled_adc_wave, STORE_BUF_SIZE);   
}
/*------------------------------------------------------------------*/
/*                     OBJECT DETECTION FUNCTION                    */
/*------------------------------------------------------------------*/

uint8_t object_detected(float* xcorr_buf, uint32_t adc_data_length) {
    for (int32_t m = 0; m < adc_data_length; m++) {
        if(xcorr_buf[m] > POST_FILTER_THRESHOLD) return 1;
    }
    return 0;
}

/*------------------------------------------------------------------*/
/*                     COMPARE DISTANCES                            */
/*------------------------------------------------------------------*/


uint8_t compare_distances(float calculated_distance, float assumed_distance) {
    // calculated_distance - distance we calculate after xcorr
    // assumed_distance - distance we get after xcorr modification and trust algorithm
    if (abs(calculated_distance - assumed_distance) < 0.0001) return 1; 

    return 0; // in case they are not the same return 0
}

/*------------------------------------------------------------------*/
/*                     COMPARE DISTANCES                            */
/*------------------------------------------------------------------*/
void xcorr_modification_logic(float* xcorr_buf, float* xcorr_buf_modified) {
    float buf_value = 0.0;
    for(uint16_t i = 0; i < STORE_BUF_SIZE; i++) {
        if(i < 400) {
            buf_value = 0.25 * xcorr_buf[i] + xcorr_buf[i];
        }
        else buf_value = xcorr_buf[i];
        xcorr_buf_modified[i] = buf_value;
    }
}

/*------------------------------------------------------------------*/
/*                     CALCULATE DISTANCES                          */
/*------------------------------------------------------------------*/

float calculate_distance(float *xcorr_buf) {
    uint32_t peak_index = 0;
	float biggest = 0.0f;

	for (uint32_t i = 0; i < STORE_BUF_SIZE; i++) {
		if (xcorr_buf[i] > biggest) {
			biggest = xcorr_buf[i];
			peak_index = i;
		}
	}
	uint16_t sr = ACTUAL_SAMPLING_RATE/1000; // kS/sec  sample rate
	uint16_t c = 343; // speed in air
	// (lag_samples * sample_time) * air_speed / 2
	uint32_t distance = ((peak_index * 1000 * c) / sr) >> 1; // in micrometers
    return distance;
}
