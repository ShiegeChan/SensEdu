/*
 * Buffer clearing helpers.
 *
 * Processing buffers are reused between channels, so they must be zeroed before
 * each pass rather than left holding the previous channel's data.
 */

void clear_float_buf(float array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0.0f;
    }
}

void clear_8bit_buf(uint8_t array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0x00;
    }
}

void clear_16bit_buf(uint16_t array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0x0000;
    }
}