/* -------------------------------------------------------------------------- */
/*                           ADC DATA REARRANGEMENT                           */
/* -------------------------------------------------------------------------- */
void get_channel_data(uint16_t* adc_array, uint16_t* ch_buf, const uint16_t ch_buf_size, const uint16_t total_ch_num, const uint8_t selected_ch) {
    for(uint16_t i = 0; i < ch_buf_size; i++) {
        // if this bottlenecks the execution, use DMA for data rearrangement or move it to MATLAB
        ch_buf[i] = adc_array[i*total_ch_num + selected_ch];
    }
}

/* -------------------------------------------------------------------------- */
/*                         CROSS-CORRELATION FUNCTION                         */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/*                         BANDPASS FILTERING FUNCTION                        */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/*                             RESCALING FUNCTION                             */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/*                             CALCULATE DISTANCES                            */
/* -------------------------------------------------------------------------- */
float calculate_distance(float* echo, uint16_t echo_length, uint32_t sampling_rate) {
    uint16_t peak_index = 0u;
    float max_value = 0.0f;
    for (uint16_t i = 0u; i < echo_length; i++) {
        if (echo[i] > max_value) {
            max_value = echo[i];
            peak_index = i;
        }
    }
    
    // (lag_samples * sample_time) * air_speed / 2
    float dist_um = (float)peak_index * HALF_AIR_SPEED_UM_S;
    dist_um = (dist_um / sampling_rate);
    return dist_um;
}




/* -------------------------------------------------------------------------- */
/*                             FUNCTIONS FOR PEAK SEARCH, ENVELOPE, ETC                            */
/* -------------------------------------------------------------------------- */


// Comparison function for sorting peaks in descending order
int comparePeaks(const void* a, const void* b) {
    Peak* peakA = (Peak*)a;
    Peak* peakB = (Peak*)b;
    if (peakB->value > peakA->value) return 1;
    if (peakB->value < peakA->value) return -1;
    return 0;
}

// // Simple envelope detection using peak algorithm
// void calculateEnvelope(float* input, float* output, int length, int windowSize) {
//     // Copy input to output initially
//     memcpy(output, input, length * sizeof(float));
    
//     // Simple peak envelope: for each point, take max in window
//     for (int i = 0; i < length; i++) {
//         int start = max(0, i - windowSize);
//         int end = min(length - 1, i + windowSize);
        
//         float maxVal = input[start];
//         for (int j = start + 1; j <= end; j++) {
//             if (input[j] > maxVal) {
//                 maxVal = input[j];
//             }
//         }
//         output[i] = maxVal;
//     }
// }

void calculateEnvelope(float* input, float* output, int length, int windowSize) {
Serial.println(*input);
Serial.println(windowSize);
Serial.println(input[0]);
Serial.println(input[10]);
Serial.println(length);


    for (int i = 0; i < length; i++) {
        int start = max(0, i - windowSize);
        int end = min(length - 1, i + windowSize);
        float maxVal = input[start];
        for (int j = start + 1; j <= end; j++) {
            if (input[j] > maxVal) maxVal = input[j];
        }
        output[i] = maxVal;
    }
}

