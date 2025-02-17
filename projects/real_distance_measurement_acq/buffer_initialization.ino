// buffer needs to be initialized because it crashes if you access an array element w/o initialization
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