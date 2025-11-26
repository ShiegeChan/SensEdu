function data = read_data(arduino, data_length)
    total_byte_length = data_length * 2; % 2 bytes per sample
    serial_rx_data = zeros(1, total_byte_length);

    for i = 1:(total_byte_length/32) % 32 byte chunk size
        serial_rx_data((32*i - 31):(32*i)) = read(arduino, 32, 'uint8');
    end
    
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end