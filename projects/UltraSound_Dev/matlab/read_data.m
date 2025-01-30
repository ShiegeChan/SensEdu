function [data_mic1, data_mic2] = read_data(arduino, data_length)
    total_byte_length = data_length * 2; % 2 bytes per sample
    serial_rx_data = zeros(1, total_byte_length);

    for i = 1:(total_byte_length/32) % 32 byte chunk size
        serial_rx_data((32*i - 31):(32*i)) = read(arduino, 32, 'uint8');
    end
    
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
    data_mic_size = data_length/2;
    data_mic1 = zeros(1, data_mic_size);
    data_mic2 = zeros(1, data_mic_size);
    
    
    ind = 1;
    for i = 1:2:data_length
        data_mic1(ind) = data(i);
        data_mic2(ind) = data(i+1);
        ind = ind+1;
    end
end