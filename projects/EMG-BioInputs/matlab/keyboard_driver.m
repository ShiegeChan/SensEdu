%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
% Arduino
ARDUINO_PORT = 'COM8';
ARDUINO_BAUDRATE = 115200;

% Processing
BUFFER_DURATION_MS = 1000; % buffer for proper digital filtering

CUT_RAW_SAMPLES = 8;   % removes couple of first readings
                       % they are error prone due to ADC input having a high capacitance

FILTER_TAPS_FILENAME = 'EMG_Filter.mat';

% Plotting
PLOT_ON = true;

% Saving
SAVE_ON = false;
FOLDERNAME = "measurements";
SETNAME = "MuscleSet";
IS_OVERWRITE = true;

% Thresholds
PRESS_RANGE_PERCENTAGE = 0.2;
RELEASE_RANGE_PERCENTAGE = 0.1;

%% Keyboard
import java.awt.Robot;
import java.awt.event.*;
robot = java.awt.Robot();
robot.delay(2000);

keys = [InputEvent.BUTTON1_DOWN_MASK, InputEvent.BUTTON1_DOWN_MASK, InputEvent.BUTTON1_DOWN_MASK, InputEvent.BUTTON1_DOWN_MASK];
keys_state = zeros(1, numel(keys));

%% Arduino Connection
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% Configuration
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8(read(arduino, 2, 'uint8'));

if (isempty(mem_size) || mem_size == 0xFFFF)
    error_msg = ['Couldn''t read configuration parameters from firmware.\n', ...
                 'Probable cause - internal firmware error.\n', ...
                 'Reset the board and pay attention to error message to see the firmware error code.\n' ...
                 'https://shiegechan.github.io/SensEdu/Library/ADC/#errors\n' ...
                 'Look for corresponding error code in the wiki.'];
    error(error_msg, mem_size);
end

channel_n = typecast_uint8(read(arduino, 2, 'uint8'));
fs = double(typecast_uint8(read(arduino, 2, 'uint8')));

data_length = double(mem_size/channel_n);
meas_duration_ms = 1000*(data_length/fs);

if numel(keys) ~= channel_n
    error_msg = ['Configured keys count does not match channel count.\n',
                 'Correct "keys" and "keys_state" vectors to match arduino configuration.'];
    error(error_msg, numel(keys));
end

%% Initialization
% Buffer Init
meas_n = BUFFER_DURATION_MS/meas_duration_ms; % number of measurements in one buffer
samples_per_meas_after_cut = data_length - CUT_RAW_SAMPLES;
buffer = zeros(channel_n, samples_per_meas_after_cut*meas_n);
is_buffer_filled = false;

% Filter
load(FILTER_TAPS_FILENAME);
filter_delay = (length(taps) - 1)/2; % cut FIR delay
cut_filtered_samples = filter_delay; % cut some additional samples for better signal quality
                                     % be careful with data trimming: 
                                     % delay appears on signal end
                                     % additional cut on signal start
% Filtered X axis
processed_x = 1:size(buffer,2);
processed_x = processed_x((cut_filtered_samples+1):(end-filter_delay));

% Maximum Values
threshold_analyzed_x = round(1/4 * size(processed_x, 2)):round(3/4 * size(processed_x, 2));
current_max = zeros(channel_n, 1);
all_history_max = zeros(channel_n, 1);
last_1sec_max = zeros(channel_n, 1);
last_1min_max = zeros(channel_n, 1);

last_1sec_max_values = zeros(channel_n, 1*(1000/meas_duration_ms));
last_1min_max_values = zeros(channel_n, 1*60*(1000/meas_duration_ms));

% Thresholds
press_thresholds = zeros(channel_n, 1);
release_thresholds = zeros(channel_n, 1);

% Keyboard Emulation
keyboard_analyzed_samples = samples_per_meas_after_cut;

% Saving Sequence
if SAVE_ON == true
    prepare_save_folders(FOLDERNAME, SETNAME, IS_OVERWRITE);
end

% Plotting
plotting_offsets = zeros(1, channel_n); % centering for raw samples
keys_plotting_x = processed_x(end); % x axis for key square wave;
i = 1; % chunk counter
while(true)
    start_x = processed_x(end) - keyboard_analyzed_samples*i + 1;
    end_x = start_x - 1;
    if start_x < processed_x(1)
        break;
    end
    keys_plotting_x = [keys_plotting_x, start_x, end_x];
    i = i + 1;
end
clear i;
keys_plotting_x = wrev(keys_plotting_x);
keys_plotting_x = keys_plotting_x(2:end);
keys_plotting_y = zeros(channel_n, numel(keys_plotting_x));

