%% TODO:
% 1. error detection
% 2. send packet improvements
% 3. filtering optimizations
% 4. check if high cap inputs stays as a problem when board was fixed

%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
% Arduino
ARDUINO_PORT = 'COM8';
ARDUINO_BAUDRATE = 115200;

% Loop
IS_INFINITE = false;    % if false: runs ITERATION amount of loops 
ITERATION = 250;

% Processing
global CUT_SAMPLES;
CUT_SAMPLES = 24;   % removes couple of first readings
                    % they are error prone due to input having a high capacitance   
FILTER_TAPS_FILENAME = 'EMG_Filter.mat';
INIT_OFFSET = 127; 

% Plotting
PLOT_ON = false;

% Saving
IS_SAVE = false;

%% Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

load(FILTER_TAPS_FILENAME);
cut_filter_delay = (length(taps) - 1)/2; % standard delay calculation
cut_filter_delay = round(cut_filter_delay*2); % corrected to improve quality

%% Configuration
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8_data(read(arduino, 2, 'uint8'));
channel_count = typecast_uint8_data(read(arduino, 1, 'uint8'));
data_length = mem_size/channel_count;

%% Prepare Data Arrays
raw_dataset = zeros(channel_count, data_length, ITERATION);
raw_data = zeros(channel_count, data_length);

processed_length = data_length-CUT_SAMPLES-cut_filter_delay;
processed_dataset = zeros(channel_count, processed_length, ITERATION);
processed_data = zeros(channel_count, processed_length);

processed_x = 1:data_length;
processed_x = processed_x((CUT_SAMPLES+1):(end-cut_filter_delay));

%% Main Loop
for it = 1:ITERATION
    % Trigger
    write(arduino, 'm', "char"); % measurement
    
    % Measurements
    for j = 1:channel_count
        for i = 1:data_length
            raw_data(j, i) = typecast_uint8_data(read(arduino, 2, 'uint8'));
        end
    end

    % Processing
    for j = 1:channel_count
        processed_data(j,:) = data_processing(raw_data(j,:), taps, cut_filter_delay);
    end
    
    % Plotting
    if PLOT_ON == true
        figure(1);
        plot_data(raw_data(:,:), 1:data_length);
        figure(2);
        plot_data(processed_data(:,:), processed_x);
        figure(3);
        mean_values = mean(raw_data(:,:), 2);
        shifted_raw_data = zeros(size(raw_data, 1), size(raw_data, 2));
        for j = 1:channel_count
            shifted_raw_data(j,:) = raw_data(j,:) - mean_values(j);
        end
        plot_data(shifted_raw_data(:,:), 1:data_length, false);
        plot_data(processed_data(:,:), processed_x, true);
    end

    % Saving
    raw_dataset(:,:,it) = raw_data(:,:);
    processed_dataset(:,:,it) = processed_data(:,:);
end

save_data(raw_dataset, processed_dataset)

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

function processed_dataset = data_processing(data, taps, cut_filter_delay)
    global CUT_SAMPLES;
    processed_dataset = data((CUT_SAMPLES+1):end);
    processed_dataset = filter(taps, 1, processed_dataset);
    
    processed_dataset = processed_dataset((cut_filter_delay+1):end);
end

function plot_data(y_matrix, x_vector, is_hold_on)
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
    ylim([-2e3, 2e3]);
    xlabel("sample");
    ylabel("ADC value");
end

function save_data(raw_dataset, processed_dataset)
    % save measurements
    file_name = sprintf('%s_%s.mat', "muscles_set", datetime("now"));
    file_name = strrep(file_name, ' ', '_');
    file_name = strrep(file_name, ':', '-');

    save(file_name, "raw_dataset", "processed_dataset");
end