int findPeaks(float* signal, int length, Peak* peaks,
              int minPeakDistance, float minPeakHeight) {
    // Temporary storage for all peaks found
// First pass: count peaks to allocate temporary storage
//Serial.println(*signal);
Serial.println(minPeakHeight);


    int tempCount = 0;
    for (uint16_t i = 1u; i < length - 1; i++) {
        if (signal[i] > signal[i-1] && signal[i] > signal[i+1]) {
            if (signal[i] >= minPeakHeight) {
                tempCount++;
            }
        }
    }
    if (tempCount == 0) return 0;

// uint16_t peak_index = 0u;
//     float max_value = 0.0f;
//     for (uint16_t i = 0u; i < echo_length; i++) {
//         if (echo[i] > max_value) {
//             max_value = echo[i];
//             peak_index = i;
//         }
//     }



    Serial.println("Test_count");

  Peak* tempPeaks = (Peak*)malloc(tempCount * sizeof(Peak));
    if (tempPeaks == NULL) return 0;

// Second pass: fill tempPeaks
    int idx = 0;
    for (int i = 1; i < length - 1; i++) {
        if (signal[i] > signal[i-1] && signal[i] > signal[i+1]) {
            if (signal[i] >= minPeakHeight) {
                tempPeaks[idx].value = signal[i];
                tempPeaks[idx].location = i;
                idx++;
            }
        }
    }



    // Peak tempPeaks[length];  // OK for Arduino? length might be large – use dynamic?
    // // For safety, we could use a smaller fixed size, but we'll assume length is moderate.
    // // If length is large, consider using malloc. For now, stack allocation is used.
    // int tempCount = 0;

    // // Find all local maxima above threshold
    // for (int i = 1; i < length - 1; i++) {
    //     if (signal[i] > signal[i-1] && signal[i] > signal[i+1]) {
    //         if (signal[i] >= minPeakHeight) {
    //             tempPeaks[tempCount].value = signal[i];
    //             tempPeaks[tempCount].location = i;
    //             tempCount++;
    //         }
    //     }
    // }

    // Sort all peaks by value descending
    qsort(tempPeaks, tempCount, sizeof(Peak), comparePeaks);

    // Now select top MAX_PEAKS while respecting minPeakDistance
    int finalCount = 0;
    for (int i = 0; i < tempCount && finalCount < MAX_PEAKS; i++) {
        bool tooClose = false;
        for (int j = 0; j < finalCount; j++) {
            if (abs(tempPeaks[i].location - peaks[j].location) < minPeakDistance) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            peaks[finalCount++] = tempPeaks[i];
        }
    }

    free(tempPeaks);
    return finalCount;
}

// // Find peaks in the signal
// int findPeaks(float* signal, int length, Peak* peaks, int maxPeaks, 
//               int minPeakDistance, float minPeakHeight) {
//     int peakCount = 0;
    
//     for (int i = 1; i < length - 1; i++) {
//         // Check if current point is a peak (greater than neighbors)
//         if (signal[i] > signal[i-1] && signal[i] > signal[i+1]) {
            
//             // Check minimum height condition
//             if (signal[i] >= minPeakHeight) {
                
//                 // Check minimum distance from previous peaks
//                 bool validPeak = true;
//                 // for (int j = 0; j < peakCount; j++) {
//                 //     if (abs(i - peaks[j].location) < minPeakDistance) {
//                 //         // Keep the higher peak
//                 //         if (signal[i] > peaks[j].value) {
//                 //             // Replace the lower peak
//                 //             peaks[j].value = signal[i];
//                 //             peaks[j].location = i;
//                 //         }
//                 //         validPeak = false;
//                 //         break;
//                 //     }
//                 // }
                
//                 if (validPeak && peakCount < maxPeaks) {
//                     peaks[peakCount].value = signal[i];
//                     peaks[peakCount].location = i;
//                     peakCount++;
//                 }
//             }
//         }
//     }
    
//     return peakCount;
// }



