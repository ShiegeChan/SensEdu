%% processing_tests.m
clear;
close all;
clc;

%% Include
addpath(genpath('./processing/'));
addpath(genpath('./keys/'));
addpath(genpath('./plotting/'));

%% Settings
% Loading
FOLDERNAME = "measurements";
SETNAME = "MuscleSet";
MEASUREMENT_SIZE = 1016; % take cut sample number from keyboard_driver
fs = 25600;
channel_n = 4;

% Processing
FILTER_TAPS_FILENAME = 'EMG_Filter.mat';

PRESS_RANGE_PERCENTAGE = 0.2;
RELEASE_RANGE_PERCENTAGE = 0.1;

%% Initialization
% load buffer
file_index = 1;
file_path = get_file_path(FOLDERNAME, SETNAME, file_index);
if isfolder(file_path)
    return;
end
load(file_path);
file_index = file_index + 1;
buffer = raw_data;

% processing
load(FILTER_TAPS_FILENAME);
filter_delay = (length(taps) - 1)/2; % cut FIR delay
cut_filtered_samples = filter_delay;
processed_x = 1:size(buffer,2);
processed_x = processed_x((cut_filtered_samples+1):(end-filter_delay));

% maximum values
threshold_analyzed_x = round(1/4 * size(processed_x, 2)):round(3/4 * size(processed_x, 2));

% keys
robot = 0;
keys = zeros(1, 4);
keys_state = zeros(1, 4);
keyboard_analyzed_samples = MEASUREMENT_SIZE;

% plotting
plotting_offsets = mean(buffer,2);
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
while(true)
    % Update Buffer
    new_meas = raw_data(:, (end-MEASUREMENT_SIZE+1):end);
    raw_data = raw_data(:, (1:(end-MEASUREMENT_SIZE)));
    if isempty(raw_data)
        file_path = get_file_path(FOLDERNAME, SETNAME, file_index);
        if isfolder(file_path)
            return;
        end
        load(file_path);
        file_index = file_index + 1;
    end
    buffer = [buffer(:, MEASUREMENT_SIZE+1:end), new_meas];
    
    % Processing
    filtered_buffer = filter_dataset(buffer, taps);
    filtered_buffer = filtered_buffer(:,(filter_delay+1+cut_filtered_samples):end);
    rectified_buffer = rectify_dataset(filtered_buffer);
    enveloped_buffer = envelope_dataset(rectified_buffer, fs);

    % Correct MAX values
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
    
    % Plotting
    figure(1);
    plot_data(processed_x, buffer, plotting_offsets, rectified_buffer, enveloped_buffer);
    %plot_max(size(buffer, 2), current_max, all_history_max, last_1min_max, last_1sec_max);
    plot_thresholds(size(buffer, 2), current_max, press_thresholds, release_thresholds);
    keys_plotting_y = [keys_plotting_y(:, 3:end), keys_state', keys_state'];
    plot_keys(keys_plotting_x, keys_plotting_y);
end

%% Functions
function file_path = get_file_path(foldername, setname, index)
    search_pattern = sprintf("%s\\%s\\%d_*", foldername, setname, index);
    file = dir(search_pattern);
    file_path = sprintf("%s\\%s\\%s", foldername, setname, file.name);
end