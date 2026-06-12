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

% Processing-steps snapshot: if enabled, also redraw the DSP pipeline stages
% (raw -> DC removed -> band-pass -> rectified -> envelope) over the current
% rolling buffer, on the same cadence. For documentation / understanding the
% pipeline; leave off during normal play.
PROC_PLOT_ENABLED = false;

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
TAPS = 150;
FIR_DELAY = TAPS/2;
FIR_COEFFS = fir1(TAPS, [F0 F1]/(Fs/2), 'bandpass');

%% Decision Settings (per-channel adaptive gate -> one game button each)
% Each channel drives ONE button as a plain gate (not a classifier): a
% contraction presses it DOWN, relaxing releases it UP, and the game decides
% tap vs hold.
%
% The thresholds derive from two DECOUPLED per-channel estimates, so they do
% not depend on how often the muscle happened to be used recently (no
% duty-cycle dependence, unlike a plain rolling-window percentile):
%
%   floor_est : rest level. A percentile of ONLY the samples where the gate is
%               idle and below the release threshold, eased slowly (fast when
%               it clearly jumps, e.g. an electrode shift), so long holds
%               cannot drag the floor up and quiet stretches cannot shrink
%               the activation range.
%   press_est : typical press height ABOVE the floor. Learned from the peaks
%               of contractions the gate actually finalizes (outlier-clamped
%               per-event EMA), so one very hard or very weak press cannot
%               permanently re-scale the thresholds, and sparse presses keep
%               the bar where the user's real presses are.
%
%   g       = max(press_est, DEC_MIN_GAP / DEC_FRAC_HIGH)  % cold-start/noise
%   TH_HIGH = floor + DEC_FRAC_HIGH * g                    % press (onset)
%   TH_LOW  = floor + DEC_FRAC_LOW  * g                    % release
%
% Safety nets: if presses stop reaching TH_HIGH (electrode moved, gain
% dropped), clear sub-threshold efforts slowly pull press_est down until
% presses register again; a gate stuck active longer than DEC_STUCK_S adopts
% the held level as the new baseline and releases.
ENABLE_KEYS = true;   % false = detect only (safe); true = inject real inputs

% Per-channel game button label (ch1..ch4), shown in the debug plot. Keep in
% sync with the key_press / key_release bindings in the Init section. Unbound
% channels are still detected and plotted (for debugging) but inject no input.
CH_BUTTON = {'LMB (R1 attack)', 'Space (roll/sprint)', 'CH3 (unbound)', 'CH4 (unbound)'};

DEC_FLOOR_PCTL   = 20;    % rest-level percentile of the idle-only history
DEC_FRAC_HIGH    = 0.40;  % onset   at floor + this fraction of g
DEC_FRAC_LOW     = 0.20;  % release at floor + this fraction of g
DEC_MIN_GAP      = 100;   % minimum onset height above the floor (counts):
                          % noise / disconnected-channel guard, and the bar a
                          % first press must clear before press_est is learned
DEC_ATTACK_S     = 0.015; % decision smoothing attack tau (fast key-down)
DEC_RELEASE_S    = 0.080; % decision smoothing release tau (stable holds)
DEC_HANGOVER_S   = 0.12;  % bridge envelope dips shorter than this (debounce)
DEC_CAL_WINDOW_S = 30;    % rest-time window the floor percentile sees
DEC_CAL_UPDATE_S = 1.0;   % how often to re-measure the floor
DEC_CAL_BOOT_S   = 3.0;   % rest data needed before a channel goes live
DEC_CAL_SKIP_S   = 1.5;   % ignore the buffer warm-up before collecting data
DEC_ADAPT_S      = 8.0;   % floor easing time constant (anti-jitter)
DEC_ADAPT_FAST_S = 1.5;   % faster easing when the floor clearly jumped
DEC_FAST_DEV     = 0.35;  % "clearly jumped" = moved by > this fraction of g
DEC_PEAK_ALPHA   = 0.25;  % per-press EMA step for press_est
DEC_PEAK_CLAMP   = [0.5 2.0];  % one press counts as at most this x press_est
DEC_LEARN_MIN_S  = 0.15;  % don't learn from activations shorter than this
DEC_RECOVER_AFTER_S = 15; % no press this long -> allow press_est to sag ...
DEC_RECOVER_WIN_S   = 5;  % ... toward the max effort of the last few seconds
DEC_RECOVER_TAU_S   = 10; % sag easing time constant
DEC_RECOVER_FRAC    = 0.30;  % effort counts only above this fraction of g
DEC_STUCK_S      = 20;    % gate active longer -> adopt level as new baseline

