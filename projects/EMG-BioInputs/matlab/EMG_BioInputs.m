%% EMG-BioInputs.m
% Real-time EMG → game-input controller.
%
% Streams 4-channel EMG from the Arduino, runs the DSP chain over a rolling buffer, 
% and drives one game button per channel through an adaptive per-channel gate: 
% a contraction presses a key / mouse button DOWN, relaxing releases it UP. 
%
% Main loop:
%   1-3.   read queued EMG chunk(s) → split by channel → push to rolling buffer
%   4-6.   DSP: DC-remove → band-pass FIR → rectify → envelope
%   7.     decision gate with adaptive thresholds, stepped once per chunk
%   8.     inject key / mouse press / release on the gate edges
%   9.     periodic calibration of the rest floor and press height
%   10-12. optional debug plots and latency meter
clear;
close all;
clc;

%% Include
addpath(genpath('./acquisition/'));
addpath(genpath('./processing/'));
addpath(genpath('./decision/'));
addpath(genpath('./plotting/'));

%% Firmware Settings (must match the Arduino sketch)
Fs = 5000;                          % per-channel sampling rate (Hz)
CHUNK_SIZE = 75;                    % samples/channel in one EMG chunk
CH_NUM = 4;
BUF_SIZE = CHUNK_SIZE * 2 * CH_NUM; % full double DMA buffer (uint16)
half_buf_size = BUF_SIZE / 2;       % one half-transfer = CH_NUM interleaved chunks

%% Connection Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 2000000; % cosmetic for USB CDC

% Flush the input buffer once it exceeds this, to stay near real time.
USB_BUF_MAX_MS = 500;
USB_BUF_MAX_BYTES = USB_BUF_MAX_MS * CH_NUM / 1e3 * Fs * 2;

%% EMG Processing Settings
EMG_BUFFER_SIZE = CHUNK_SIZE * round(Fs/CHUNK_SIZE); % rolling buffer (~1 s)

% Band-pass FIR → rectify → envelope low-pass.
F0 = 30;
F1 = 450;
TAPS = 150; % must be even
FIR_DELAY = TAPS/2;
FIR_COEFFS = fir1(TAPS, [F0 F1]/(Fs/2), 'bandpass');
ENVELOP_LP_FREQ = 10; % envelope low-pass cutoff (Hz)

%% Decision Settings (per-channel adaptive gate → one game button each)

% Onset / release thresholds.
% g = max(press_est, DEC_MIN_GAP / DEC_FRAC_HIGH) → effective press height above the floor
% onset   threshold = floor + DEC_FRAC_HIGH * g
% release threshold = floor + DEC_FRAC_LOW  * g
DEC_FLOOR_PCTL   = 20;    % rest-level percentile of the idle-only history
DEC_FRAC_HIGH    = 0.40;  % onset   at floor + this fraction of g
DEC_FRAC_LOW     = 0.20;  % release at floor + this fraction of g
DEC_MIN_GAP      = 100;   % minimum allowed g (noise / disconnected-channel guard)

% Smoothing & debouncing.
DEC_ATTACK_S     = 0.015; % smoothing attack tau (fast key-down)
DEC_RELEASE_S    = 0.080; % smoothing release tau (stable holds)
DEC_HANGOVER_S   = 0.12;  % bridge envelope dips shorter than this

% Cold start.
DEC_CAL_SKIP_S   = 1.5;   % ignored data at the start of the script (buffer warm-up)
DEC_CAL_BOOT_S   = 3.0;   % rest data needed before a channel goes live

% Floor recalibration.
DEC_CAL_WINDOW_S = 30;    % rest-time window the floor is calculated over
DEC_CAL_UPDATE_S = 1.0;   % how often the floor is updated
DEC_ADAPT_S      = 8.0;   % floor easing time constant (anti-jitter)
DEC_ADAPT_FAST_S = 1.5;   % faster easing when the floor clearly jumped
DEC_FAST_DEV     = 0.35;  % "clearly jumped" = moved by > this fraction of g

