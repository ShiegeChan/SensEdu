/* -------------------------------------------------------------------------- */
/*                           ADC DATA REARRANGEMENT                           */
/* -------------------------------------------------------------------------- */

void get_channel_data(uint16_t* adc_array, uint16_t* ch_buf, const uint16_t ch_buf_size, const uint16_t total_ch_num, const uint8_t selected_ch) {
    for(uint16_t i = 0; i < ch_buf_size; i++) {
        // If this bottlenecks the execution, use DMA for data rearrangement or move it to MATLAB
        ch_buf[i] = adc_array[i*total_ch_num + selected_ch];
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
                sum += dac_wave[n]*xcorr_buf[idx]; 
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
    for(uint16_t i = 0; i < adc_data_length; i += FILTER_BLOCK_LENGTH) {
        // Take care of the last block
        size_t block_size = min(FILTER_BLOCK_LENGTH, adc_data_length - i);
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

/* -------------------------------------------------------------------------- */
/*                             CALCULATE DISTANCES                            */
/* -------------------------------------------------------------------------- */

// Comparison function for sorting peaks in descending order
int comparePeaks(const void* a, const void* b) {
    Peak* peakA = (Peak*)a;
    Peak* peakB = (Peak*)b;
    if (peakB->value > peakA->value) return 1;
    if (peakB->value < peakA->value) return -1;
    return 0;
}


//  float envelope_process(float input) {
//       static float envelopeValue = 0.0f;

//       float absoluteInput = abs(input); // Use abs() for the upper envelope
//       if (absoluteInput > envelopeValue) {
//         // Attack: Track rising peaks
//         envelopeValue = absoluteInput + 0.001 * (envelopeValue - absoluteInput);
//       } else {
//         // Release: Decay slowly
//         envelopeValue = absoluteInput + 0.01 * (envelopeValue - absoluteInput);
//       }

//       return envelopeValue;
//     }

// void envelopeBuffer(float* signal, float* output, int length) {
//   for (int i = 0; i < length; i++) {
//     // signal[i] is the same as *(signal + i)
//     output[i] = envelope_process(signal[i]);
//     Serial.println( output[i]);
//   }
// }


void calculate_distance_new(float* echo, uint16_t echo_length, uint32_t sampling_rate, uint32_t* dist_um) {
    uint16_t peak_index = 0u;
    float max_value = 0.0f;
    for (uint16_t i = 0u; i < echo_length; i++) {
        if (echo[i] > max_value) {
            max_value = echo[i];
            peak_index = i;
        }
    }


// // Envelope attempt 2
// float envelope_res[echo_length];
// envelopeBuffer(echo, envelope_res, echo_length);


// Envelope attempt 3
    int windowSize = 20;
    float envelope_res[echo_length];

    for (uint16_t i = 0u; i < echo_length; i++) {
        float maxVal = 0;
        
        // Look back and forward by half the window size
        int start = max(0, i - windowSize / 2);
        int end = min(echo_length - 1, i + windowSize / 2);
        
        for (int j = start; j <= end; j++) {
            float absVal = abs(echo[j]);
            if (absVal > maxVal) {
                maxVal = absVal;
            }
        }
        envelope_res[i] = maxVal;
        //Serial.println( envelope_res[i]);
    }
// add moving average to remove flat parts:
    int smoothWin = 50; 
    for (uint16_t i = 0u; i < echo_length; i++) {
        float sum = 0;
        int count = 0;
        int start = max(0, i - smoothWin / 2);
        int end = min(echo_length - 1, i + smoothWin / 2);
        
        for (int j = start; j <= end; j++) {
            sum += (envelope_res[j]/10000.0);
            count++;
        }
// check memory errors:
        if (envelope_res != nullptr && i < echo_length) {
            envelope_res[i] = sum / (float)count;
           // Serial.println(envelope_res[i]);
        } else {
            Serial.println("Error: Array pointer is null or index is out of bounds!");
        }
    }

//Single Peak search:
    float max_value_envelope = 0.0f;
    for (uint16_t i = 0u; i < echo_length; i++) {
        if (envelope_res[i] > max_value_envelope) {
            max_value_envelope = envelope_res[i];
        // peak_index = i;
        }
    }
    int tempCount = 0;
    float minPeakHeight = 0.7*max_value_envelope;
    // for (uint16_t i = 1u; i < echo_length - 1; i++) {
    //     if (envelope_res[i] > envelope_res[i-1] && envelope_res[i] >= envelope_res[i+1]) {
    //         if (envelope_res[i] >= minPeakHeight) {
    //             tempCount++;
    //         }
    //     }
    // }

 //Serial.println(tempCount);
// Multi peak search
    Peak* tempPeaks = (Peak*)malloc(tempCount * sizeof(Peak));
    if (tempPeaks == NULL) return;
    int idx = 0;
    for (uint16_t i = 1u; i < echo_length - 1; i++) {
        if (envelope_res[i] > envelope_res[i-1] && envelope_res[i] >= envelope_res[i+1]) {
            if (envelope_res[i] >= minPeakHeight) {
                tempPeaks[idx].value = envelope_res[i];
                tempPeaks[idx].location = i;
                idx++;
            }
        }
    }
    qsort(tempPeaks, idx, sizeof(Peak), comparePeaks); // sort them in ascending order. We'll take the first ones and check distances
    // Serial.println(idx);
    // Serial.print(tempPeaks[0].location);Serial.print(" ");Serial.println(tempPeaks[0].value);
    // Serial.print(tempPeaks[1].location);Serial.print(" ");Serial.println(tempPeaks[1].value);
    // Serial.print(tempPeaks[2].location);Serial.print(" ");Serial.println(tempPeaks[2].value);
//     for (int i = peaksToCopy; i < 3; i++) {
//     topThree[i].value = 0;
//     topThree[i].location = 0;
// }

    // float dist_um[MAX_PEAKS] = {0.0f, 0.0f, 0.0f};
    for (int p = 0; p < MAX_PEAKS; p++) {
         dist_um[p] = (float)tempPeaks[p].location * HALF_AIR_SPEED_UM_S / sampling_rate;
        //  Serial.println(dist_um[p]);
    }



    // // (lag_samples * sample_time) * air_speed / 2
    // float dist_um = (float)peak_index * HALF_AIR_SPEED_UM_S;
    // dist_um = (dist_um / sampling_rate);
    // return dist_um;
}
