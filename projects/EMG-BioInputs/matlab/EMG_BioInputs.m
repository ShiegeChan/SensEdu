%% EMG-BioInputs.m
clear;
close all;
clc;

%% Include
addpath(genpath('./processing/'));
addpath(genpath('./keys/'));
addpath(genpath('./plotting/'));

%% Debug Settings
LATENCY_METER_ENABLED = false;
LATENCY_METER_ITERATIONS = 1000;

%% EMG Settings

% Plot Processing Steps (slows down the script)
ENABLE_PLOTS = true;
PLOT_FREQUENCY_SEC = 5;

% Sampling Rates
Fs = 5000;

% EMG chunk size in 16-bit samples
CHUNK_SIZE = 75;

% EMG rolling buffer size for processing
% Contains ~1 second worth of data chunks
EMG_BUFFER_SIZE = CHUNK_SIZE * round(Fs/CHUNK_SIZE);

%% Filter Settings

% Bandpass frequency #1
F0 = 30;

% Bandpass frequency #2
F1 = 450;

% FIR taps (must be even)
TAPS = 150;
FIR_DELAY = TAPS/2;
FIR_COEFFS = fir1(TAPS, [F0 F1]/(Fs/2), 'bandpass');

%% Connection Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 2000000;

% ADC+DMA Settings
CH_NUM = 4;
BUF_SIZE = CHUNK_SIZE * 2 * CH_NUM;

% USB Settings
USB_BUF_MAX_MS = 500;
USB_BUF_MAX_BYTES = USB_BUF_MAX_MS * CH_NUM / 1e3 * Fs * 2;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% Init
half_buf_size = BUF_SIZE / 2;
emg_chunks = zeros(1, half_buf_size);
emg_buffers = zeros(EMG_BUFFER_SIZE, CH_NUM);
filt_emg_buffers = zeros(EMG_BUFFER_SIZE - FIR_DELAY, CH_NUM);

if LATENCY_METER_ENABLED
    latency_meter = zeros(1, LATENCY_METER_ITERATIONS);
    latency_idx = 1;
end

if LATENCY_METER_ENABLED || ENABLE_PLOTS
    tic;
end

[f1] = init_figures();
pause(3);

flush(arduino);

%% Loop
while (true)
    if (arduino.NumBytesAvailable > USB_BUF_MAX_BYTES)
        disp("Too much input buffered data. USB buffer has been flushed.");
        flush(arduino);
    end
    
    % 1. Record EMG chunk
    [is_recorded, emg_chunks] = read_data(arduino, half_buf_size);
    if ~is_recorded
        continue;
    end
    
    % 2. Rearrange chunk by channel
    emg_chunks_per_channel = split_by_channel(emg_chunks, CH_NUM);
    chunk_size = size(emg_chunks_per_channel, 1);
    
    % 3. Add chunk to the rolling buffer
    emg_buffers(1:end-chunk_size, :) = emg_buffers(chunk_size+1:end, :);
    emg_buffers(end-chunk_size+1:end, :) = emg_chunks_per_channel;
    
    % 4. Filter the buffer around EMG frequencies
    filtered_data = filter(FIR_COEFFS, 1, emg_buffers);
    filt_emg_buffers = filtered_data((FIR_DELAY + 1):end, :);

    % 5. DC removal

    % 6. Rectification

    % 7. Envelope

    % 8. Decision Block
    
    % 9. Key Trigger

    % 10. Plot
    if ENABLE_PLOTS
        elapsed_time = toc;
        if elapsed_time > PLOT_FREQUENCY_SEC
            tic;
            figure(f1);
            plot_dataset(emg_buffers, CH_NUM, false);
            plot_dataset(filt_emg_buffers, CH_NUM, true);
        end
    end
    
    % 11. Latency measurements
    if LATENCY_METER_ENABLED && latency_idx <= LATENCY_METER_ITERATIONS
        if ENABLE_PLOTS
            fprintf("You cannot use latency meter and plots at the same time.\n" + ...
                "Disable one of them.\n");
            continue;
        end
        latency_meter(latency_idx) = toc;
        latency_idx = latency_idx + 1;
        if latency_idx > LATENCY_METER_ITERATIONS
            fprintf("avg latency: %ims\n", round(mean(diff(latency_meter)) * 1000));
            figure(1);
            plot_dataset(emg_buffers, CH_NUM, false);
            plot_dataset(filt_emg_buffers, CH_NUM, true);
        end
    end
end

%% Functions
function [is_recorded, data] = read_data(arduino, buf_size)
    total_byte_length = buf_size * 2;
    is_recorded = true;
    if arduino.NumBytesAvailable < total_byte_length
        is_recorded = false;
        data = 0;
        N = 0;
        return;
    end

    available = arduino.NumBytesAvailable;
    N = floor(available / total_byte_length);
    serial_rx_data = read(arduino, total_byte_length * N, "uint8");

    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end

function split_data = split_by_channel(data, ch_num)
    data = reshape(data, ch_num, []);
    split_data = data.';
end

function plot_dataset(data, ch_num, enable_hold)
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        if enable_hold
            hold on;
        end
        plot(data(:, ch));
        ylim([-65535, 65535]);
        hold off;
    end
    
end

function [f1] = init_figures()
    f1 = figure('WindowState', 'maximized');
end
