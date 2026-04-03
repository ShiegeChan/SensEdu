%% ADC_3CH_DMA_Circular.m
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
% Contains ~1 second worth of data chunks
ROLLING_BUF_SIZE = CHUNK_SIZE * round(Fs/CHUNK_SIZE);

%% Connection Settings
ARDUINO_PORT = 'COM16';
ARDUINO_BAUDRATE = 2000000;

% ADC+DMA Settings
CH_NUM = 3;
TRANSFER_BUF_SIZE = CHUNK_SIZE * 2 * CH_NUM;

% USB Settings
USB_BUF_MAX_MS = 500;
USB_BUF_MAX_BYTES = USB_BUF_MAX_MS * CH_NUM / 1e3 * Fs * 2;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% Init
half_buf_size = TRANSFER_BUF_SIZE / 2;
chunks = zeros(1, half_buf_size);
buffers = zeros(ROLLING_BUF_SIZE, CH_NUM);

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
    
    % 2. Rearrange chunk by channel
    chunks_per_channel = split_by_channel(chunks, CH_NUM);
    chunk_size = size(chunks_per_channel, 1);
    
    % 3. Add chunk to the rolling buffer
    buffers(1:end-chunk_size, :) = buffers(chunk_size+1:end, :);
    buffers(end-chunk_size+1:end, :) = chunks_per_channel;

    % 4. Plot
    if ENABLE_PLOTS
        elapsed_time = toc;
        if elapsed_time > PLOT_FREQUENCY_SEC
            figure(f1);
            plot_dataset(buffers(:, :), CH_NUM, false);
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

function split_data = split_by_channel(data, ch_num)
    data = reshape(data, ch_num, []);
    split_data = data.';
end

function plot_dataset(data, ch_num, enable_hold)
    for ch = 1:ch_num
        if ch_num > 1
            if (mod(ch_num, 2) == 0)
                subplot(2, ch_num/2, ch);
            else
                subplot(1, ch_num, ch);
            end
        end
        if enable_hold
            hold on;
        end
        plot(data(:, ch));
        ylim([0, 65535]);
        hold off;
    end
end
