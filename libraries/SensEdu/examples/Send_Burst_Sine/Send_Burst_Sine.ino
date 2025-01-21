#include <Arduino_AdvancedAnalog.h>

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

#define DAC_PIN          	A12
#define DAC_SINE_FREQ     	32000                   // 32kHz
#define DAC_RESOLUTION    	AN_RESOLUTION_12        // 12bit
#define DAC_SAMPLE_RATE    	DAC_SINE_FREQ * 64      // ~2MHz
#define DAC_SAMPLES_PER_CH	sine_lut_size    	    // samples in each buffer (one sine wave)
#define DAC_QUEUE_DEPTH 	10                      // queue depth

AdvancedDAC dac0(DAC_PIN);

// how many LUT repeats for one DAC transfer
const uint16_t dac_cycle_num = 10;

// DAC transfered symbols
const uint16_t sine_lut[] = {
    0x0000,0x000a,0x0027,0x0058,0x009c,0x00f2,0x0159,0x01d1,0x0258,0x02ed,0x038e,0x043a,0x04f0,0x05ad,0x0670,0x0737,
	0x0800,0x08c8,0x098f,0x0a52,0x0b0f,0x0bc5,0x0c71,0x0d12,0x0da7,0x0e2e,0x0ea6,0x0f0d,0x0f63,0x0fa7,0x0fd8,0x0ff5,
	0x0fff,0x0ff5,0x0fd8,0x0fa7,0x0f63,0x0f0d,0x0ea6,0x0e2e,0x0da7,0x0d12,0x0c71,0x0bc5,0x0b0f,0x0a52,0x098f,0x08c8,
	0x0800,0x0737,0x0670,0x05ad,0x04f0,0x043a,0x038e,0x02ed,0x0258,0x01d1,0x0159,0x00f2,0x009c,0x0058,0x0027,0x000a
};
const size_t sine_lut_size = sizeof(sine_lut) / sizeof(sine_lut[0]);

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);
    dac0.begin(DAC_RESOLUTION, DAC_SAMPLE_RATE, DAC_SAMPLES_PER_CH, DAC_QUEUE_DEPTH);
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    for (uint16_t i = 0; i < dac_cycle_num; i++) {
        dac_output_sinewave(dac0);
    }
    //dac_output_zero(dac0);

    // do something
    delay(100);
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */
void dac_output_sinewave(AdvancedDAC& dac_out) {
    while(!(dac_out.available()));

    SampleBuffer buf = dac_out.dequeue();
    for (size_t i = 0; i < buf.size(); i++) {
        buf[i] = sine_lut[i];
    }

    dac_out.write(buf);
}

void dac_output_zero(AdvancedDAC& dac_out) {
    while(!(dac_out.available()));

    SampleBuffer buf = dac_out.dequeue();
    for (size_t i = 0; i < buf.size(); i++) {
        buf[i] = 0;
    }

    dac_out.write(buf);
}
