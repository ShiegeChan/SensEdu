%% Basic_UltraSound_4CH_ReadData.m
% reads config data and then ADC mics measurements from Arduino
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 10000;

ACTIVATE_PLOTS = true;

DATA_LENGTH = 16*128*2; % make sure to match this number with firmware

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

    [data_mic1, data_mic2] = read_2mic_data(arduino, DATA_LENGTH);
    [data_mic3, data_mic4] = read_2mic_data(arduino, DATA_LENGTH);
    plot_data(data_mic1, data_mic2, data_mic3, data_mic4);
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
function [data_mic1, data_mic2] = read_2mic_data(arduino, data_length)
    total_byte_length = data_length * 2; % 2 bytes per sample
    serial_rx_data = zeros(1, total_byte_length);

    for i = 1:(total_byte_length/32) % 32 byte chunk size
        serial_rx_data((32*i - 31):(32*i)) = read(arduino, 32, 'uint8');
    end
    
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
    data_mic1 = zeros(1, floor(length(data)/2));
    data_mic2 = zeros(1, floor(length(data)/2));
    ind = 1;
    for i = 1:2:length(data)
        data_mic1(ind) = data(i);
        data_mic2(ind) = data(i+1);
        ind = ind+1;
    end
end

function plot_data(data_mic1, data_mic2, data_mic3, data_mic4)
    subplot(2,2,1)
    plot(data_mic1);
    ylim([0, 65535]);
    xlim([0, length(data_mic1)]);
    xlabel("Sample #");
    ylabel("ADC1 CH1 16bit");
    title("Microphone 1 data");
    grid on;

    subplot(2,2,2)
    plot(data_mic2);
    ylim([0, 65535]);
    xlim([0, length(data_mic2)]);
    xlabel("Sample #");
    ylabel("ADC1 CH2 16bit");
    title("Microphone 2 data");
    grid on;

    subplot(2,2,3)
    plot(data_mic3);
    ylim([0, 65535]);
    xlim([0, length(data_mic3)]);
    xlabel("Sample #");
    ylabel("ADC2 CH1 16bit");
    title("Microphone 3 data");
    grid on;

    subplot(2,2,4)
    plot(data_mic4);
    ylim([0, 65535]);
    xlim([0, length(data_mic4)]);
    xlabel("Sample #");
    ylabel("ADC2 CH2 16bit");
    title("Microphone 4 data");
    grid on;
end
