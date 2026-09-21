%% Basic_UltraSound_Filtered_ReadData.m
%
% Triggers an ultrasonic measurement with 't', reads the raw and the filtered
% microphone buffer from the Arduino, plots both on one figure and saves all
% iterations into Measurements/.
%
% The raw buffer is sent unscaled to halve its transfer size, so this script has
% to apply exactly the same rescaling the firmware uses before filtering.
% Any other scaling would make the comparison between the two traces unfair.
%
% DATA_LENGTH must match the firmware.

clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM6';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 1000;

CHUNK_SIZE = 32; % Bytes read at once from serial -> 32 is optimal
DATA_LENGTH = 2048; % Must match the firmware

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Select port and baudrate

%% Readings Loop
rescaled = zeros(ITERATIONS, DATA_LENGTH);
filtered = zeros(ITERATIONS, DATA_LENGTH);

for it = 1:ITERATIONS
    write(arduino, 't', "char"); % Trigger arduino measurement
    [rescaled(it, :), filtered(it, :)] = read_data(arduino, DATA_LENGTH, CHUNK_SIZE);
    plot_data(rescaled(it, :), filtered(it, :));
end

% Set COM port back free
arduino = [];

% Save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('Measurements/%s_%s.mat', "measurements", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "rescaled", "filtered");

%% Functions
function [rescaled, filtered] = read_data(arduino, data_length, chunk_size)
    raw_bytes = read_buffer(arduino, data_length * 2, chunk_size);
    filtered_bytes = read_buffer(arduino, data_length * 4, chunk_size);

    raw = double(typecast(uint8(raw_bytes), 'uint16'));
    filtered = double(typecast(uint8(filtered_bytes), 'single'));

    rescaled = (2.0 * raw) / 65535.0 - 1.0;
end

function serial_rx_data = read_buffer(arduino, total_byte_length, chunk_size)
    serial_rx_data = zeros(1, total_byte_length, 'uint8');
    bytes_read = 0;
    while bytes_read < total_byte_length 
        transfer_size = min(chunk_size, total_byte_length - bytes_read);
        serial_rx_data(bytes_read + 1 : bytes_read + transfer_size) = read(arduino, transfer_size, 'uint8');
        bytes_read = bytes_read + transfer_size;
    end
end

function plot_data(rescaled, filtered)
    plot(rescaled, 'DisplayName', 'Raw (rescaled)');
    hold on;
    plot(filtered, 'DisplayName', 'Filtered');
    hold off;
    xlabel("Sample #");
    ylabel("Amplitude");
    legend('Location', 'northeast');
    ylim([-0.5, 0.5]);
    grid on;
end
