%% EMG-BioInputs.m
clear;
close all;
clc;

%% Include
addpath(genpath('./processing/'));
addpath(genpath('./decision/'));

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
% Each channel drives ONE key: a contraction presses the key DOWN, relaxing
% releases it UP. The game decides tap vs hold (e.g. Dark Souls: tap B = roll,
% hold B = sprint; repeated R1 = attack combo), so the controller is a plain
% gate, not a classifier.
%
% Thresholds work in a NORMALISED range, not absolute counts, so they stay
% valid through a long session as the raw amplitude drifts (electrode position,
% and fatigue which raises amplitude for a given force):
%   floor   = low  percentile of the recent envelope  (the rest level)
%   ceiling = high percentile of the recent envelope  (a typical press)
%   n = (env - floor) / (ceiling - floor)              (~0 at rest, ~1 on press)
% The gate fires at n >= DEC_FRAC_HIGH and releases at n <= DEC_FRAC_LOW
% (computed here as plain counts floor + frac*(ceiling-floor), which is the
% same thing without dividing). Floor and ceiling are tracked continuously
% over a LONG rolling window - so a single hard contraction cannot dominate the
% ceiling and a quiet spell cannot collapse it - and are eased with a time
% constant so they never jump. (ceiling - floor) is clamped to at least
% DEC_MIN_GAP: the minimum effort a press must clear above rest, which both
% stops rest noise from triggering and keeps a dead/disconnected channel silent.
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

    % Re-measure the floor/ceiling periodically and ease the thresholds toward
    % them. The long window keeps the ceiling at a TYPICAL press (not a one-off
    % max), and easing (DEC_ADAPT_S) stops the thresholds jumping. The gap is
    % clamped to DEC_MIN_GAP so rest noise never reaches the press level and
    % dead channels stay silent. Adapting starts after a short bootstrap, using
    % only the samples gathered so far, so play is possible within seconds
    % instead of after a full window.
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

function plot_debug_window(t, env, env_dec, th_high, th_low, keys, ch_num, ch_button, win_s, floor_est, gap_est)
    % Retrospective view of the last win_s seconds: the processed envelope (raw
    % plus the smoothed value the gate actually uses), the adaptive onset /
    % release thresholds that were in effect over time, and the resulting
    % key-down decisions (shaded spans). Lets the gate be checked vs the signal.
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        cla;
        hold on;

        % Thresholds: Inf (before calibration / inactive channel) -> NaN so they
        % leave gaps and do not distort the y-axis autoscale.
        th_h = th_high(:, ch); th_h(~isfinite(th_h)) = NaN;
        th_l = th_low(:, ch);  th_l(~isfinite(th_l)) = NaN;

        % Stable y-range anchored to the (slow-moving) floor/ceiling so the
        % axis does not rescale every redraw -> the same press looks the same
        % size from one window to the next, which makes comparison possible.
        if isfinite(floor_est(ch)) && isfinite(gap_est(ch)) && gap_est(ch) > 0
            yl = [floor_est(ch) - 0.3 * gap_est(ch), ...
                  floor_est(ch) + 2.2 * gap_est(ch)];
        else
            % Not calibrated yet: fall back to the data range.
            vals = [env(:, ch); env_dec(:, ch)];
            lo = min(vals); hi = max(vals);
            if ~(isfinite(lo) && isfinite(hi)) || hi <= lo
                lo = 0; hi = 1;
            end
            pad = 0.05 * (hi - lo);
            yl = [lo - pad, hi + pad];
        end

        % Shade the spans where the key was held down (drawn first = behind).
        kd = keys(:, ch);
        edges  = diff([false; kd(:); false]);
        starts = find(edges == 1);
        stops  = find(edges == -1) - 1;
        for j = 1:numel(starts)
            xs = t(starts(j)); xe = t(stops(j));
            patch([xs xe xe xs], [yl(1) yl(1) yl(2) yl(2)], [0.2 0.7 0.2], ...
                'FaceAlpha', 0.15, 'EdgeColor', 'none', 'HandleVisibility', 'off');
        end

        h_env = plot(t, env(:, ch), 'Color', [0.7 0.7 0.7]);
        h_dec = plot(t, env_dec(:, ch), 'b');
        h_hi  = plot(t, th_h, '--', 'Color', [0.85 0.33 0.10]);
        h_lo  = plot(t, th_l, ':',  'Color', [0.85 0.33 0.10]);
        hold off;

        ylim(yl);
        xlim([t(1) t(end)]);
        title(sprintf('Channel %d [%s]', ch, ch_button{ch}));
        xlabel('Time (s)'); ylabel('Envelope');
        if ch == 1
            legend([h_env h_dec h_hi h_lo], ...
                {'envelope', 'gate input', 'onset', 'release'}, ...
                'Location', 'northwest');
        end
    end
    sgtitle(sprintf('Last %g s   (shaded = key DOWN)', win_s));
end