%% Main Loop
counter = 1;
save_loop = 1;
tic;
while(true)
    % Trigger
    write(arduino, 'm', "char"); % measurement

    % Measurements
    new_meas = read_data(arduino, channel_n, data_length);
    check_firmware_error(new_meas, channel_n);

    % Update Buffer
    new_meas = new_meas(:, (CUT_RAW_SAMPLES+1):end);
    buffer = [buffer(:, samples_per_meas_after_cut+1:end), new_meas];

    % Processing
    filtered_buffer = filter_dataset(buffer, taps);
    filtered_buffer = filtered_buffer(:,(filter_delay+1+cut_filtered_samples):end);
    rectified_buffer = rectify_dataset(filtered_buffer);
    enveloped_buffer = envelope_dataset(rectified_buffer, fs);

    % Buffer State
    if is_buffer_filled == false
        if counter < meas_n
            counter = counter + 1;
            continue;
        else
            is_buffer_filled = true;
            plotting_offsets = mean(buffer,2);
        end
    end

    % Correct MAX values
    % analyze only middle of the envelope for increased precision
    loop_max = max(enveloped_buffer(:,threshold_analyzed_x), [], 2);
    all_history_max = max([all_history_max, loop_max], [], 2);
    last_1min_max_values = [last_1min_max_values(:, 2:end), loop_max];
    last_1min_max = max(last_1min_max_values, [], 2);
    last_1sec_max_values = [last_1sec_max_values(:, 2:end), loop_max];
    last_1sec_max = max(last_1sec_max_values, [], 2);
    current_max = adjust_current_max(all_history_max, last_1min_max, last_1sec_max);
    
    % Thresholds
    press_thresholds = PRESS_RANGE_PERCENTAGE.*current_max;
    release_thresholds = RELEASE_RANGE_PERCENTAGE.*current_max;
    
    % Keyboard Emulation
    keys_actions = analyze_keys_state(enveloped_buffer(:,(end-keyboard_analyzed_samples+1):end), press_thresholds, release_thresholds);
    keys_state = release_keys(robot, keys, keys_state, keys_actions(1,:));
    keys_state = press_keys(robot, keys, keys_state, keys_actions(2,:));
    
    % Save Data
    if SAVE_ON == true
        save_data(FOLDERNAME, SETNAME, save_loop, processed_x, buffer, plotting_offsets, rectified_buffer, enveloped_buffer, ...
            current_max, all_history_max, last_1min_max, last_1sec_max);
        save_loop = save_loop + 1;
    end
    
    % Plotting
    if PLOT_ON == true
        figure(1);
        plot_data(processed_x, buffer, plotting_offsets, rectified_buffer, enveloped_buffer);
        %plot_max(size(buffer, 2), current_max, all_history_max, last_1min_max, last_1sec_max);
        plot_thresholds(size(buffer, 2), current_max, press_thresholds, release_thresholds);
        keys_plotting_y = [keys_plotting_y(:, 3:end), keys_state', keys_state'];
        plot_keys(keys_plotting_x, keys_plotting_y);
    end
    toc;
end

% set COM port back free
arduino = [];