% Press recalibration.
DEC_PEAK_ALPHA   = 0.25;       % per-press EMA step for press_est (~4 presses to adapt)
DEC_PEAK_CLAMP   = [0.5 2.0];  % clamp one press to this x press_est (reject outliers)
DEC_LEARN_MIN_S  = 0.15;       % don't learn from activations shorter than this

% Recovery from drifting press level.
DEC_RECOVER_AFTER_S = 15;   % no press this long → allow press_est to sag
DEC_RECOVER_WIN_S   = 5;    % sag toward the max effort of the last few seconds
DEC_RECOVER_TAU_S   = 10;   % sag easing time constant
DEC_RECOVER_FRAC    = 0.30; % in this look-back window effort counts only above this fraction of g

% Recovery from very long hold (probably stuck).
DEC_STUCK_S         = 20;   % gate active longer that this → adopt it as new baseline

% Convert the time-based settings to chunk counts.
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
A_ATK = 1 - exp(-(CHUNK_SIZE / Fs) / DEC_ATTACK_S);
A_REL = 1 - exp(-(CHUNK_SIZE / Fs) / DEC_RELEASE_S);

%% Debug Settings

% false = detect only; true = inject real OS input.
ENABLE_KEYS = false;

% Estimate and report loop / button-press latency.
LATENCY_METER_ENABLED = true;
LATENCY_METER_ITERATIONS = 1000;

% Retrospective decision-chain plot.
DEBUG_PLOT_ENABLED = false;
DEBUG_PLOT_S = 10;

% DSP-pipeline snapshot plot.
PROC_PLOT_ENABLED = false;

% Per-channel action labels (status prints + plots).
CH_BUTTON = {'LMB (R1 attack)', 'Space (roll/sprint)', 'CH3 (unbound)', 'CH4 (unbound)'};

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% State Init

% Acquisition buffers.
emg_chunks  = zeros(1, half_buf_size);
emg_buffers = zeros(EMG_BUFFER_SIZE, CH_NUM);

% Per-channel calibration estimates and thresholds.
% A NaN floor yields NaN thresholds, so the gate stays off until calibrated.
floor_est   = nan(1, CH_NUM);
press_est   = nan(1, CH_NUM);
floor_ready = false(1, CH_NUM);
calibrated  = false;
[g_est, dec_th_high, dec_th_low] = dec_thresholds(floor_est, press_est, ...
    DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);

% Decision signal and calibration history.
env_dec      = zeros(1, CH_NUM);               % asymmetric-smoothed decision signal
rest_hist    = nan(DEC_CAL_WINDOW, CH_NUM);    % idle-only envelope history
rest_idx     = zeros(1, CH_NUM);               % per-channel ring write index
interval_max = -inf(1, CH_NUM);                % env max since the last calibration
secmax_hist  = -inf(DEC_RECOVER_WIN, CH_NUM);  % per-tick maxima (press_est recovery)

% Per-press tracking.
press_peak  = -inf(1, CH_NUM);   % running peak of the activation in progress
hold_chunks = zeros(1, CH_NUM);  % chunks the gate has been active
since_press = zeros(1, CH_NUM);  % chunks since the last finalized press

% Gate and key state.
gate = struct('mode', zeros(1, CH_NUM), 'onset', zeros(1, CH_NUM), ...
              'off',  zeros(1, CH_NUM), 'gap',   zeros(1, CH_NUM));
keys_down  = false(1, CH_NUM);
keys_state = false(1, CH_NUM);

loop_k = 0;        % global chunk counter
cal_timer = tic;   % calibration-tick clock

%% Keys Emulation Init
if ENABLE_KEYS
    robot = java.awt.Robot();
    LMB = java.awt.event.InputEvent.BUTTON1_DOWN_MASK; % left mouse button mask
    key_press = { @() robot.mousePress(LMB), ...                            % ch1 → left mouse button (R1)
                  @() robot.keyPress(java.awt.event.KeyEvent.VK_SPACE), ... % ch2 → Space (roll/sprint)
                  [], ...                                                   % ch3 → unbound
                  [] };                                                     % ch4 → unbound
    key_release = { @() robot.mouseRelease(LMB), ...                            % ch1
                    @() robot.keyRelease(java.awt.event.KeyEvent.VK_SPACE), ... % ch2
                    [], ...                                                     % ch3
                    [] };                                                       % ch4
