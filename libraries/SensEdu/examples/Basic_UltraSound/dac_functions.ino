// output sine wave from the defined look up table
void dac_output_sinewave(AdvancedDAC& dac_out) {
    static size_t lut_offs = 0;
    for (uint16_t i = 0; i <= dac_cycle_num; i++) {
        if (dac_out.available()) {
            SampleBuffer buf = dac_out.dequeue();
            for (size_t i=0; i<buf.size(); i++, lut_offs++) {
                buf[i] = sine_lut[lut_offs % sine_lut_size];
            }
            
            dac_out.write(buf);
        }
    }
}

// outputs dc at half max value
void dac_output_dc_halfrail(AdvancedDAC& dac_out) {
    for (uint16_t i = 0; i <= dac_cycle_num; i++) {
        if (dac_out.available()) {
            SampleBuffer buf = dac_out.dequeue();
            for (size_t i=0; i<buf.size(); i++) {
                buf[i] = 0x0800;
            }
            
            dac_out.write(buf);
        }
    }
}