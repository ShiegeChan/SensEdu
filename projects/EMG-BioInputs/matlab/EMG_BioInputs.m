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
PLOT_FREQUENCY_SEC = 1;

% Sampling Rates
Fs = 5000;

% EMG chunk size in 16-bit samples
CHUNK_SIZE = 75;

% EMG rolling buffer size for processing
% Contains ~1 second worth of data chunks
EMG_BUFFER_SIZE = CHUNK_SIZE * round(Fs/CHUNK_SIZE);

% Envelop LP Frequency
ENVELOP_LP_FREQ = 10;

%% Filter Settings

% Bandpass frequency #1
F0 = 30;

% Bandpass frequency #2
F1 = 450;

% FIR taps (must be even)
%TAPS = 150;
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
filt_emg_buffers = zeros(EMG_BUFFER_SIZE - TAPS, CH_NUM);

if LATENCY_METER_ENABLED
    latency_meter = zeros(1, LATENCY_METER_ITERATIONS);
    latency_idx = 1;
end

if ENABLE_PLOTS
    [f1, f2, f3] = init_figures();
    pause(3);
end

if LATENCY_METER_ENABLED || ENABLE_PLOTS    
    tic;
end

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
    %load("2hold3press.mat", "emg_buffers");
    %emg_buffers = extractfield(emg_buffers, "emg_buffers");
    % Remove the large ADC DC offset BEFORE band-passing. With 150 taps at
    % Fs = 5 kHz the FIR transition is ~Fs/TAPS wide, so a 30 Hz lower edge
    % cannot reject DC (sum(FIR_COEFFS) ~ 0.2). Left in, the ~25k offset leaks
    % ~5k counts into the output and buries the EMG, so subtract it first.
    emg_buffers_dc = emg_buffers - mean(emg_buffers, 1);
    filtered_data = filter(FIR_COEFFS, 1, emg_buffers_dc);
    % Drop the first TAPS samples: removes the FIR start-up transient and
    % compensates the TAPS/2 linear-phase group delay in one step.
    filt_emg_buffers = filtered_data((TAPS + 1):end, :);

    % 5. Rectification
    % Band-pass already removes DC, so rectify the filtered buffer directly.
    filt_emg_buffers_abs = abs(filt_emg_buffers);

    % 6. Envelope
    filt_emg_buffers_env = envelop(filt_emg_buffers_abs, Fs, ENVELOP_LP_FREQ);

    % 7. Decision Block
    
    % 8. Key Trigger

    % 9. Plot
    if ENABLE_PLOTS
        elapsed_time = toc;
        % Throttle the (relatively slow) redraw so the acquisition loop can
        % stay ahead of the USB stream. Replotting every iteration lets the
        % input buffer overflow and get flushed, which breaks the rolling
        % buffer so a clean, continuous second is never displayed.
        if elapsed_time > PLOT_FREQUENCY_SEC
            % Acquisition sanity check: a channel pinned at the ADC rail
            % (~0 or ~65535) carries no EMG -> check electrode/bias/wiring.
            railed = find(max(emg_buffers, [], 1) >= 65500 | ...
                          min(emg_buffers, [], 1) <= 35);
            if ~isempty(railed)
                fprintf('WARNING: channel(s) [%s] railed/saturated.\n', ...
                    num2str(railed));
            end

            % Figure 1: complete raw measurements (full ADC range).
            figure(f1);
            plot_dataset_raw(emg_buffers, CH_NUM, false);

            % Figure 2: all processing steps overlaid per channel. The raw
            % trace is shown with its DC offset removed so it sits on the
            % same scale as the band-pass / rectified / envelope traces.
            figure(f2);
            raw_aligned = emg_buffers((FIR_DELAY+1):(end-FIR_DELAY), :);
            plot_dataset(raw_aligned - mean(raw_aligned, 1), CH_NUM, false);
            plot_dataset(filt_emg_buffers, CH_NUM, true);
            plot_dataset(filt_emg_buffers_abs, CH_NUM, true);
            plot_dataset(filt_emg_buffers_env, CH_NUM, true);
            add_step_legend(CH_NUM);

            % Figure 3: decisions.
            figure(f3);
            plot_decision(filt_emg_buffers_env, CH_NUM, false);

            drawnow limitrate;
            tic;
        end
    end
    
    % 10. Latency measurements
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

function enveloped_data = envelop(data, fs, cutoff)
    [b, a] = butter(2, cutoff / (fs / 2), 'low');
    enveloped_data = filter(b, a, data);
    
    [gd, w] = grpdelay(b, a, 512, fs);
    avg_gd = mean(gd(w <= cutoff));
    d = max(0, round(avg_gd));
    
    enveloped_data = enveloped_data(d+1:end, :);
end

function plot_dataset_raw(data, ch_num, enable_hold)
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        if enable_hold
            hold on;
        end
        plot(data(:, ch));
        ylim([0, 65535]);
        hold off;
    end
end

function plot_dataset(data, ch_num, enable_hold)
    % Plots each channel as-is (no DC removal here); callers that need a
    % DC-corrected trace subtract the mean before calling.
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        if enable_hold
            hold on;
        end
        plot(data(:, ch));
        hold off;
    end
end

function plot_decision(data, ch_num, enable_hold)
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        if enable_hold
            hold on;
        end
        plot(data(:, ch));
        ylim([-250, 250]);
        hold off;
    end
end

function [f1, f2, f3] = init_figures()
    f1 = figure('WindowState', 'maximized', 'Name', 'Raw Measurements', ...
                'NumberTitle', 'off');
    f2 = figure('WindowState', 'maximized', 'Name', 'Processing Steps', ...
                'NumberTitle', 'off');
    f3 = figure('WindowState', 'maximized', 'Name', 'Decisions', ...
                'NumberTitle', 'off');
end

function add_step_legend(ch_num)
    labels = {'raw (DC removed)', 'band-pass', 'rectified', 'envelope'};
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        legend(labels, 'Location', 'northeast');
        title(sprintf('Channel %d', ch));
    end
end
