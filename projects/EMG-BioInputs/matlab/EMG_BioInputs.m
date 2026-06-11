%% EMG-BioInputs.m
clear;
close all;
clc;

%% Include
addpath(genpath('./acquisition/'));
addpath(genpath('./processing/'));
addpath(genpath('./decision/'));
addpath(genpath('./plotting/'));

%% Debug Settings
LATENCY_METER_ENABLED = false;
LATENCY_METER_ITERATIONS = 1000;

%% EMG Settings

% Retrospective debug view: once per DEBUG_PLOT_SEC seconds, redraw the last
% DEBUG_PLOT_SEC seconds of the processed envelope on a real time axis, with
% the adaptive thresholds and the gate's key-down decisions overlaid, so the
% decision making can be reviewed after the fact.
DEBUG_PLOT_ENABLED = true;
DEBUG_PLOT_SEC = 10;

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

%% Decision Settings (per-channel adaptive gate -> one game button each)
% Each channel drives ONE button as a plain gate (not a classifier): a
% contraction presses it DOWN, relaxing releases it UP, and the game decides
% tap vs hold. The onset/release thresholds adapt to a rolling floor/ceiling of
% the envelope so they stay valid as the amplitude drifts (fatigue, electrode
% position). See the "Decision Block" section of the EMG-BioInputs docs for the
% full rationale (normalisation, long window, easing, min-gap clamp).
ENABLE_KEYS      = true;  % false = detect only (safe); true = inject real keystrokes
CH_BUTTON        = {'LMB', 'Space', 'R1', 'B'};  % per-channel game button (ch1..ch4)

DEC_FLOOR_PCTL   = 20;    % rest-level (floor) percentile of the envelope
DEC_CEIL_PCTL    = 90;    % typical-press (ceiling) percentile; < 95 so one very
                          % hard press does not set the bar for all the rest
DEC_FRAC_HIGH    = 0.50;  % fire    when normalised level n >= this
DEC_FRAC_LOW     = 0.30;  % release when normalised level n <= this (n in 0..1)
DEC_MIN_GAP      = 100;   % min ceiling-floor (counts): press effort above rest;
                          % also the dead/disconnected-channel guard
DEC_SMOOTH_S     = 0.05;  % causal smoothing of the decision input (s)
DEC_HANGOVER_S   = 0.12;  % bridge envelope dips shorter than this (release debounce)
DEC_CAL_WINDOW_S = 30;    % rolling window the floor/ceiling are measured over
DEC_ADAPT_S      = 8.0;   % time constant for easing floor/ceiling (anti-jitter)
DEC_CAL_UPDATE_S = 1.0;   % how often to re-measure floor/ceiling
DEC_CAL_BOOT_S   = 3.0;   % start adapting after this much data (fast start-up)
DEC_CAL_SKIP_S   = 1.5;   % ignore the buffer warm-up before collecting data

% Convert the time-based settings to chunk counts (chunk rate = Fs/CHUNK_SIZE).
chunks_per_sec  = Fs / CHUNK_SIZE;
DEC_HANGOVER    = max(1, round(DEC_HANGOVER_S   * chunks_per_sec));
DEC_SMOOTH_CH   = max(1, round(DEC_SMOOTH_S     * chunks_per_sec));
DEC_CAL_WINDOW  = max(1, round(DEC_CAL_WINDOW_S * chunks_per_sec));
DEC_CAL_BOOT    = max(1, round(DEC_CAL_BOOT_S   * chunks_per_sec));
DEC_CAL_SKIP_CH = max(1, round(DEC_CAL_SKIP_S   * chunks_per_sec));

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

% --- Decision / calibration state ---
% calibrate_thresholds() is reused only to read the floor/ceiling percentiles
% of the window (its base + span outputs); the live threshold policy (easing,
% min-gap clamp, normalised fractions) is applied in the loop so it can adapt
% continuously over a long session.
cal_p = struct('base_pctl', DEC_FLOOR_PCTL, 'span_pctl', DEC_CEIL_PCTL, ...
               'frac_high', DEC_FRAC_HIGH, 'frac_low', DEC_FRAC_LOW, ...
               'min_span',  0);   % min_span unused here (we clamp the gap instead)

% Per-channel thresholds (inf = not yet calibrated).
DEC_TH_HIGH = inf(1, CH_NUM);
DEC_TH_LOW  = inf(1, CH_NUM);
floor_est   = nan(1, CH_NUM);   % eased rest floor       per channel
gap_est     = nan(1, CH_NUM);   % eased (ceiling-floor)  per channel

