function details_matrix = read_mcu_xcorr_details(arduino, mic_num, data_length, details_size)
% not flexible, hard coded for 3 detail arrays
% data length = 64*32

    details_matrix = zeros(mic_num*details_size, data_length);
    for i = 1:mic_num
        details_matrix(1 + 3*(i-1), :) = read_debug_array(arduino, data_length, 'uint16', 2);
        details_matrix(2 + 3*(i-1), :) = read_debug_array(arduino, data_length, 'single', 4);
        details_matrix(3 + 3*(i-1), :) = read_debug_array(arduino, data_length, 'single', 4);
    end
end

function data = read_debug_array(arduino, data_length, data_type, byte_per_sample)
    total_byte_length = data_length * byte_per_sample;
    serial_rx_data = zeros(1, total_byte_length);
    for i = 1:(total_byte_length/32) % 32 byte chunk size
        serial_rx_data((32*i - 31):(32*i)) = read(arduino, 32, 'uint8')
    end

    data = double(typecast(uint8(serial_rx_data), data_type)); 
end

% function data = read_debug_array(arduino, data_length, data_type, byte_per_sample)
%     total_byte_length = data_length * byte_per_sample;
%     serial_rx_data = zeros(1, total_byte_length, 'uint8');
% 
%     Ensure the total_byte_length is a multiple of 32 for chunk processing
%     if mod(total_byte_length, 32) ~= 0
%         error('Total byte length must be a multiple of 32');
%     end
% 
%     for i = 1:(total_byte_length / 32) % 32 byte chunk size
%         chunk = read(arduino, 32, 'uint8')
%         Ensure that exactly 32 bytes were read
%         if length(chunk) ~= 32
%             error('Failed to read 32 bytes from serial port');
%         end
% 
%         serial_rx_data((32 * (i - 1) + 1):(32 * i)) = chunk;
%     end
% 
%     data = double(typecast(uint8(serial_rx_data), data_type)); 
% end