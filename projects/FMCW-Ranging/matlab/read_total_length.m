function total_byte_length = read_total_length(arduino)
% Reads the 4-byte size header that precedes each ADC frame from the Arduino
len_bytes = read(arduino, 4, 'uint8');
total_byte_length = typecast(uint8(len_bytes), 'uint32');
end