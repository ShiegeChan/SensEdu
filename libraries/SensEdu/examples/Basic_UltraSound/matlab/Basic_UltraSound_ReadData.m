%% Basic_UltraSound_ReadData.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 10000;

ACTIVATE_PLOTS = true;

DATA_LENGTH = 16*128; % make sure to match this number with firmware

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate

%% Readings Loop
data = zeros(1,ITERATIONS);
time_axis = zeros(1,ITERATIONS);

tic;
for it = 1:ITERATIONS
    % Data readings
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;

    data = read_data(arduino, DATA_LENGTH);
    plot_data(data);
end

% set COM port back free
arduino = [];

% save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('Measurements/%s_%s.mat', "measurements", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "data", "time_axis");

% calculate average time between measurements
buf = time_axis(2) - time_axis(1);
for i = 2:(length(time_axis) - 1)
    buf = mean([buf, (time_axis(i+1) - time_axis(i))]);
end
fprintf("Plots are activated: %s\n", mat2str(ACTIVATE_PLOTS));
fprintf("average time between measurements: %fsec\n", buf);

%% functions
function data = read_data(arduino, data_length)
    total_byte_length = data_length * 2; % 2 bytes per sample
    serial_rx_data = zeros(1, total_byte_length);

    for i = 1:(total_byte_length/32) % 32 byte chunk size
        serial_rx_data((32*i - 31):(32*i)) = read(arduino, 32, 'uint8');
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
