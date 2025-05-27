%% TODO:
% 1. error detection

%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM8';
ARDUINO_BAUDRATE = 115200;

ITERATION = 50;
CUT_SAMPLES = 96; % first few readings of ADCs are wrong since input has a high capacitance, so first couple of samples are removed
FILTER_TAPS_FILENAME = 'EMG_Filter.mat';

%% Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate
load(FILTER_TAPS_FILENAME);
cut_filter_delay = (length(taps) - 1)/2;

%% Configuration Readings
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8_data(read(arduino, 2, 'uint8'));
channel_count = typecast_uint8_data(read(arduino, 1, 'uint8'));

data_length = mem_size/channel_count;

%{
% For test purposes:
mem_size = 4096;
channel_count = 4;
data_length = 1024;
%}

%% Data Readings Loop
raw_data = zeros(channel_count, data_length, ITERATION);
processed_data = zeros(channel_count, data_length-CUT_SAMPLES-cut_filter_delay, ITERATION);
for it = 1:ITERATION
    % Trigger
    write(arduino, 'm', "char"); % measurement
    
    % Measurements
    for j = 1:channel_count
        for i = 1:data_length
            raw_data(j, i, it) = typecast_uint8_data(read(arduino, 2, 'uint8'));
        end
    end

    % Processing
    for j = 1:channel_count
        %raw_data(j,:,it) = raw_data(j,:,it) + 10000*rand(1,size(raw_data, 2));
        processed_data(j,:,it) = data_processing(raw_data(j,:,it), taps, CUT_SAMPLES, cut_filter_delay);
    end
    
    % Plotting
    figure(1);
    plot_whole_dataset(raw_data(:,(CUT_SAMPLES+1):end,it), size(raw_data, 2) - CUT_SAMPLES);
    figure(2);
    plot_whole_dataset(processed_data(:,:,it), size(processed_data, 2));
end

save_data(raw_data, processed_data)

% set COM port back free
arduino = [];

%% functions
function casted_data = typecast_uint8_data(data)
    % recalculated data by byte position
    byte_length = length(data);
    shifts = 0:(byte_length - 1);
    shifted_data = data .* 2.^(shifts * 8);
    
    % sum separate bytes to calculate single number
    casted_data = sum(shifted_data);
end

function processed_data = data_processing(data, taps, cut_samples, cut_filter_delay)
    processed_data = data((cut_samples+1):end);
    processed_data = filter(taps, 1, processed_data);
    
    processed_data = processed_data((cut_filter_delay+1):end);
end

function plot_whole_dataset(data, x_max)
    subplot(2,2,1);
    plot_one_electrode(data(1,:), x_max);

    subplot(2,2,2);
    plot_one_electrode(data(2,:), x_max);

    subplot(2,2,3);
    plot_one_electrode(data(3,:), x_max);
    
    subplot(2,2,4);
    plot_one_electrode(data(4,:), x_max);
end

function plot_one_electrode(data, x_max)
    plot(data);
    xlim([-200, x_max + 200]);
    %ylim([0, 65535]);
    xlabel("sample")
    ylabel("ADC value")
end

function save_data(raw_data, processed_data)
    % save measurements
    file_name = sprintf('%s_%s.mat', "muscles_set", datetime("now"));
    file_name = strrep(file_name, ' ', '_');
    file_name = strrep(file_name, ':', '-');

    save(file_name, "raw_data", "processed_data");
end