% Convert the time-based settings to chunk counts (chunk rate = Fs/CHUNK_SIZE).
chunks_per_sec    = Fs / CHUNK_SIZE;
DEC_HANGOVER      = max(1, round(DEC_HANGOVER_S      * chunks_per_sec));
DEC_CAL_WINDOW    = max(1, round(DEC_CAL_WINDOW_S    * chunks_per_sec));
DEC_CAL_BOOT      = max(1, round(DEC_CAL_BOOT_S      * chunks_per_sec));
DEC_CAL_SKIP_CH   = max(1, round(DEC_CAL_SKIP_S      * chunks_per_sec));
DEC_LEARN_MIN_CH  = max(1, round(DEC_LEARN_MIN_S     * chunks_per_sec));
DEC_RECOVER_AFTER = max(1, round(DEC_RECOVER_AFTER_S * chunks_per_sec));
DEC_STUCK_CH      = max(1, round(DEC_STUCK_S         * chunks_per_sec));
DEC_RECOVER_WIN   = max(1, round(DEC_RECOVER_WIN_S / DEC_CAL_UPDATE_S));

% Asymmetric decision smoothing coefficients (one step = one chunk).
A_ATT = 1 - exp(-(CHUNK_SIZE / Fs) / DEC_ATTACK_S);
A_REL = 1 - exp(-(CHUNK_SIZE / Fs) / DEC_RELEASE_S);

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

% --- Decision / calibration state ---
floor_est   = nan(1, CH_NUM);   % eased rest floor (NaN = not yet measured)
press_est   = nan(1, CH_NUM);   % press height above floor (NaN = not learned)
floor_ready = false(1, CH_NUM);
calibrated  = false;            % all floors measured (status message only)

% Thresholds (NaN until the floor is measured -> channel cannot fire).
[g_est, DEC_TH_HIGH, DEC_TH_LOW] = dec_thresholds(floor_est, press_est, ...
    DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);

env_dec      = zeros(1, CH_NUM);  % asymmetric-smoothed decision signal
rest_hist    = nan(DEC_CAL_WINDOW, CH_NUM);  % idle-only envelope history
rest_idx     = zeros(1, CH_NUM);             % per-channel ring write index
press_peak   = -inf(1, CH_NUM);  % running peak of the activation in progress
hold_chunks  = zeros(1, CH_NUM); % how long the gate has been active (chunks)
since_press  = zeros(1, CH_NUM); % chunks since the last finalized press
interval_max = -inf(1, CH_NUM);  % env max since the last calibration tick
secmax_hist  = -inf(DEC_RECOVER_WIN, CH_NUM);  % per-tick maxima (recovery)
loop_k       = 0;                % global chunk counter
cal_timer    = tic;
gate         = struct('mode', zeros(1, CH_NUM), 'onset', zeros(1, CH_NUM), ...
                      'off',  zeros(1, CH_NUM), 'gap',   zeros(1, CH_NUM));
keys_down    = false(1, CH_NUM);
keys_state   = false(1, CH_NUM);