void calculate_distance_new(float* echo,            //CC output
                       uint16_t echo_length, 
                       //float tol,            // tolerance for peak location difference
                       int windowSize,       // envelope window
                       int minPeakDistance,
                       uint32_t sampling_rate, //min distance between peaks
                       float dist_um[3])  // output
{

Serial.println(*echo);
Serial.println(windowSize);
Serial.println(echo[0]);
Serial.println(echo[10]);
Serial.println(echo[100]);
Serial.println(dist_um[0]);


    // Allocate working arrays
    float* envelope = (float*)malloc(echo_length * sizeof(float));
    Peak peaks[MAX_PEAKS];

    int previous_loc = -1;   // -1 indicates undefined

    // for (int i = 0; i < d1; i++) {
        // // Extract slice echo(i, 1, 3, :)  (using 0‑based indices: j=1, k=3)
        // float* signal = (float*)malloc(echo_length * sizeof(float));
        // for (int l = 0; l < echo_length; l++) {
        //     signal[l] = echo[IDX(i, 1, 3, l, d2, d3, echo_length)];
        // }


    // Compute envelope
    calculateEnvelope(echo, envelope, echo_length, windowSize);
Serial.print("Envelope done");
    // Determine threshold: 0.7 * max of envelope (from index 200 onward if we mimic matlab but let s add it later)
    int startIdx = min(0, echo_length - 1); // change 0 to 200 in case
    float maxVal = envelope[startIdx];

    Serial.print("Max val envelope");
    Serial.println(maxVal);


    for (int j = startIdx + 1; j < echo_length; j++) {
        if (envelope[j] > maxVal) maxVal = envelope[j];
    }
    float minPeakHeight = 0.7f * maxVal;

    // Find peaks (up to MAX_PEAKS); notice we have the Struct which stores the peak information to be used later
    int numPeaks = findPeaks(envelope, echo_length, peaks, minPeakDistance, minPeakHeight);

    // We could do some check on how the peak location is varying wrt the previous step, but
    // it's simpler to just send out N peaks (i.e., N distances) with N = 3 maybe. Note that the peaks
    // should be sorted. 




    // // Handle first iteration separately
    // if (i == 0) {
    //     if (numPeaks > 0) {
    //         previous_loc = peaks[0].location;
    //         Serial.print("First peak at location ");
    //         Serial.println(previous_loc);
    //     } else {
    //         Serial.println("No peaks found in first iteration.");
    //     }
    // } else {
    //     // Find a peak whose location is within tolerance of previous_loc
    //     int chosenIndex = -1;
    //     for (int p = 0; p < numPeaks; p++) {
    //         if (abs(peaks[p].location - previous_loc) <= tol) {
    //             chosenIndex = p;
    //             break;
    //         }
    //     }
    //     if (chosenIndex >= 0) {
    //         previous_loc = peaks[chosenIndex].location;
    //         Serial.print("Iteration ");
    //         Serial.print(i);
    //         Serial.print(": selected peak at ");
    //         Serial.print(previous_loc);
    //         Serial.print(" (value ");
    //         Serial.print(peaks[chosenIndex].value);
    //         Serial.println(")");
    //     } else {
    //         Serial.print("Iteration ");
    //         Serial.print(i);
    //         Serial.println(": no peak within tolerance.");
    //     }
    // }

    // free(signal);
    // // }

    // free(envelope);


    // float dist_um = (float)peak_index * HALF_AIR_SPEED_UM_S;
    // dist_um = (dist_um / sampling_rate);

 // Compute distances for up to 3 peaks (pad with zeros)
   // float dist_um[3] = {0.0f, 0.0f, 0.0f};
    for (int p = 0; p < numPeaks && p < 3; p++) {
        dist_um[p] = (float)peaks[p].location * HALF_AIR_SPEED_UM_S / sampling_rate;
    }


//    free(signal);
    free(envelope);

  //  return dist_um;
}


















// // Main function to process peaks across iterations
// void processPeakSearch(float*** echo, int echo_size_1, int echo_size_2, int echo_size_3, 
//                        int echo_size_4, float tol, int windowSize, int minPeakDistance) {
    
//     // Allocate memory for envelope
//     float* echo_env = (float*)malloc(echo_size_4 * sizeof(float));
//     Peak* peaks = (Peak*)malloc(echo_size_4 * sizeof(Peak)); // Max possible peaks
    
//     int previous_loc = -1;
    
//     for (int i = 0; i < echo_size_1; i++) {
//         // Extract the data slice echo(i,1,3,:)
//         float* signal = (float*)malloc(echo_size_4 * sizeof(float));
//         for (int k = 0; k < echo_size_4; k++) {
//             signal[k] = echo[i][1][k]; // Assuming 3rd dimension is accessed like this
//         }
        
//         // Calculate envelope
//         calculateEnvelope(signal, echo_env, echo_size_4, windowSize);
        
//         // Calculate threshold (0.7 * max of envelope from index 200 onward)
//         int startIdx = min(200, echo_size_4 - 1);
//         float maxVal = echo_env[startIdx];
//         for (int j = startIdx + 1; j < echo_size_4; j++) {
//             if (echo_env[j] > maxVal) {
//                 maxVal = echo_env[j];
//             }
//         }
//         float minPeakHeight = 0.7 * maxVal;
        
//         // Find peaks
//         int numPeaks = findPeaks(echo_env, echo_size_4, peaks, echo_size_4, 
//                                  minPeakDistance, minPeakHeight);
        
