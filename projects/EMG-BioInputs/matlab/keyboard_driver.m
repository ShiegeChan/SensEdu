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
HISTORY_LOOPS = 100;
global CUT_SAMPLES;
CUT_SAMPLES = 24;   % removes couple of first readings
                    % they are error prone due to input having a high capacitance   
FILTER_TAPS_FILENAME = 'EMG_Filter.mat';
DEFAULT_OFFSET = 127;

% Decision Block
PRESS_THRESHOLD = 1000;

% Plotting
PLOT_ON = true;

%% Keyboard
import java.awt.Robot;
import java.awt.event.*;
robot = java.awt.Robot();
robot.delay(2000);

keys = [KeyEvent.VK_SPACE, InputEvent.BUTTON1_DOWN_MASK]
keys_status = [false, false];

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
raw_data = zeros(channel_count, data_length);
raw_history = zeros(channel_count, data_length, HISTORY_LOOPS);

processed_length = data_length-CUT_SAMPLES-cut_filter_delay;
processed_data = zeros(channel_count, processed_length);

processed_history = zeros(channel_count, processed_length, HISTORY_LOOPS);
processed_history_x = zeros(1, processed_length*HISTORY_LOOPS);
for i = 1:HISTORY_LOOPS
    processed_data_x = (1 + data_length*(i-1)):(data_length*i);
    processed_data_x = processed_data_x((CUT_SAMPLES+1):(end-cut_filter_delay));
    processed_history_x((1 + processed_length*(i-1)):(processed_length*i)) = processed_data_x;
end

%% Main Loop
count = 0;
while(true)
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
        %  cut and filter
        processed_data(j,:) = data_cut_and_filter(raw_data(j,:), taps, cut_filter_delay);
        
        % offset + abs
        if (count == HISTORY_LOOPS)
            offset = mean(processed_history(j,:));
        else
            offset = DEFAULT_OFFSET;
        end
        processed_data(j,:) = processed_data(j,:) - offset;
        processed_data(j,:) = abs(processed_data(j,:));

        % envelope
        processed_data(j,:) = envelope(processed_data(j,:), 500, "rms");
        
        % press according keys
        keys_status(j) = press_key(processed_data(j,:), PRESS_THRESHOLD, keys(j), keys_status(j), robot);
    end

    % Saving History
    raw_history(:, :, 1:end-1) = raw_history(:, :, 2:end);
    raw_history(:,:,end) = raw_data(:,:);
    processed_history(:, :, 1:end-1) = processed_history(:, :, 2:end);
    processed_history(:,:,end) = processed_data(:,:);
    
    % Counter to indicate filled history
    if count < HISTORY_LOOPS
        count = count + 1;
    end

    % Plotting
    if PLOT_ON == true
        figure(1);
        plot_dataset(raw_history(:,:,:), 1:(data_length*HISTORY_LOOPS), false);
        plot_dataset(processed_history(:,:,:), processed_history_x, true);
    end
end

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

function processed_dataset = data_cut_and_filter(data, taps, cut_filter_delay)
    global CUT_SAMPLES;
    processed_dataset = data((CUT_SAMPLES+1):end);
    processed_dataset = filter(taps, 1, processed_dataset);
    
    processed_dataset = processed_dataset((cut_filter_delay+1):end);
end

function new_key_status = press_key(data, PRESS_THRESHOLD, key, key_status, robot)
    new_key_status = key_status;
    if (data > PRESS_THRESHOLD)
        if key_status == false
            % press only if isn't already pressed (fix this for better pushing)
            robot.keyPress(key);
            new_key_status = true;
            fprintf("Button Pressed.");
        end
    else
        if key_status == true
            robot.keyRelease(key);
            new_key_status = false;
            fprintf("Button Released.");
        end
    end
end

function plot_dataset(y_dataset, x_vector, is_hold_on)
    if nargin < 3
        is_hold_on = false;
    end

    data_length = size(y_matrix,2);
    y_matrix = zeros(size(y_matrix,1),data_length*HISTORY_LOOPS);
    for i = 1:HISTORY_LOOPS
        y_matrix(:,(1 + data_length*(i-1)):(data_length*i)) = y_dataset(:,:,i);
    end

    plot_data(y_matrix, x_vector, is_hold_on);
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