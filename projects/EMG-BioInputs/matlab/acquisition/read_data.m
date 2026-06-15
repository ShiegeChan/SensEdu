function [is_recorded, data] = read_data(arduino, buf_size)
%READ_DATA  Reads whole EMG half-buffers from the Arduino serial port.
%   Returns is_recorded = false (and data = 0) when less than one full
%   half-buffer is queued, so the caller can simply skip the iteration. 
%   When data is available it reads as many whole half-buffers as are queued.
    total_byte_length = buf_size * 2;
    is_recorded = true;
    if arduino.NumBytesAvailable < total_byte_length
        is_recorded = false;
        data = 0;
        return;
    end

    available = arduino.NumBytesAvailable;
    N = floor(available / total_byte_length);
    serial_rx_data = read(arduino, total_byte_length * N, "uint8");

    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end