else
    robot = [];
    key_press = {};
    key_release = {};
end

%% Debug Init
if LATENCY_METER_ENABLED
    latency_meter = zeros(1, LATENCY_METER_ITERATIONS);
    latency_idx = 1;

    % Theoretical key-down latency: half-chunk + FIR + envelope + smoothing.
    [eb, ea]   = butter(2, ENVELOP_LP_FREQ / (Fs / 2), 'low');
    [egd, ew]  = grpdelay(eb, ea, 512, Fs);
    lat_chunk  = 0.5 * CHUNK_SIZE / Fs; % onset lands mid-chunk on average
    lat_fir    = FIR_DELAY / Fs;
    lat_env    = mean(egd(ew <= ENVELOP_LP_FREQ)) / Fs;
    lat_smooth = DEC_ATTACK_S;
    lat_est_ms = (lat_chunk + lat_fir + lat_env + lat_smooth) * 1000;
end

if DEBUG_PLOT_ENABLED
    % Retrospective decision-history ring buffers.
    DBG_WINDOW  = max(1, round(DEBUG_PLOT_S * chunks_per_sec));
    dbg_env     = zeros(DBG_WINDOW, CH_NUM);
    dbg_env_dec = zeros(DBG_WINDOW, CH_NUM);
    dbg_keys    = false(DBG_WINDOW, CH_NUM);
    dbg_th_high = nan(DBG_WINDOW, CH_NUM);
    dbg_th_low  = nan(DBG_WINDOW, CH_NUM);
    dbg_time    = linspace(-DEBUG_PLOT_S, 0, DBG_WINDOW);

    f4 = figure('WindowState', 'maximized', 'NumberTitle', 'off', 'Name', ...
        sprintf('Debug - Last %gs (decisions)', DEBUG_PLOT_S));
    pause(3);
    dbg_timer = tic;
end

if PROC_PLOT_ENABLED
    f5 = figure('WindowState', 'maximized', 'NumberTitle', 'off', ...
        'Name', 'Debug - Processing steps over the rolling buffer');
    f6 = figure('WindowState', 'maximized', 'NumberTitle', 'off', ...
        'Name', 'Debug - Processing steps over the rolling buffer (combined)');
    pause(3);
    proc_timer = tic;
end

if LATENCY_METER_ENABLED
    tic;
end

flush(arduino);

