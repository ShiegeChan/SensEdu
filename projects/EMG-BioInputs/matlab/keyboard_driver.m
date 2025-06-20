%% keyboard_driver.m
clear;
close all;
clc;

%% Include
addpath(genpath('./processing/'));
addpath(genpath('./keys/'));
addpath(genpath('./plotting/'));

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
PLOT_ON = false;

% Saving
SAVE_ON = true;
FOLDERNAME = "measurements";
SETNAME = "MuscleSet";
IS_OVERWRITE = true;

% Maximum Values
SHORT_MAXIMUM_SEC = 1;
LONG_MAXIMUM_SEC = 60;

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

last_1sec_max_values = zeros(channel_n, SHORT_MAXIMUM_SEC*(1000/meas_duration_ms)); % for initial calcs we use ideal duration per measurement
last_1min_max_values = zeros(channel_n, LONG_MAXIMUM_SEC*(1000/meas_duration_ms));  % they are recalibrated after couple of first cycles

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

% Timings
exec_durations = zeros(1,200);
exec_counter = 1;
are_timings_corrected = false;

%% Main Loop
counter = 1;
save_loop = 1;
save_counter = 1;
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
            tic;
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
        if save_counter > (meas_n-1)
            save_data(FOLDERNAME, SETNAME, save_loop, buffer, all_history_max, last_1min_max_values, last_1sec_max_values);
            save_loop = save_loop + 1;
            save_counter = 0;
        end
        save_counter = save_counter + 1;
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

    % Timings
    if are_timings_corrected == false
        exec_durations(exec_counter) = toc;
        exec_counter = exec_counter + 1;
        if exec_counter > size(exec_durations, 2)
            exec_duration = mean(diff(exec_durations(2:end)))*1000; % ignore first one due to "tic" position
            short_vals_size = round(SHORT_MAXIMUM_SEC*(1000/exec_duration));
            last_1sec_max_values = last_1sec_max_values(:, (end-short_vals_size+1):end);
            long_vals_size = round(LONG_MAXIMUM_SEC*(1000/exec_duration));
            last_1min_max_values = last_1min_max_values(:, (end-long_vals_size+1):end);
            fprintf("Expected ideal duration per measurement: %dms\n" + ...
                "Real duration per measurement: %dms\n" + ...
                "Threshold calculations are corrected accordingly.\n", meas_duration_ms, round(exec_duration));
            are_timings_corrected = true;
        end
    end
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

function save_data(foldername, setname, buffer_num, raw_data, all_history_max, last_1min_max_values, last_1sec_max_values)
    subfolder_path = sprintf("%s\\%s", foldername, setname);
    full_filename = sprintf("%s\\%d_%s.mat", subfolder_path, buffer_num, datetime("now"));
    full_filename = strrep(full_filename, ' ', '_');
    full_filename = strrep(full_filename, ':', '-');
    save(full_filename, "raw_data", "all_history_max", "last_1min_max_values", "last_1sec_max_values");
end
