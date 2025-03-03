%% DAC_data.m
% reads DAC data from Arduino
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;

ACTIVATE_PLOTS = true;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % initialize serial port

%% Readings Loop
write(arduino, 't', "char"); % trigger Arduino measurement

% Step 1: Retrieve the total data length (number of bytes) sent by Arduino
total_byte_length = read_total_length(arduino);
DATA_LENGTH = total_byte_length / 2; % Calculate number of samples (each sample is 2 bytes)
disp(['Receiving ', num2str(DATA_LENGTH), ' samples (', num2str(total_byte_length), ' bytes)...']);

% Step 2: Read the actual data
data = read_data(arduino, DATA_LENGTH);

% Step 3: Plot the data (if enabled)
if ACTIVATE_PLOTS
    plot_data(data);
end

% Set COM port back free
clear arduino;

%% Functions
function total_byte_length = read_total_length(arduino)
    % Reads the 4-byte total length (in bytes) as a uint32 from the Arduino
    len_bytes = read(arduino, 4, 'uint8'); % Read 4 bytes from the Arduino
    total_byte_length = typecast(uint8(len_bytes), 'uint32'); % Convert bytes to uint32
end

function data = read_data(arduino, data_length)
    % Reads `data_length` samples from the Arduino
    total_byte_length = data_length * 2; % Total bytes to read
    serial_rx_data = zeros(1, total_byte_length, 'uint8'); % Preallocate storage for bytes

    % Read the data from the Arduino
    for i = 1:total_byte_length
        serial_rx_data(i) = read(arduino, 1, 'uint8');
    end

    % Convert the received bytes into uint16 samples
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end

function plot_data(data)
    % Plots the DAC data
    plot(data);
    ylim([0, 4096]); % Assuming 12-bit DAC values
    xlabel("Sample #");
    ylabel("DAC Data (12-bit value)");
    grid on;
end