%% Loop
while (true)
    % Drop stale buffered data so decisions stay near real time.
    if (arduino.NumBytesAvailable > USB_BUF_MAX_BYTES)
        flush(arduino);
    end

    % 1. Record EMG chunk(s). A slow iteration (e.g., a debug plot) makes
    % the next read return several queued chunks at once; ALL are processed.
    [is_recorded, emg_chunks] = read_data(arduino, half_buf_size);
    if ~is_recorded
        continue;
    end

    % 2. Rearrange chunk(s) by channel.
    emg_chunks_per_channel = split_by_channel(emg_chunks, CH_NUM);
    new_rows = size(emg_chunks_per_channel, 1);
    if new_rows > EMG_BUFFER_SIZE   % extreme stall: keep only the newest data
        emg_chunks_per_channel = emg_chunks_per_channel(end-EMG_BUFFER_SIZE+1:end, :);
        new_rows = EMG_BUFFER_SIZE;
    end

    % 3. Add chunk(s) to the rolling buffer
    emg_buffers(1:end-new_rows, :) = emg_buffers(new_rows+1:end, :);
    emg_buffers(end-new_rows+1:end, :) = emg_chunks_per_channel;

    % 4-6. DSP: DC-remove → band-pass → rectify → envelope.
    % The dc/bp/rect stages feed only the debug plot.
    [filt_emg_buffers_env, filt_bp, filt_rect, filt_dc] = ...
        process_emg_buffer(emg_buffers, FIR_COEFFS, TAPS, Fs, ENVELOP_LP_FREQ);

    % 7. Decision block: step the gate once per chunk for every chunk in this
    % read (oldest first), so catch-up reads drop no decision samples and the
    % calibration history / debug time axis stay continuous.
    env_len = size(filt_emg_buffers_env, 1);
    n_new = new_rows / CHUNK_SIZE;
    n_proc = min(n_new, floor((env_len - 1) / CHUNK_SIZE) + 1);
    for chunk_idx = (n_proc - 1):-1:0
        % Extract chunk's envelope.
        env_now = filt_emg_buffers_env(env_len - chunk_idx * CHUNK_SIZE, :);
        loop_k = loop_k + 1;

        % Asymmetric causal smoothing: fast attack keeps the key-down latency
        % low, slower release steadies holds (pairs with the gate hangover).
        rise = env_now > env_dec;
        env_dec(rise)  = env_dec(rise)  + A_ATK * (env_now(rise)  - env_dec(rise));
        env_dec(~rise) = env_dec(~rise) + A_REL * (env_now(~rise) - env_dec(~rise));

        % Rest history for the floor estimate: only idle, 
        % contractions should never contaminate the floor.
        if loop_k > DEC_CAL_SKIP_CH
            resting = gate.mode == 0 & ~(env_dec > dec_th_low);
            for ch = find(resting)
                rest_idx(ch) = mod(rest_idx(ch), DEC_CAL_WINDOW) + 1;
                rest_hist(rest_idx(ch), ch) = env_dec(ch);
            end
            interval_max = max(interval_max, env_dec);
        end

        % Step the gate (hysteresis + hangover).
        prev_down = keys_down;
        [gate, keys_down, done, onset_k, off_k] = emg_gate_step(env_dec, ...
            loop_k, gate, dec_th_high, dec_th_low, DEC_HANGOVER);

        % Track the peak of the activation in progress.
        press_peak(keys_down & ~prev_down) = -inf;
        press_peak(keys_down) = max(press_peak(keys_down), env_dec(keys_down));

        % Learn press_est from finalized presses: long enough to be a real
        % contraction, clearly above the floor, and not a stuck-gate rescue.
        since_press = since_press + 1;
        for ch = find(done)
            since_press(ch) = 0;
            if (off_k(ch) - onset_k(ch)) < DEC_LEARN_MIN_CH || hold_chunks(ch) > DEC_STUCK_CH
                continue;
            end
            press_height = press_peak(ch) - floor_est(ch);
            if isnan(press_est(ch))
                if press_height >= DEC_MIN_GAP
                    press_est(ch) = press_height;
                    fprintf('Channel %d [%s]: press level learned (%.0f above rest).\n', ...
                        ch, CH_BUTTON{ch}, press_height);
                end
            else
                press_height = min(max(press_height, DEC_PEAK_CLAMP(1) * press_est(ch)), ...
                        DEC_PEAK_CLAMP(2) * press_est(ch));
                press_est(ch) = press_est(ch) + DEC_PEAK_ALPHA * (press_height - press_est(ch));
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

        % Recalculate thresholds based on new press estimate.
        [g_est, dec_th_high, dec_th_low] = dec_thresholds(floor_est, ...
            press_est, DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);

        % 8. Key trigger: edge-detect press / release and inject input.
        %    Empty (unbound) channels are skipped.
        if ENABLE_KEYS
            for i = find(keys_down & ~keys_state)
                if ~isempty(key_press{i})
                    key_press{i}(); 
                end
            end
            for i = find(~keys_down & keys_state)
                if ~isempty(key_release{i})
                    key_release{i}();
                end
            end
        end
        keys_state = keys_down;

        % Record this chunk into the retrospective debug history.
        if DEBUG_PLOT_ENABLED
            dbg_env     = [dbg_env(2:end, :);     env_now];
            dbg_env_dec = [dbg_env_dec(2:end, :); env_dec];
            dbg_keys    = [dbg_keys(2:end, :);    keys_down];
            dbg_th_high = [dbg_th_high(2:end, :); dec_th_high];
            dbg_th_low  = [dbg_th_low(2:end, :);  dec_th_low];
        end
    end

    % 9. Calibration tick (every DEC_CAL_UPDATE_S): re-measure the rest floor,
    % run the press_est recovery, and refresh the thresholds.
    if toc(cal_timer) > DEC_CAL_UPDATE_S
        dt = toc(cal_timer);
        cal_timer = tic;

        % Re-measure each rest floor from its idle history and ease toward it.
        for ch = 1:CH_NUM
            r = rest_hist(~isnan(rest_hist(:, ch)), ch);
            if numel(r) < DEC_CAL_BOOT
                % not enough rest data (yet / after a stuck reset)
                continue;
            end
            floor_now = pctl(r, DEC_FLOOR_PCTL);
            if ~floor_ready(ch)
                floor_est(ch) = floor_now; % seed directly (no easing yet)
                floor_ready(ch) = true;
            else
                tau = DEC_ADAPT_S;
                if abs(floor_now - floor_est(ch)) > DEC_FAST_DEV * g_est(ch)
                    tau = DEC_ADAPT_FAST_S; % step change: re-acquire fast
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

        % press_est recovery: presses stopped but clear sub-onset efforts keep
        % appearing (electrode moved / gain dropped) → sag press_est toward
        % them until presses fire again and normal learning resumes.
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

        [g_est, dec_th_high, dec_th_low] = dec_thresholds(floor_est, ...
            press_est, DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);
    end

    % 10. Retrospective debug view (every DEBUG_PLOT_S): redraw the last
    % DEBUG_PLOT_S s of envelope / thresholds / decisions for tuning.
    if DEBUG_PLOT_ENABLED && toc(dbg_timer) > DEBUG_PLOT_S
        % Pinned channel check.
        railed = find(max(emg_buffers, [], 1) >= 65500 | ...
                      min(emg_buffers, [], 1) <= 35);
        if ~isempty(railed)
            fprintf('WARNING: channel(s) [%s] railed/saturated.\n', num2str(railed));
        end

        figure(f4);
        plot_debug_window(dbg_time, dbg_env, dbg_env_dec, dbg_th_high, ...
            dbg_th_low, dbg_keys, CH_NUM, CH_BUTTON, DEBUG_PLOT_S, ...
            floor_est, g_est);
        drawnow limitrate;
        dbg_timer = tic;
    end

    % 11. Processing-steps snapshot: redraw the DSP pipeline 
    % over the current rolling buffer.
    if PROC_PLOT_ENABLED && toc(proc_timer) > DEBUG_PLOT_S
        figure(f5);
        plot_processing_steps(emg_buffers, filt_dc, filt_bp, filt_rect, ...
            filt_emg_buffers_env, Fs, CH_NUM);
        figure(f6);
        plot_processing_combined(filt_dc, filt_bp, filt_rect, ...
            filt_emg_buffers_env, Fs, CH_NUM);
        drawnow limitrate;
        proc_timer = tic;
    end

    % 12. Latency measurements.
    if LATENCY_METER_ENABLED && latency_idx <= LATENCY_METER_ITERATIONS
        if DEBUG_PLOT_ENABLED || PROC_PLOT_ENABLED
            fprintf("You cannot use latency meter and plots at the same time.\n" + ...
                "Disable them.\n");
            continue;
        end
        latency_meter(latency_idx) = toc;
        latency_idx = latency_idx + 1;
        if latency_idx > LATENCY_METER_ITERATIONS
            loop_ms = mean(diff(latency_meter)) * 1000;
            fprintf('------------\n');
            fprintf(['loop period:                    ~%.1f ms (%.0f Hz) ' ...
                '- how often the algorithm reads and processes\n'], ...
                loop_ms, 1000 / loop_ms);
            fprintf('estimated button press latency: ~%.1f ms (%.0f Hz)\n', ...
                lat_est_ms, 1000 / lat_est_ms);
            fprintf(['(half-chunk %.1f + FIR %.1f + envelope %.1f + smoothing %.1f)\n' ...
                'USB / OS / game render are not taken into account\n'], ...
                lat_chunk * 1000, lat_fir * 1000, lat_env * 1000, lat_smooth * 1000);
            fprintf('------------\n');
        end
    end
end

%% Functions
% All helpers are shared on the path:
% - ./acquisition/  (read_data, split_by_channel)
% - ./processing/   (process_emg_buffer, envelop)
% - ./decision/     (pctl, dec_thresholds, emg_gate_step)
% - ./plotting/     (plot_debug_window, plot_processing_steps, plot_processing_combined)
