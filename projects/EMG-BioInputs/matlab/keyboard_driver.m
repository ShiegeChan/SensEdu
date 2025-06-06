%% TODO:
% 1. error detection
% 2. send packet improvements
% 3. filtering optimizations
% 4. check if high cap inputs stays as a problem when board was fixed
% 5. analyze FFT
% 6. improve reading loop
% 7. implement button long press

%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
% Arduino
ARDUINO_PORT = 'COM8';
ARDUINO_BAUDRATE = 115200;

% Processing
HISTORY_LOOPS = 15;
CUT_RAW_SAMPLES = 8;   % removes couple of first readings
                        % they are error prone due to input having a high capacitance
FILTER_TAPS_FILENAME = 'EMG_Filter.mat';
DEFAULT_OFFSET = 127;

% Decision Block
CALIBRATION_LOOPS = 10; % make sure that HISTORY_LOOPS is equals or bigger than calibration loops
PRESS_THRESHOLD = 80;

% Plotting
PLOT_ON = true;

%% Sanity Checks
if (HISTORY_LOOPS < CALIBRATION_LOOPS)
    error("HISTORY_LOOPS must be bigger or equal to CALIBRATION_LOOPS");
end

%% Keyboard
import java.awt.Robot;
import java.awt.event.*;
robot = java.awt.Robot();
robot.delay(2000);

keys = [InputEvent.BUTTON1_DOWN_MASK, InputEvent.BUTTON1_DOWN_MASK, InputEvent.BUTTON1_DOWN_MASK, KeyEvent.VK_ALT];
keys_status = [false, false, false, false];

%% Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

load(FILTER_TAPS_FILENAME);
filter_delay = (length(taps) - 1)/2; % cut FIR delay
cut_filtered_samples = filter_delay; % cut some additional samples for better signal quality
                                     % be careful with plotting: 
                                     % delay appears on signal end
                                     % additional cut on signal start

%% Configuration
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8_data(read(arduino, 2, 'uint8'));
channel_count = typecast_uint8_data(read(arduino, 1, 'uint8'));
data_length = mem_size/channel_count;

%% Prepare Data Arrays
raw_data = zeros(channel_count, data_length);
raw_history = zeros(channel_count, data_length, HISTORY_LOOPS);

processed_length = data_length-CUT_RAW_SAMPLES-cut_filtered_samples-filter_delay;
processed_data = zeros(channel_count, processed_length);

processed_history = zeros(channel_count, processed_length, HISTORY_LOOPS);
processed_history_x = zeros(1, processed_length*HISTORY_LOOPS);
for i = 1:HISTORY_LOOPS
    processed_data_x = (1 + data_length*(i-1)):(data_length*i);
    processed_data_x = processed_data_x((CUT_RAW_SAMPLES+cut_filtered_samples+1):(end-filter_delay));
    processed_history_x((1 + processed_length*(i-1)):(processed_length*i)) = processed_data_x;
end

processed_history2 = processed_history;
processed_history2_x = processed_history_x;
processed_data2 = processed_data;

%% Main Loop
is_calibrated = false;
is_calibration_started = false;
press_threshold = zeros(1, channel_count);
while(true)
    % Trigger
    write(arduino, 'm', "char"); % measurement
    
    % Measurements
    raw_data = read_data(arduino, channel_count, data_length);

    % Processing
    for j = 1:channel_count
        %  cut and filter
        processed_data(j,:) = data_cut_and_filter(raw_data(j,:), taps, filter_delay, CUT_RAW_SAMPLES, cut_filtered_samples);

        % abs
        processed_data(j,:) = abs(processed_data(j,:));

        % envelope
        processed_data(j,:) = envelope(processed_data(j,:), 200, "rms");

        % offset?
        %processed_data(j,:) = processed_data(j,:) - offset;
    end

    % Decision Block
    if is_calibrated == false
        if is_calibration_started == false
            calibration_counter = 0;
            fprintf("Please enter relaxed state for 10 seconds.\n");
            is_calibration_started = true;
        else
            calibration_counter = calibration_counter + 1;
        end

        if calibration_counter == CALIBRATION_LOOPS
            for j = 1:channel_count
                press_threshold(j) = 2*mean(mean(processed_history(j,:,end-CALIBRATION_LOOPS+1:end)));
            end
            is_calibrated = true;
            fprintf("Calibration finished.\n");
        end
    end

    for j = 1:channel_count
        if is_calibrated == true
            %keys_status(j) = press_key(processed_data(j,:), press_threshold(j), keys(j), keys_status(j), robot);
        end
    end

    % Saving History
    raw_history(:, :, 1:end-1) = raw_history(:, :, 2:end);
    raw_history(:,:,end) = raw_data(:,:);
    processed_history(:, :, 1:end-1) = processed_history(:, :, 2:end);
    processed_history(:,:,end) = processed_data(:,:);
    processed_history2(:, :, 1:end-1) = processed_history2(:, :, 2:end);
    processed_history2(:,:,end) = processed_data2(:,:);

    % Plotting
    if PLOT_ON == true
        figure(1);
        %plot_data(raw_data, 1:data_length, false);
        %plot_data(raw_data - mean(raw_data(3,:)), 1:data_length);
        %processed_data_x = 1:(data_length);
        %processed_data_x = processed_data_x((CUT_RAW_SAMPLES+1+cut_filtered_samples):(end-filter_delay));
        %plot_data(processed_data, processed_data_x, true);

        plot_dataset(raw_history(:,:,:), 1:(data_length*HISTORY_LOOPS), true, false);
        plot_dataset(processed_history(:,:,:), processed_history_x, false, true);
        %plot_dataset(processed_history2(:,:,:), processed_history_x, false, true);
    end
