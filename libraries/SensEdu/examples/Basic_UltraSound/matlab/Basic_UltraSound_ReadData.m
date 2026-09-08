%% Basic_UltraSound_ReadData.m
%
% Triggers an ultrasonic measurement with 't', reads the microphone buffer from
% the Arduino, plots it and saves all iterations into Measurements/.
%
% DATA_LENGTH must match the firmware.

clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM22';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 100;

ACTIVATE_PLOTS = true;
CHUNK_SIZE = 32; % Bytes read at once from serial -> 32 is optimal
DATA_LENGTH = 5142; % Must match the firmware

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Select port and baudrate

%% Readings Loop
data = zeros(1,ITERATIONS);
time_axis = zeros(1,ITERATIONS);

for it = 1:ITERATIONS
    % Data readings
    write(arduino, 't', "char"); % Trigger arduino measurement
    time_axis(it) = toc;
    tic
    data = read_data(arduino, DATA_LENGTH, CHUNK_SIZE);
    toc
    plot_data(data);
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
save(file_name, "data", "time_axis");

% Calculate average time between measurements
buf = time_axis(2) - time_axis(1);
for i = 2:(length(time_axis) - 1)
    buf = abs(mean([buf, (time_axis(i+1) - time_axis(i))]));
end
fprintf("Plots are activated: %s\n", mat2str(ACTIVATE_PLOTS));
fprintf("average time between measurements: %fsec\n", buf);

%% Functions
function data = read_data(arduino, data_length, chunk_size)
    total_byte_length = data_length * 2; % 2 bytes per sample
    serial_rx_data = zeros(1, total_byte_length, 'uint8');
    bytes_read = 0;
    while bytes_read < total_byte_length 
        transfer_size = min(chunk_size, total_byte_length - bytes_read);
        serial_rx_data(bytes_read + 1 : bytes_read + transfer_size) = read(arduino, transfer_size, 'uint8');
        bytes_read = bytes_read + transfer_size;
    end
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end

function plot_data(data)
    plot(data);
    ylim([0, 65535]);
    xlabel("Sample #");
    ylabel("ADC 16bit value");
    grid on;
end