% --- Retrospective debug history (last DEBUG_PLOT_SEC seconds) ---
% Per-chunk ring buffers, appended once per processed chunk, so the debug view
% can redraw the recent envelope / thresholds / decisions on a real time axis.
DBG_WINDOW  = max(1, round(DEBUG_PLOT_SEC * chunks_per_sec));
dbg_env     = zeros(DBG_WINDOW, CH_NUM);   % per-chunk envelope (decision input)
dbg_env_dec = zeros(DBG_WINDOW, CH_NUM);   % smoothed value the gate uses
dbg_keys    = false(DBG_WINDOW, CH_NUM);   % gate key-down decision per chunk
dbg_th_high = nan(DBG_WINDOW, CH_NUM);     % onset   threshold in effect per chunk
dbg_th_low  = nan(DBG_WINDOW, CH_NUM);     % release threshold in effect per chunk
dbg_time    = linspace(-DEBUG_PLOT_SEC, 0, DBG_WINDOW);  % x-axis (s; now = 0)
dbg_timer   = tic;

% Keyboard / mouse emulation (java.awt.Robot). Each channel drives one input,
% which may be a keyboard key OR a mouse button, so the press/release actions
% are stored per channel as function handles. An EMPTY entry ([]) means the
% channel is unbound: it is still detected and plotted but injects nothing.
% IMPORTANT: every BOUND channel must map to a DISTINCT key/button: java.awt.Robot
% does no reference counting, so two channels sharing one key would release it
% from under each other. Keep CH_BUTTON (settings above) in sync with these.
% Currently CH1 -> left mouse button (R1 attack), CH2 -> Space (roll/sprint);
% CH3/CH4 are left unbound for now.
if ENABLE_KEYS
    robot = java.awt.Robot();
    LMB = java.awt.event.InputEvent.BUTTON1_DOWN_MASK;   % left mouse button mask
    key_press = { @() robot.mousePress(LMB), ...                           % ch1 -> left mouse button (R1)
                  @() robot.keyPress(java.awt.event.KeyEvent.VK_SPACE), ... % ch2 -> Space (roll/sprint)
                  [], ...                                                  % ch3 -> unbound
                  [] };                                                    % ch4 -> unbound
    key_release = { @() robot.mouseRelease(LMB), ...                           % ch1
                    @() robot.keyRelease(java.awt.event.KeyEvent.VK_SPACE), ... % ch2
                    [], ...                                                    % ch3
                    [] };                                                      % ch4
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