end

% set COM port back free
arduino = [];

%% functions
function data_by_channel = read_data(arduino, channel_count, data_length)
    chunk_size = 64; % in bytes
    total_length = channel_count*data_length*2; % in bytes

    raw_data_8bit = zeros(total_length/chunk_size, chunk_size);
    raw_data_16bit = zeros(total_length/chunk_size, chunk_size/2);
    
    % readings
    for i = 1:(total_length/chunk_size)
        raw_data_8bit(i, :) = read(arduino, chunk_size, 'uint8');
        raw_data_16bit(i, :) = typecast_uint8(raw_data_8bit(i, :));
    end
    

    % rearrange by channel
    combined_raw_data = reshape(raw_data_16bit', 1, []);
    data_by_channel = reshape(combined_raw_data, channel_count, []);
end

function casted_data = typecast_uint8(data)
    reshaped_data = reshape(data, 2, []);
    casted_data = bitshift(uint16(reshaped_data(2, :)), 8) + uint16(reshaped_data(1, :));
end

function casted_data = typecast_uint8_data(data)
    % recalculated data by byte position
    byte_length = length(data);
    shifts = 0:(byte_length - 1);
    shifted_data = data .* 2.^(shifts * 8);
    
    % sum separate bytes to calculate single number
    casted_data = sum(shifted_data);
end

function processed_dataset = data_cut_and_filter(data, taps, filter_delay, cut_raw_samples, cut_filtered_samples)
    % cut ADC errors
    processed_dataset = data((cut_raw_samples+1):end);

    % filter
    processed_dataset = filter(taps, 1, processed_dataset);

    % cut delay + some error prone samples
    processed_dataset = processed_dataset((filter_delay+cut_filtered_samples+1):end);
end

function new_key_status = press_key(data, PRESS_THRESHOLD, key, key_status, robot)
    %figure(5);
    %plot(data);


    new_key_status = key_status;
    if (any(data > PRESS_THRESHOLD))
        if key_status == false
            % press only if isn't already pressed (fix this for better pushing)
            robot.mousePress(key);
            robot.delay(50);
            robot.mouseRelease(key);
            %new_key_status = true;
            fprintf(datestr(datetime('now'), 'HH:MM:SS') + " - Button Pressed.\n");
        end
    else
        if key_status == true
            robot.mouseRelease(key);
            new_key_status = false;
            fprintf(datestr(datetime('now'), 'HH:MM:SS') + " - Button Released.\n");
        end
    end
end

function plot_dataset(y_dataset, x_vector, is_center, is_hold_on)
    if nargin < 4
        is_hold_on = false;
    end

    data_length = size(y_dataset,2);
    y_matrix = zeros(size(y_dataset,1),data_length*size(y_dataset,3));
    for i = 1:size(y_dataset,3)
        y_matrix(:,(1 + data_length*(i-1)):(data_length*i)) = y_dataset(:,:,i);
    end

    plot_data(y_matrix, x_vector, is_center, is_hold_on);
end

function plot_data(y_matrix, x_vector, is_center, is_hold_on)
    if nargin < 4
        is_hold_on = false;
    end

    for i = 1:size(y_matrix, 1)
        subplot(1,size(y_matrix, 1),i);
        if is_hold_on == true
            hold on;
        else
            hold off;
        end
        
        if is_center == true
             y_matrix(i,:) = y_matrix(i,:) - mean(y_matrix(i,:));
        end

        plot_one_electrode(y_matrix(i,:), x_vector);
    end
end

function plot_one_electrode(y_vector, x_vector)
    plot(x_vector, y_vector);
    %xlim([-200, x_vector(end) + 200]);
    %ylim([-2e3, 2e3]);
    %ylim([-500, 500]);
    ylim([-1000, 1000]);
    xlabel("sample");
    ylabel("ADC value");
end