function total_byte_length = read_total_length(arduino)
% Reads the 4-byte total length (in bytes) of incoming data from Arduino
len_bytes = read(arduino, 4, 'uint8'); % Read header
total_byte_length = typecast(uint8(len_bytes), 'uint32'); % Convert to uint32
end