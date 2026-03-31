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
int comparePeaks(const void* a, const void* b) {
  Peak* peakA = (Peak*)a;
  Peak* peakB = (Peak*)b;
  if (peakB->value > peakA->value) return 1;
  if (peakB->value < peakA->value) return -1;
  return 0;
}

/* -------------------------------------------------------------------------- */
/*                             CALCULATE DISTANCES                            */
/* -------------------------------------------------------------------------- */
void calculate_distance_new(float* echo, uint16_t echo_length, uint32_t sampling_rate, uint32_t* dist_um) {

  // Leaving here the old code (before Barnaba) just for reference:
  // uint16_t peak_index = 0u;
  // float max_value = 0.0f;
  // for (uint16_t i = 0u; i < echo_length; i++) {
  //     if (echo[i] > max_value) {
  //         max_value = echo[i];
  //         peak_index = i;
  //     }
  // }

  // First, we need an envelope for a peak search, otherwise we'll see all the high-frequency peaks of the waveform
  uint32_t enveloped_signal[echo_length];
  uint8_t windowSize = 20;
  uint8_t halfWindow = windowSize / 2;
  uint32_t currentMax = 0;
  int maxIndex = -1;

  for (int i = 0; i < echo_length; i++) {
    int start = max(0, i - halfWindow);
    int end = min(echo_length - 1, i + halfWindow);
    if (maxIndex < start) {
      currentMax = 0;
      for (int j = start; j <= end; j++) {
        uint32_t absVal = abs(echo[j]);
        if (absVal >= currentMax) {
          currentMax = absVal;
          maxIndex = j;
        }
      }
    } else {
      uint32_t newVal = abs(echo[end]);
      if (newVal >= currentMax) {
        currentMax = newVal;
        maxIndex = end;
      }
    }
    enveloped_signal[i] = currentMax;
    // Serial.println(enveloped_signal[i]);
  }

    // Then, we add a moving average to smooth the envelope and especially to remove flat parts:
    uint32_t* smoothed = (uint32_t*)malloc(echo_length * sizeof(uint32_t));
    if (smoothed != NULL) {
        int windowSize = 50;
        int halfWin = windowSize/2;
        double runningSum = 0.0; 
        int count = 0;

        for (int j = 0; j <= halfWin && j < echo_length; j++) {
            runningSum += enveloped_signal[j];
            count++;
        }
        for (int i = 0; i < echo_length; i++) {
            smoothed[i] = (uint32_t)(runningSum / count);

            int nextToEnter = i + halfWin + 1;
            if (nextToEnter < echo_length) {
                runningSum += enveloped_signal[nextToEnter];
                count++;
            }
            int nextToLeave = i - halfWin;
            if (nextToLeave >= 0) {
                runningSum -= enveloped_signal[nextToLeave];
                count--;
            }
        }
        memcpy(enveloped_signal, smoothed, echo_length * sizeof(uint32_t));
        free(smoothed);
    }


// For the peak search on the envelope, we also consider a threshold relative to the max peak height.
// We ll olny consider peaks which are X% of the maximum, e.g., 70% 
uint32_t max_val = 0;
for (uint32_t i = 0; i < echo_length; i++) {
    if (enveloped_signal[i] > max_val) {
        max_val = enveloped_signal[i];
    }
}
uint32_t threshold = (max_val * 7) / 10;

// echo_length / 2 is the theoretical maximum number of peaks possible
Peak* tempPeaks = (Peak*)malloc((echo_length / 2) * sizeof(Peak));

if (tempPeaks != NULL && max_val > 0) {
    int peakCount = 0;

    for (uint32_t i = 1; i < echo_length - 1; i++) {
        uint32_t current = enveloped_signal[i];

        if (current >= threshold) {
            if (current > enveloped_signal[i - 1] && current >= enveloped_signal[i + 1]) {
                tempPeaks[peakCount].value = current;
                tempPeaks[peakCount].location = i;
                peakCount++;
            }
        }
    }

    if (peakCount > 0) {
        qsort(tempPeaks, peakCount, sizeof(Peak), comparePeaks);
    }

}

// // Peak search OLD (slower)
//   uint32_t max_value_envelope = 0;
//   for (uint16_t i = 0u; i < echo_length; i++) {
//     if (enveloped_signal[i] > max_value_envelope) {
//       max_value_envelope = enveloped_signal[i];
//       // peak_index = i;
//     }
//   }
//   int tempCount = 0;
//   float minPeakHeight = 0.7 * max_value_envelope;

//   //Serial.println(tempCount);
//   // Multi peak search
//   Peak* tempPeaks = (Peak*)malloc(tempCount * sizeof(Peak));
//   if (tempPeaks == NULL) return;
//   int idx = 0;
//   for (uint16_t i = 1u; i < echo_length - 1; i++) {
//     if (enveloped_signal[i] > enveloped_signal[i - 1] && enveloped_signal[i] >= enveloped_signal[i + 1]) {
//       if ((float)enveloped_signal[i] >= minPeakHeight) {
//         tempPeaks[idx].value = enveloped_signal[i];
//         tempPeaks[idx].location = i;
//         idx++;
//       }
//     }
//   }
//   qsort(tempPeaks, idx, sizeof(Peak), comparePeaks);  // sort them in ascending order. We'll take the first ones and check distances
//   Serial.println(idx);
//   Serial.print(tempPeaks[0].location);Serial.print(" ");Serial.println(tempPeaks[0].value);
//   Serial.print(tempPeaks[1].location);Serial.print(" ");Serial.println(tempPeaks[1].value);
//   Serial.print(tempPeaks[2].location);Serial.print(" ");Serial.println(tempPeaks[2].value);

  for (int p = 0; p < MAX_PEAKS; p++) {
    dist_um[p] = (float)tempPeaks[p].location * HALF_AIR_SPEED_UM_S / sampling_rate;
    //  Serial.println(dist_um[p]);
  }
free(tempPeaks);

  // old
  // // (lag_samples * sample_time) * air_speed / 2
  // float dist_um = (float)peak_index * HALF_AIR_SPEED_UM_S;
  // dist_um = (dist_um / sampling_rate);
  // return dist_um;




}
