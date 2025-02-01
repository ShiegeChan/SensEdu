function dist_vector = read_mcu_xcorr(arduino, mic_num)
    dist_vector = zeros(1, mic_num);
    for i = 1:mic_num
        serial_rx_data = read(arduino, 4, 'uint8'); % 32bit per one distance measurement
        dist_vector(i) = double(typecast(uint8(serial_rx_data), 'uint32'))/1e6; % expected in micrometers
        %dist_vector(i) = double(typecast(uint8(serial_rx_data), 'uint32'));
    end
end