//         // Sort peaks in descending order
//         qsort(peaks, numPeaks, sizeof(Peak), comparePeaks);
        
//         // Handle first iteration
//         if (i == 0) {
//             if (numPeaks > 0) {
//                 previous_loc = peaks[0].location;
//                 Serial.print("First peak location: ");
//                 Serial.println(previous_loc);
//             }
//         } else {
//             // Find peak closest to previous location
//             int closest_idx = -1;
//             float min_diff = 1e9;
            
//             for (int j = 0; j < numPeaks; j++) {
//                 float diff = abs(peaks[j].location - previous_loc);
//                 if (diff <= tol) {
//                     closest_idx = j;
//                     break; // Found first peak within tolerance
//                 }
//             }
            
//             if (closest_idx >= 0) {
//                 previous_loc = peaks[closest_idx].location;
//                 Serial.print("Iteration ");
//                 Serial.print(i);
//                 Serial.print(": Selected peak at location ");
//                 Serial.print(previous_loc);
//                 Serial.print(" with value ");
//                 Serial.println(peaks[closest_idx].value);
//             } else {
//                 Serial.print("Iteration ");
//                 Serial.print(i);
//                 Serial.println(": No peak found within tolerance");
//             }
//         }
        
//         free(signal);
//     }
    
//     free(echo_env);
//     free(peaks);
// }


// float calculate_distance_new(float* echo, uint16_t echo_length, uint32_t sampling_rate) {

//     // we want to find ALL the local maxima in the CC curve. We can also restrict the 
//     // search to the peaks above a certain variable threshold. We can vary the threshold 
//     // to be X% above the average (taking care of the fact that we have a double-sided curve
//     // symmetric along the vertical axis)
//     static int prev_peak_idx = -1;  // remembered across calls
//     static bool start = true;
//     float max_value = 0.0f;
//     int   best_peak_idx = -1;

//     if (start== 1) {
//         for (uint16_t i = 0u; i < echo_length; i++) {
//             if (echo[i] > max_value) {
//                 max_value = echo[i];
//                 best_peak_idx = i;
//             }
//         }
//         prev_peak_idx = best_peak_idx;
//         start = 0;
//     } else {


//         double sum = 0.0;
//         double ave = 0.0;
//         for (int i = 0; i < echo_length; i++) {
//             sum += echo[i];
//         }
//         ave = sum / echo_length;
//         for (int i = 0; i < echo_length; i++) {
//         echo[i] = (echo[i] - ave); // * (echo[i] - ave);// remove mean
//         } 


//         // int* peak_indices;
//         // int *peak_count;

//         // *peak_count=0;

//         double threshold = 0.0*ave;

//         for (int i = 1; i < echo_length - 1; i++) {
//             // Check if the current element is a local maximum
//             if (echo[i] > echo[i - 1] && echo[i] > echo[i + 1]) {
//                 // Check if the local maximum exceeds the threshold
//                 if (echo[i] >= threshold) {

//                         // JUST FOR NOW: compare the location to the one before. We want to exclude sudden "jumps" in the peak's location
//                         // later we will need to include all peak because if I now get the wrong one it will keep failing...
//                         if (prev_peak_idx >= 0 && i >= 0) {
//                             int diff = i - prev_peak_idx;  
//                             if (abs(diff) <= 20)
//                                 best_peak_idx = i;
        
//                         }
//                     // peaks_indices[*peak_count] = i; 
//                     // (*peak_count)++;

//                     prev_peak_idx = best_peak_idx;
                    
//                 }
//             }
//         }
//     }




//     // uint16_t peak_index = 0u;
//     // float max_value = 0.0f;
//     // for (uint16_t i = 0u; i < echo_length; i++) {
//     //     if (echo[i] > max_value) {
//     //         max_value = echo[i];
//     //         peak_index = i;
//     //     }
//     // }
    
//     // (lag_samples * sample_time) * air_speed / 2
//     float dist_um = (float)best_peak_idx * HALF_AIR_SPEED_UM_S;
//     dist_um = (dist_um / sampling_rate);
//     return dist_um;
// }


