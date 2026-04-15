%% ADC_1CH_DMA_Circular.m
clear;
close all;
clc;

%% EMG Settings

% Plot Processing Steps (slows down the script)
ENABLE_PLOTS = true;
PLOT_FREQUENCY_SEC = 1;

% Sampling Rates
Fs = 44100;

% Per channel chunk size in 16-bit samples
CHUNK_SIZE = 75;

% Rolling buffer size for processing
% Contains ROLLING_BUF_DUR_MS worth of data chunks
ROLLING_BUF_DUR_MS = 50;
ROLLING_BUF_SIZE = CHUNK_SIZE * round(Fs/CHUNK_SIZE/1000*ROLLING_BUF_DUR_MS);

%% Connection Settings
ARDUINO_PORT = 'COM16';
ARDUINO_BAUDRATE = 2000000;

% ADC+DMA Settings
TRANSFER_BUF_SIZE = CHUNK_SIZE * 2;

% USB Settings
USB_BUF_MAX_MS = 500;
USB_BUF_MAX_BYTES = USB_BUF_MAX_MS / 1e3 * Fs * 2;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% Init
half_buf_size = TRANSFER_BUF_SIZE / 2;
chunks = zeros(1, half_buf_size);
buffers = zeros(ROLLING_BUF_SIZE, 1);

if ENABLE_PLOTS
    f1 = figure('WindowState', 'maximized');
    pause(1);
    tic;
end

flush(arduino);
tic;

%% Loop
while (true)
    if (arduino.NumBytesAvailable > USB_BUF_MAX_BYTES)
        disp("Too much input buffered data. USB buffer has been flushed.");
        flush(arduino);
    end
    
    % 1. Record chunk of data
    [is_recorded, chunks] = read_data(arduino, half_buf_size);
    if ~is_recorded
        continue;
    end
    
    % 2. Add chunk to the rolling buffer
    chunks = chunks';
    chunk_size = size(chunks, 1);
    if chunk_size >= numel(buffers)
        buffers = chunks(end-numel(buffers)+1:end);
    else
        buffers(1:end-chunk_size) = buffers(chunk_size+1:end);
        buffers(end-chunk_size+1:end) = chunks;
    end

    % 3. Plot
    if ENABLE_PLOTS
        elapsed_time = toc;
        if elapsed_time > PLOT_FREQUENCY_SEC
            figure(f1);
            plot_dataset(buffers(:, :), false);
            tic;
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

function plot_dataset(data, enable_hold)
    if enable_hold
        hold on;
    end
    plot(data);
    ylim([0, 65535]);
    hold off;
end
