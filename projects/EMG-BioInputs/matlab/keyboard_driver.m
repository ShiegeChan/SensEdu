%% TODO:
% 1. error detection

%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
global CUT_SAMPLES;

ARDUINO_PORT = 'COM8';
ARDUINO_BAUDRATE = 115200;

ITERATION = 250;
CUT_SAMPLES = 24; % first few readings of ADCs are wrong since input has a high capacitance, so first couple of samples are removed
FILTER_TAPS_FILENAME = 'EMG_Filter.mat';
PLOT_ON = false;
INIT_OFFSET = 127; 

%% Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate
load(FILTER_TAPS_FILENAME);
cut_filter_delay = (length(taps) - 1)/2; % official delay calculation
cut_filter_delay = round(cut_filter_delay*2);

%% Configuration Readings
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8_data(read(arduino, 2, 'uint8'));
channel_count = typecast_uint8_data(read(arduino, 1, 'uint8'));

data_length = mem_size/channel_count;

%{
% For test purposes:
mem_size = 4096*4;
channel_count = 4;
data_length = 1024*4;
%}


%% Data Readings Loop
raw_data = zeros(channel_count, data_length, ITERATION);
%load('muscles_set_27-May-2025_18-06-57_sine_noisy_4000samples.mat');
%load('muscles_set_27-May-2025_18-08-40_sine_clean_4000samples.mat');

processed_length = data_length-CUT_SAMPLES-cut_filter_delay;
processed_data = zeros(channel_count, processed_length, ITERATION);
processed_x = 1:data_length;
processed_x = processed_x((CUT_SAMPLES+1):(end-cut_filter_delay));

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
        % add noise
        %raw_data(j,:,it) = raw_data(j,:,it) + 10000*rand(1,size(raw_data, 2));
        processed_data(j,:,it) = data_processing(raw_data(j,:,it), taps, cut_filter_delay);
    end
    
    % Plotting
    if PLOT_ON == true
        figure(1);
        plot_whole_dataset(raw_data(:,:,it), 1:data_length);
        figure(2);
        plot_whole_dataset(processed_data(:,:,it), processed_x);
        figure(3);
        mean_values = mean(raw_data(:,:,it), 2);
        shifted_raw_data = zeros(size(raw_data, 1), size(raw_data, 2));
        for j = 1:channel_count
            shifted_raw_data(j,:) = raw_data(j,:, it) - mean_values(j);
        end
        plot_whole_dataset(shifted_raw_data(:,:), 1:data_length, false);
        plot_whole_dataset(processed_data(:,:,it), processed_x, true);
    end
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

function processed_data = data_processing(data, taps, cut_filter_delay)
    global CUT_SAMPLES;
    processed_data = data((CUT_SAMPLES+1):end);
    processed_data = filter(taps, 1, processed_data);
    
    processed_data = processed_data((cut_filter_delay+1):end);
end

function plot_whole_dataset(y_matrix, x_vector, is_hold_on)
    if nargin < 3
        is_hold_on = false;
    end

    for i = 1:size(y_matrix, 1)
        subplot(2,2,i);
        if is_hold_on == true
            hold on;
        else
            hold off;
        end
        plot_one_electrode(y_matrix(i,:), x_vector);
    end
end

function plot_one_electrode(y_vector, x_vector)
    plot(x_vector, y_vector);
    xlim([-200, x_vector(end) + 200]);
    ylim([-2000, 2e3]);
    xlabel("sample");
    ylabel("ADC value");
end

function save_data(raw_data, processed_data)
    % save measurements
    file_name = sprintf('%s_%s.mat', "muscles_set", datetime("now"));
    file_name = strrep(file_name, ' ', '_');
    file_name = strrep(file_name, ':', '-');

    save(file_name, "raw_data", "processed_data");
end