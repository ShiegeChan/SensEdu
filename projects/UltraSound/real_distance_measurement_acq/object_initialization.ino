// initialize main structure
void main_obj_init(SenseduBoard* obj_ptr) {
	obj_ptr->ban_flag = 1;
	obj_ptr->serial_read_buf = '0';
    clear_float_buf(obj_ptr->xcorr_buffer, STORE_BUF_SIZE);
}

void handle_error() {
    // serial is taken by matlab, use LED as indication
    digitalWrite(error_led, LOW);
}