%% functions
function data_by_channel = read_data(arduino, channel_n, data_length)
    chunk_size = 64; % in bytes
    total_length = channel_n*data_length*2; % in bytes

    raw_data_8bit = zeros(total_length/chunk_size, chunk_size);
    raw_data_16bit = zeros(total_length/chunk_size, chunk_size/2);
    
    % readings
    for i = 1:(total_length/chunk_size)
        raw_data_8bit(i, :) = read(arduino, chunk_size, 'uint8');
        raw_data_16bit(i, :) = typecast_uint8(raw_data_8bit(i, :));
    end
    

    % rearrange by channel
    combined_raw_data = reshape(raw_data_16bit', 1, []);
    data_by_channel = reshape(combined_raw_data, channel_n, []);
end

function casted_data = typecast_uint8(data)
    reshaped_data = reshape(data, 2, []);
    casted_data = bitshift(uint16(reshaped_data(2, :)), 8) + uint16(reshaped_data(1, :));
end

function check_firmware_error(data, channel_n)
    % error code is 0x0000FFFF
    % LSB first, so expected 0xFFFF, then 0x0000
    % due to channel data rearrangement if channel_n > 1, second part
    % expected to be in different column

    error_flag = 0;
    if data(1,1) == 0xFFFF
        error_flag = 1;
    end
    
    if error_flag == 1
        if channel_n == 1
            if data(1,2) ~= 0x0000
                error_flag = 0;
            end
        else
            if data(2,1) ~= 0x0000
                error_flag = 0;
            end
        end
    end

    if error_flag == 1
        error(['Internal Firmware Error. Code: %s. Reset the board before proceeding.\n' ...
            'https://shiegechan.github.io/SensEdu/Library/ADC/#errors\nLook for corresponding error code in the wiki.'], string(dec2hex(data(1,3))));
    end
end

function filtered_dataset = filter_dataset(dataset, taps)
    filtered_dataset = zeros(size(dataset));
    for i = 1:size(dataset,1)
        filtered_dataset(i,:) = filter(taps, 1, dataset(i,:));
    end
end

function rectified_dataset = rectify_dataset(dataset)
    rectified_dataset = abs(dataset);
end

function enveloped_dataset = envelope_dataset(dataset, fs)
    envelope_cutoff = 15;  % LP filter in Hz
    [b_env, a_env] = butter(4, envelope_cutoff / (fs / 2), 'low');

    enveloped_dataset = zeros(size(dataset));
    for i = 1:size(dataset,1)
        enveloped_dataset(i,:)  = filtfilt(b_env, a_env, dataset(i,:));
    end
end

function plot_data(processed_x, buffer, buffer_plotting_offsets, rectified_buffer, enveloped_buffer)
    for i = 3
        subplot(1,size(buffer,1),i);
        %plot(buffer(i,:) - buffer_plotting_offsets(i));
        %hold on;
        %plot(processed_x, rectified_buffer(i,:));
        plot(processed_x, enveloped_buffer(i,:), 'r', 'linewidth', 2.5);
        ylim([-600,800]);
        %legend(["Raw Data (centered)", "Filtered and Rectified", "Envelope"]);
        hold off;
    end
end

function prepare_save_folders(foldername, setname, is_overwrite)
    if ~isfolder(foldername)
        mkdir(foldername);
    end

    subfolder_path = sprintf("%s\\%s", foldername, setname);
    if ~isfolder(subfolder_path)
        mkdir(subfolder_path);
    elseif is_overwrite == true
        rmdir(subfolder_path, 's');
        mkdir(subfolder_path);
    else
        error("Measurement Set with this name already exists.\nChange SETNAME or put IS_OVERWRITE to 'true'", subfolder_path);
    end
end

function save_data(foldername, setname, buffer_num, processed_x, buffer, buffer_plotting_offsets, rectified_buffer, enveloped_buffer, ...
    current_max, all_history_max, last_1min_max, last_1sec_max)
    subfolder_path = sprintf("%s\\%s", foldername, setname);
    full_filename = sprintf("%s\\%d_%s.mat", subfolder_path, buffer_num, datetime("now"));
    full_filename = strrep(full_filename, ' ', '_');
    full_filename = strrep(full_filename, ':', '-');
    save(full_filename, "processed_x", "buffer", "buffer_plotting_offsets", "rectified_buffer", "enveloped_buffer", ...
        "current_max", "all_history_max", "last_1min_max", "last_1sec_max");
end

function current_max = adjust_current_max(all_history_max, last_1min_max, last_1sec_max)
    current_max = 0.0.*all_history_max + 0.3.*last_1min_max + 0.7*last_1sec_max;
    for i = 1:size(all_history_max, 1)
        %limit = 0.25*all_history_max(i);
        limit = 50; % hard coded value here is less error prone
        if current_max(i) < limit
            current_max(i) = limit;
        end
    end
end

function plot_max(max_index, current_max, all_history_max, last_1min_max, last_1sec_max)
    for i = 1:size(current_max, 1)
        subplot(1,size(current_max, 1),i);
        hold on;
        plot([1,max_index], [current_max(i), current_max(i)], 'color', '#29505d', 'linewidth', 2.5);
        plot([1,max_index], [all_history_max(i), all_history_max(i)], 'color', '#010f1c', 'linewidth', 2.5);
        plot([1,max_index], [last_1min_max(i), last_1min_max(i)], 'color', '#304529', 'linewidth', 2.5);
        plot([1,max_index], [last_1sec_max(i), last_1sec_max(i)], 'color', '#4a6741', 'linewidth', 2.5);
        hold off;
    end
end

function plot_thresholds(max_index, current_max, press_thresholds, release_thresholds)
    for i = 3
        subplot(1,size(current_max, 1),i);
        hold on;
        plot([1,max_index], [current_max(i), current_max(i)], 'color', '#000000', 'linewidth', 2.5);
        plot([1,max_index], [press_thresholds(i), press_thresholds(i)], 'color', '#6B8E23', 'linewidth', 2.5);
        plot([1,max_index], [release_thresholds(i), release_thresholds(i)], 'color', '#FF8C00', 'linewidth', 2.5);
        hold off;
    end
end

function keys_actions = analyze_keys_state(data, press_thresholds, release_thresholds)
    released_states = any(data < release_thresholds, 2)';
    pressed_states = any(data > press_thresholds, 2)';
    keys_actions = [released_states; pressed_states];
end

function keys_state = release_keys(robot, keys, keys_state, keys_release)
    to_release = find(keys_state == 1 & keys_release == 1);
    for i = to_release
        robot.mouseRelease(keys(i));
        keys_state(i) = 0;
    end
end

function keys_state = press_keys(robot, keys, keys_state, keys_press)
    to_press = find(keys_state == 0 & keys_press == 1);
    for i = to_press
        robot.mousePress(keys(i));
        keys_state(i) = 1;
    end
end

function plot_keys(keys_plotting_x, keys_plotting_y)
    for i = 3 % 1:size(keys_plotting_y, 1)
        subplot(1,size(keys_plotting_y, 1),i);
        hold on;
        plot(keys_plotting_x, 200.*keys_plotting_y(i,:), 'color', '#29505d', 'linewidth', 2.5);
        hold off;
    end
end