// initialize main structure
void main_obj_init(SenseduBoard* obj_ptr) {
	obj_ptr->ban_flag = 1;
	obj_ptr->serial_read_buf = '0';
    clear_float_buf(obj_ptr->processing_buffer, STORE_BUF_SIZE);
    clear_16bit_buf(obj_ptr->channel_buffer, STORE_BUF_SIZE);
}