% Online buffers / state.
smooth_buf = zeros(DEC_SMOOTH_CH, CH_NUM);   % moving-average ring of env samples
cal_hist   = zeros(DEC_CAL_WINDOW, CH_NUM);  % rolling envelope history
cal_count  = 0;
calibrated = false;
loop_k     = 0;
cal_timer  = tic;
gate       = struct('mode', zeros(1, CH_NUM), 'onset', zeros(1, CH_NUM), ...
                    'off',  zeros(1, CH_NUM), 'gap',   zeros(1, CH_NUM));
keys_down  = false(1, CH_NUM);
keys_state = false(1, CH_NUM);

% --- Retrospective debug history (last DEBUG_PLOT_SEC seconds) ---
% Per-chunk ring buffers, appended every loop iteration, so the debug view can
% redraw the recent envelope / thresholds / decisions on a real time axis.
DBG_WINDOW  = max(1, round(DEBUG_PLOT_SEC * chunks_per_sec));
dbg_env     = zeros(DBG_WINDOW, CH_NUM);   % per-chunk envelope (decision input)
dbg_env_dec = zeros(DBG_WINDOW, CH_NUM);   % smoothed value the gate uses
dbg_keys    = false(DBG_WINDOW, CH_NUM);   % gate key-down decision per chunk
dbg_th_high = inf(DBG_WINDOW, CH_NUM);     % onset   threshold in effect per chunk
dbg_th_low  = inf(DBG_WINDOW, CH_NUM);     % release threshold in effect per chunk
dbg_time    = linspace(-DEBUG_PLOT_SEC, 0, DBG_WINDOW);  % x-axis (s; now = 0)
dbg_timer   = tic;

% Keyboard / mouse emulation (java.awt.Robot). Each channel drives one input,
% which may be a keyboard key OR a mouse button, so the press/release actions
% are stored per channel as function handles. Bind these in the game.
% CH1 = left mouse button, CH2 = Space, CH3 = "R1" key, CH4 = "B" key.
if ENABLE_KEYS
    robot = java.awt.Robot();
    LMB = java.awt.event.InputEvent.BUTTON1_DOWN_MASK;   % left mouse button mask
    key_press = { @() robot.mousePress(LMB), ...                            % ch1 -> left mouse button
                  @() robot.keyPress(java.awt.event.KeyEvent.VK_SPACE), ... % ch2 -> spacebar
                  @() robot.keyPress(java.awt.event.KeyEvent.VK_L), ...     % ch3 -> R1 (attack)
                  @() robot.keyPress(java.awt.event.KeyEvent.VK_SPACE) };   % ch4 -> B  (roll / sprint)
    key_release = { @() robot.mouseRelease(LMB), ...                            % ch1
                    @() robot.keyRelease(java.awt.event.KeyEvent.VK_SPACE), ... % ch2
                    @() robot.keyRelease(java.awt.event.KeyEvent.VK_L), ...     % ch3
                    @() robot.keyRelease(java.awt.event.KeyEvent.VK_SPACE) };   % ch4
else
    robot = [];
    key_press = {};
    key_release = {};
end

if LATENCY_METER_ENABLED
    latency_meter = zeros(1, LATENCY_METER_ITERATIONS);
    latency_idx = 1;
end

if DEBUG_PLOT_ENABLED
    f4 = figure('WindowState', 'maximized', 'NumberTitle', 'off', 'Name', ...
        sprintf('Debug - Last %gs (envelope / thresholds / decisions)', DEBUG_PLOT_SEC));
    pause(3);
end

if LATENCY_METER_ENABLED
    tic;
end

flush(arduino);

