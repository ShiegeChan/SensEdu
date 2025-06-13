%% data_analysis2.m
clc;
clear;

%% Settings
filename = 'muscles_set.mat';
channel = 3;
buffer_duration = 1000; % history for filtering and envelope in ms

%% logic
load(filename);
clear arduino ARDUINO_PORT ARDUINO_BAUDRATE;

%% INIT
fs = double(sampling_rate);
meas_duration_ms =  meas_time_us; % one measurement duration in ms
meas_n = buffer_duration/meas_duration_ms;
cut_samples_per_meas = data_length - CUT_RAW_SAMPLES;

load(FILTER_TAPS_FILENAME);
filter_delay = (length(taps) - 1)/2; % cut FIR delay
cut_filtered_samples = filter_delay; % cut some additional samples for better signal quality
                                     % be careful with plotting: 
                                     % delay appears on signal end
                                     % additional cut on signal start

%% LOAD DATA
buffer = zeros(1, cut_samples_per_meas*meas_n);
for i = 1:meas_n
    buffer((cut_samples_per_meas*(i-1)+1):(cut_samples_per_meas*i)) = raw_history(channel, (CUT_RAW_SAMPLES+1):end, i);
end

offset = mean(buffer);

%% UPDATE DATA

for i = 26:50
    % Update buffer
    new_meas = raw_history(channel, (CUT_RAW_SAMPLES+1):end, i);
    buffer = [buffer(cut_samples_per_meas+1:end), new_meas];

    % process buffer
    processed_buffer = filter(taps, 1, buffer);
    processed_buffer = processed_buffer((filter_delay+1+cut_filtered_samples):end);
    processed_data_x = 1:numel(buffer);
    processed_data_x = processed_data_x((cut_filtered_samples+1):(end-filter_delay));

    % abs
    processed_buffer = abs(processed_buffer);

    % envelope
    %processed_buffer = envelope(processed_buffer, 500, "rms");
    envelope_cutoff = 15;  % Low-pass filter cutoff frequency (Hz)
    [b_env, a_env] = butter(4, envelope_cutoff / (fs / 2), 'low');  % Design Butterworth filter
    enveloped_buffer  = filtfilt(b_env, a_env, processed_buffer);     % Apply zero-phase filtering


    figure(1);
    plot(buffer - offset);
    hold on;
    plot(processed_data_x, processed_buffer);
    plot(processed_data_x, enveloped_buffer, 'r', 'linewidth', 2.5);

    ylim([-600,800]);
    hold off; 
end