if PROC_PLOT_ENABLED
    f5 = figure('WindowState', 'maximized', 'NumberTitle', 'off', ...
        'Name', 'Debug - Processing steps over the rolling buffer');
    proc_timer = tic;
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

    % 1. Record EMG chunk(s). A slow iteration (e.g. a debug redraw) makes
    % the next read return several queued chunks at once; ALL are processed.
    [is_recorded, emg_chunks] = read_data(arduino, half_buf_size);
    if ~is_recorded
        continue;
    end

    % 2. Rearrange chunk by channel
    emg_chunks_per_channel = split_by_channel(emg_chunks, CH_NUM);
    new_rows = size(emg_chunks_per_channel, 1);
    if new_rows > EMG_BUFFER_SIZE   % extreme stall: keep only the newest data
        emg_chunks_per_channel = emg_chunks_per_channel(end-EMG_BUFFER_SIZE+1:end, :);
        new_rows = EMG_BUFFER_SIZE;
    end

    % 3. Add chunk(s) to the rolling buffer
    emg_buffers(1:end-new_rows, :) = emg_buffers(new_rows+1:end, :);
    emg_buffers(end-new_rows+1:end, :) = emg_chunks_per_channel;

    % 4-6. Filter -> rectify -> envelope (shared with the offline processor).
    % The intermediate stages (dc/bp/rect) are only used by the optional
    % processing-steps plot; they are computed regardless, so capturing them
    % here is free.
    [filt_emg_buffers_env, filt_bp, filt_rect, filt_dc] = ...
        process_emg_buffer(emg_buffers, FIR_COEFFS, TAPS, Fs, ENVELOP_LP_FREQ);

    % 7. Decision block: step the gate once per chunk for EVERY chunk in this
    % read (oldest first), so catch-up reads drop no decision samples and the
    % calibration history / debug time axis stay continuous.
    env_len = size(filt_emg_buffers_env, 1);
    n_new = new_rows / CHUNK_SIZE;
    n_proc = min(n_new, floor((env_len - 1) / CHUNK_SIZE) + 1);
    for ci = (n_proc - 1):-1:0
        % Decision input: the envelope sample at this chunk's end.
        env_now = filt_emg_buffers_env(env_len - ci*CHUNK_SIZE, :);
        loop_k = loop_k + 1;

        % Asymmetric causal smoothing: fast attack keeps the key-down latency
        % low, slower release steadies holds (pairs with the gate hangover).
        rise = env_now > env_dec;
        env_dec(rise)  = env_dec(rise)  + A_ATT * (env_now(rise)  - env_dec(rise));
        env_dec(~rise) = env_dec(~rise) + A_REL * (env_now(~rise) - env_dec(~rise));

        % Rest history for the floor estimate: ONLY idle, sub-release samples,
        % so contractions never contaminate the floor.
        if loop_k > DEC_CAL_SKIP_CH
            resting = gate.mode == 0 & ~(env_dec > DEC_TH_LOW);
            for ch = find(resting)
                rest_idx(ch) = mod(rest_idx(ch), DEC_CAL_WINDOW) + 1;
                rest_hist(rest_idx(ch), ch) = env_dec(ch);
            end
            interval_max = max(interval_max, env_dec);
        end

        % Step the gate (hysteresis + hangover).
        prev_down = keys_down;
        [gate, keys_down, done, onset_k, off_k] = emg_gate_step(env_dec, ...
            loop_k, gate, DEC_TH_HIGH, DEC_TH_LOW, DEC_HANGOVER);

        % Track the peak of the activation in progress.
        press_peak(keys_down & ~prev_down) = -inf;
        press_peak(keys_down) = max(press_peak(keys_down), env_dec(keys_down));

        % Learn press_est from finalized presses: long enough to be a real
        % contraction, clearly above the floor, and not a stuck-gate rescue.
        since_press = since_press + 1;
        for ch = find(done)
            since_press(ch) = 0;
            if (off_k(ch) - onset_k(ch)) < DEC_LEARN_MIN_CH || ...
                    hold_chunks(ch) > DEC_STUCK_CH
                continue;
            end
            h = press_peak(ch) - floor_est(ch);
            if isnan(press_est(ch))
                if h >= DEC_MIN_GAP
                    press_est(ch) = h;
                    fprintf('Channel %d [%s]: press level learned (%.0f above rest).\n', ...
                        ch, CH_BUTTON{ch}, h);
                end
            else
                h = min(max(h, DEC_PEAK_CLAMP(1) * press_est(ch)), ...
                        DEC_PEAK_CLAMP(2) * press_est(ch));
                press_est(ch) = press_est(ch) + DEC_PEAK_ALPHA * (h - press_est(ch));
            end
        end

        hold_chunks(keys_down)  = hold_chunks(keys_down) + 1;
        hold_chunks(~keys_down) = 0;
        % A gate just declared stuck starts a fresh rest history: the old
        % floor samples no longer describe the new baseline.
        for ch = find(hold_chunks == DEC_STUCK_CH + 1)
            rest_hist(:, ch) = NaN;
            rest_idx(ch) = 0;
        end

        [g_est, DEC_TH_HIGH, DEC_TH_LOW] = dec_thresholds(floor_est, ...
            press_est, DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);

        % 8. Key trigger: edge-detect press / release and inject input.
        %    Empty (unbound) channels are skipped.
        if ENABLE_KEYS
            for i = find(keys_down & ~keys_state)
                if ~isempty(key_press{i}); key_press{i}(); end
            end
            for i = find(~keys_down & keys_state)
                if ~isempty(key_release{i}); key_release{i}(); end
            end
        end
        keys_state = keys_down;

        % Record this chunk into the retrospective debug history.
        if DEBUG_PLOT_ENABLED
            dbg_env     = [dbg_env(2:end, :);     env_now];
            dbg_env_dec = [dbg_env_dec(2:end, :); env_dec];
            dbg_keys    = [dbg_keys(2:end, :);    keys_down];
            dbg_th_high = [dbg_th_high(2:end, :); DEC_TH_HIGH];
            dbg_th_low  = [dbg_th_low(2:end, :);  DEC_TH_LOW];
        end
    end

    % 9. Calibration tick (every DEC_CAL_UPDATE_S): re-measure the rest floor,
    % run the press_est recovery, and refresh the thresholds.
    if toc(cal_timer) > DEC_CAL_UPDATE_S
        dt = toc(cal_timer);
        cal_timer = tic;

        for ch = 1:CH_NUM
            r = rest_hist(~isnan(rest_hist(:, ch)), ch);
            if numel(r) < DEC_CAL_BOOT
                continue;   % not enough rest data (yet / after a stuck reset)
            end
            floor_now = pctl(r, DEC_FLOOR_PCTL);
            if ~floor_ready(ch)
                floor_est(ch) = floor_now;     % seed directly (no easing yet)
                floor_ready(ch) = true;
            else
                tau = DEC_ADAPT_S;
                if abs(floor_now - floor_est(ch)) > DEC_FAST_DEV * g_est(ch)
                    tau = DEC_ADAPT_FAST_S;    % step change: re-acquire fast
                end
                a = 1 - exp(-dt / tau);
                floor_est(ch) = floor_est(ch) + a * (floor_now - floor_est(ch));
            end
        end
        if ~calibrated && all(floor_ready)
            calibrated = true;
            fprintf('Rest floors calibrated. Contract each channel once to set its press level.\n');
        end

        % Stuck-gate rescue: pull the floor straight to the held level so the
        % release threshold climbs above it and the key lets go.
        for ch = find(hold_chunks > DEC_STUCK_CH)
            a = 1 - exp(-dt / DEC_ADAPT_FAST_S);
            floor_est(ch) = floor_est(ch) + a * (env_dec(ch) - floor_est(ch));
        end

        % press_est recovery: if presses stopped registering but clear efforts
        % keep appearing below the onset (electrode moved, gain dropped), sag
        % press_est toward those efforts until presses fire again and normal
        % learning takes over.
        secmax_hist = [secmax_hist(2:end, :); interval_max];
        interval_max = -inf(1, CH_NUM);
        for ch = 1:CH_NUM
            if isnan(press_est(ch)) || since_press(ch) < DEC_RECOVER_AFTER
                continue;
            end
            mh = max(secmax_hist(:, ch)) - floor_est(ch);
            if mh > max(DEC_RECOVER_FRAC * g_est(ch), DEC_MIN_GAP) && mh < press_est(ch)
                a = 1 - exp(-dt / DEC_RECOVER_TAU_S);
                press_est(ch) = press_est(ch) + a * (mh - press_est(ch));
            end
        end

        [g_est, DEC_TH_HIGH, DEC_TH_LOW] = dec_thresholds(floor_est, ...
            press_est, DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);
    end

    % 10. Retrospective debug view: once per DEBUG_PLOT_SEC, redraw the last
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
            floor_est, g_est);
        drawnow limitrate;
        dbg_timer = tic;
    end

    % 11. Processing-steps snapshot (optional, for documentation): redraw the
    % DSP pipeline over the current rolling buffer on the same cadence.
    if PROC_PLOT_ENABLED && toc(proc_timer) > DEBUG_PLOT_SEC
        figure(f5);
        plot_processing_steps(emg_buffers, filt_dc, filt_bp, filt_rect, ...
            filt_emg_buffers_env, Fs, CH_NUM);
        drawnow limitrate;
        proc_timer = tic;
    end

    % 12. Latency measurements
    if LATENCY_METER_ENABLED && latency_idx <= LATENCY_METER_ITERATIONS
        if DEBUG_PLOT_ENABLED || PROC_PLOT_ENABLED
            fprintf("You cannot use latency meter and plots at the same time.\n" + ...
                "Disable them.\n");
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
% All helpers are shared on the path: 
% - ./acquisition/  (read_data, split_by_channel)
% - ./decision/     (pctl, dec_thresholds)
% - ./plotting/     (plot_debug_window, plot_processing_steps)