%% Loop
while (true)
    if (arduino.NumBytesAvailable > USB_BUF_MAX_BYTES)
        %disp("Too much input buffered data. USB buffer has been flushed.");
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

    % 4-6. Filter -> rectify -> envelope (shared with the offline processor).
    [filt_emg_buffers_env, filt_emg_buffers, filt_emg_buffers_abs] = ...
        process_emg_buffer(emg_buffers, FIR_COEFFS, TAPS, Fs, ENVELOP_LP_FREQ);

    % 7. Decision block: per-channel adaptive gate (one game button each).
    %    The newest envelope sample is the decision input for this iteration.
    env_now = filt_emg_buffers_env(end, :);

    % Short causal smoothing (moving average of the most recent chunks) to
    % suppress sub-movement ripple without adding much key-down latency.
    smooth_buf = [smooth_buf(2:end, :); env_now];
    if DEC_SMOOTH_CH > 1
        env_dec_now = mean(smooth_buf, 1);
    else
        env_dec_now = env_now;
    end

    % Rolling envelope history for the online floor/ceiling estimate. Skip the
    % first ~DEC_CAL_SKIP_S so the buffer warm-up transient is excluded.
    loop_k = loop_k + 1;
    if loop_k > DEC_CAL_SKIP_CH
        cal_hist = [cal_hist(2:end, :); env_now];
        cal_count = min(cal_count + 1, DEC_CAL_WINDOW);
    end

    % Periodically re-measure the floor/ceiling and ease the thresholds toward
    % them. Adapting starts after a short bootstrap (DEC_CAL_BOOT) so play is
    % possible within seconds rather than after a full window.
    if cal_count >= DEC_CAL_BOOT && (~calibrated || toc(cal_timer) > DEC_CAL_UPDATE_S)
        % Percentiles over only the filled part of the ring buffer.
        win = cal_hist(end - cal_count + 1:end, :);
        [~, ~, floor_now, span_now] = calibrate_thresholds(win, cal_p);
        gap_now = max(span_now, DEC_MIN_GAP);

        if ~calibrated
            floor_est = floor_now;          % seed directly (no easing yet)
            gap_est   = gap_now;
            calibrated = true;
            fprintf('Calibrated (adaptive floor/ceiling). Ready.\n');
        else
            a = 1 - exp(-toc(cal_timer) / DEC_ADAPT_S);   % ease toward new estimate
            floor_est = floor_est + a * (floor_now - floor_est);
            gap_est   = gap_est   + a * (gap_now   - gap_est);
        end

        DEC_TH_HIGH = floor_est + DEC_FRAC_HIGH * gap_est;
        DEC_TH_LOW  = floor_est + DEC_FRAC_LOW  * gap_est;
        cal_timer = tic;
    end

    % Step the gate and drive the keys once calibrated.
    if calibrated
        [gate, keys_down] = emg_gate_step(env_dec_now, 0, gate, ...
            DEC_TH_HIGH, DEC_TH_LOW, DEC_HANGOVER);

        % 8. Key trigger: edge-detect press / release and inject input.
        if ENABLE_KEYS
            for i = find(keys_down & ~keys_state)
                key_press{i}();
            end
            for i = find(~keys_down & keys_state)
                key_release{i}();
            end
        end
        keys_state = keys_down;
    end

    % Record this chunk into the retrospective debug history (every iteration,
    % so the DEBUG_PLOT_SEC window stays continuous regardless of throttling).
    if DEBUG_PLOT_ENABLED
        dbg_env     = [dbg_env(2:end, :);     env_now];
        dbg_env_dec = [dbg_env_dec(2:end, :); env_dec_now];
        dbg_keys    = [dbg_keys(2:end, :);    keys_down];
        dbg_th_high = [dbg_th_high(2:end, :); DEC_TH_HIGH];
        dbg_th_low  = [dbg_th_low(2:end, :);  DEC_TH_LOW];
    end

    % 9. Retrospective debug view: once per DEBUG_PLOT_SEC, redraw the last
    % DEBUG_PLOT_SEC seconds of envelope / thresholds / decisions on a time
    % axis so the gate behaviour can be checked against the signal.
    if DEBUG_PLOT_ENABLED && toc(dbg_timer) > DEBUG_PLOT_SEC
        % Acquisition sanity check: a channel pinned at the ADC rail
        % (~0 or ~65535) carries no EMG -> check electrode/bias/wiring.
        railed = find(max(emg_buffers, [], 1) >= 65500 | ...
                      min(emg_buffers, [], 1) <= 35);
        if ~isempty(railed)
            fprintf('WARNING: channel(s) [%s] railed/saturated.\n', ...
                num2str(railed));
        end

        figure(f4);
        plot_debug_window(dbg_time, dbg_env, dbg_env_dec, dbg_th_high, ...
            dbg_th_low, dbg_keys, CH_NUM, CH_BUTTON, DEBUG_PLOT_SEC, ...
            floor_est, gap_est);
        drawnow limitrate;
        dbg_timer = tic;
    end

    % 10. Latency measurements
    if LATENCY_METER_ENABLED && latency_idx <= LATENCY_METER_ITERATIONS
        if DEBUG_PLOT_ENABLED
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
% Acquisition helpers (read_data, split_by_channel) live in ./acquisition/ and
% the debug view (plot_debug_window) in ./plotting/, both shared on the path.
