void serial_send_array(const uint8_t* data, size_t size, const char* channel, uint8_t adc_total_ch_num) {
    const size_t chunk_size = 32; // buffer is 32 bytes, but 32 for 2400 data samples
    if (adc_total_ch_num == 3) {
        if (channel[0] == '1') {
            // first extract the data 
            static uint8_t ch1[2*STORE_BUF_SIZE]; 
            // initialize the buffer
            clear_8bit_buf(ch1, 2*STORE_BUF_SIZE);
            uint16_t cnt = 0;
            for(uint16_t i = 0; i < size; i+=6) {
                ch1[cnt++] = data[i];
                ch1[cnt++] = data[i+1];
            }
            // send the data in chunks of 32
            size /= 2;
            for (uint16_t i = 0; i < size/chunk_size; i++) {
                Serial.write(ch1 + chunk_size * i, chunk_size);
            }
            return;
        }
        else if(channel[0] == '2') {
            // first extract the data 
            uint16_t cnt = 0;
            static uint8_t ch2[2*STORE_BUF_SIZE]; 
            clear_8bit_buf(ch2, 2*STORE_BUF_SIZE);
            for(uint16_t i = 0; i < size; i+=6) {
                ch2[cnt++] = data[i+2];
                ch2[cnt++] = data[i+3];
            }
            // send the data in chunks of 32
            size /= 2;
            for (uint16_t i = 0; i < size/chunk_size; i++) {
                Serial.write(ch2 + chunk_size * i, chunk_size);
            }
            return;
        }
        else if(channel[0] == '3') {
            // first extract the data 
            uint16_t cnt = 0;
            static uint8_t ch3[2*STORE_BUF_SIZE]; 
            clear_8bit_buf(ch3, 2*STORE_BUF_SIZE);
            for(uint16_t i = 0; i < size; i+=6) {
                ch3[cnt++] = data[i+4];
                ch3[cnt++] = data[i+5];
            }
            // send the data in chunks of 32
            size /= 2;
            for (uint16_t i = 0; i < size/chunk_size; i++) {
                Serial.write(ch3 + chunk_size * i, chunk_size);
            }
            return;
        } 
    }
    else if (adc_total_ch_num == 2) {
        if (channel[0] == '1') {
            // first extract the data 
            static uint8_t ch1[2*STORE_BUF_SIZE]; 
            // initialize the buffer
            clear_8bit_buf(ch1, 2*STORE_BUF_SIZE);
            uint16_t cnt = 0;
            for(uint16_t i = 0; i < size; i+=4) {
                ch1[cnt++] = data[i];
                ch1[cnt++] = data[i+1];
            }
            // send the data in chunks of 32
            size /= 2;
            for (uint16_t i = 0; i < size/chunk_size; i++) {
                Serial.write(ch1 + chunk_size * i, chunk_size);
            }
            return;
        }
        else if(channel[0] == '2') {
            // first extract the data 
            uint16_t cnt = 0;
            static uint8_t ch2[2*STORE_BUF_SIZE]; 
            clear_8bit_buf(ch2, 2*STORE_BUF_SIZE);
            for(uint16_t i = 0; i < size; i+=4) {
                ch2[cnt++] = data[i+2];
                ch2[cnt++] = data[i+3];
            }
            // send the data in chunks of 32
            size /= 2;
            for (uint16_t i = 0; i < size/chunk_size; i++) {
                Serial.write(ch2 + chunk_size * i, chunk_size);
            }
            return;
        }
    }
    else {
        // normal data send
        for (uint16_t i = 0; i < size/chunk_size; i++) {
            Serial.write(data + chunk_size * i, chunk_size);
        }
        return;
    }   
}