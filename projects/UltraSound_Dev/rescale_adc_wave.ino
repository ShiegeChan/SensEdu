// rescale microphone data from adc 
#ifndef RESCALE_ADC_WAVE_H
#define RESCALE_ADC_WAVE_H

void rescale_adc_wave(float* rescaled_adc_wave, uint16_t* adc_wave, const char* channel, size_t adc_data_length);

#endif