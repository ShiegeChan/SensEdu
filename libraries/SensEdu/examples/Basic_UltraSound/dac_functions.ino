// output sine wave from the defined look up table
void dac_output_sinewave(AdvancedDAC& dac_out) {
    for (uint16_t i = 0; i < dac_cycle_num; i++) {
        while(!(dac_out.available()));

        SampleBuffer buf = dac_out.dequeue();
        for (size_t i = 0; i < buf.size(); i++) {
            buf[i] = sine_lut[i];
        }

        dac_out.write(buf);
    }
}

// output sine wave from the defined look up table
void dac_output_halfrail(AdvancedDAC& dac_out) {
    while(!(dac_out.available()));

    SampleBuffer buf = dac_out.dequeue();
    for (size_t i = 0; i < buf.size(); i++) {
        buf[i] = 0x0800;
    }

    dac_out.write(buf);
}

// output zero
void dac_output_zero(AdvancedDAC& dac_out) {
    while(!(dac_out.available()));

    SampleBuffer buf = dac_out.dequeue();
    for (size_t i = 0; i < buf.size(); i++) {
        buf[i] = 0x0000;
    }

    dac_out.write(buf);
}
