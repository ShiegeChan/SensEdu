%% EMG_Raw_Recorder.m
% Offline recorder for the EMG-BioInputs.
%
% Captures RECORD_SECONDS of raw multi-channel EMG saves it to the tests/ folder. 
% The saved recording can then be replayed through the exact live pipeline with 
% EMG_Offline_Processor.m.
%
% Acquisition is identical to EMG_BioInputs.m so the stored data 
% matches what the live script would see.
clear;
close all;
clc;

%% User settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 2000000;   % Cosmetic for USB CDC

% How many seconds of data to record.
RECORD_SECONDS = 30;

% Where to save the recording.
OUTPUT_DIR = './tests/';
OUTPUT_PREFIX = 'emg_recording';

%% Firmware constants

% Per-channel sampling rate (Hz).
Fs = 5000;

% EMG chunk size in 16-bit samples per channel.
CHUNK_SIZE = 75;

% Number of ADC channels.
CH_NUM = 4;

% Final DMA buffer size.
BUF_SIZE = CHUNK_SIZE * 2 * CH_NUM;
half_buf_size = BUF_SIZE / 2;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% Record
samples_per_channel = RECORD_SECONDS * Fs;
emg_data = zeros(samples_per_channel, CH_NUM);
write_idx = 0;
last_print_s = 0;

fprintf('Recording %d s (%d samples/ch) from %s ...\n', ...
    RECORD_SECONDS, samples_per_channel, ARDUINO_PORT);

flush(arduino);
tic;
while write_idx < samples_per_channel
    [is_recorded, emg_chunks] = read_data(arduino, half_buf_size);
    if ~is_recorded
        continue;
    end

    % Split EMG channels.
    chunk = split_by_channel(emg_chunks, CH_NUM);

    % Append data.
    take = min(size(chunk, 1), samples_per_channel - write_idx);
    emg_data(write_idx + 1:write_idx + take, :) = chunk(1:take, :);
    write_idx = write_idx + take;

    % Progress, once per recorded second.
    cur_s = floor(write_idx / Fs);
    if cur_s > last_print_s
        fprintf('  %d / %d s\n', cur_s, RECORD_SECONDS);
        last_print_s = cur_s;
    end
end
elapsed = toc;
fprintf('Done. Captured %d samples/ch in %.1f s.\n', write_idx, elapsed);

% Release the serial port.
clear arduino;

%% Save
if ~exist(OUTPUT_DIR, 'dir')
    mkdir(OUTPUT_DIR);
end

timestamp = char(datetime('now', 'Format', 'yyyyMMdd_HHmmss'));
output_file = fullfile(OUTPUT_DIR, sprintf('%s_%s.mat', OUTPUT_PREFIX, timestamp));

% Save the data together with the metadata the processing needs.
save(output_file, 'emg_data', 'Fs', 'CHUNK_SIZE', 'CH_NUM', 'RECORD_SECONDS');

fprintf('Saved recording to %s\n', output_file);

%% Plot
t = (0:samples_per_channel - 1) / Fs;
n_cols = ceil(CH_NUM / 2);
figure('Name', 'Offline Recording', 'NumberTitle', 'off', 'WindowState', 'maximized');
for ch = 1:CH_NUM
    subplot(2, n_cols, ch);
    plot(t, emg_data(:, ch));
    title(sprintf('Channel %d', ch));
    xlabel('time [s]');
    ylabel('ADC Value');
    ylim([0, 65535]);
end

%% Functions
function [is_recorded, data] = read_data(arduino, buf_size)
    total_byte_length = buf_size * 2;
    is_recorded = true;
    if arduino.NumBytesAvailable < total_byte_length
        is_recorded = false;
        data = 0;
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
