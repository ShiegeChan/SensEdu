%% EMG_Offline_Processor.m
% Offline processing for the EMG_Raw_Recorder.
%
% Replays a recording saved by EMG_Raw_Recorder.m through the exact same
% pipeline as the live EMG_BioInputs.m: it walks the recording one EMG chunk
% at a time, pushes each chunk into a rolling buffer and runs the
% identical DC-removal -> band-pass -> rectify -> envelope -> decision steps.
%
% This lets you validate / tune the processing on repeatable data without the
% hardware attached.

clear;
close all;
clc;

%% Include (shared processing + decision functions)
addpath(genpath('./processing/'));
addpath(genpath('./decision/'));

%% Input File

INPUT_FILE = './tests/emg_recording_20260610_195843.mat';
TESTS_DIR = './tests/';

%% Filter Settings
F0 = 30;
F1 = 450;
TAPS = 150;
FIR_DELAY = TAPS / 2;

ENVELOP_LP_FREQ = 10;

%% Decision Settings (mirrors the live EMG_BioInputs.m adaptive gate)
% Offline replica of the live decision so recordings validate live behaviour.
% Each channel drives ONE button as a plain GATE (not a classifier): a
% contraction turns it ON at the onset, relaxing turns it OFF after the
% hangover. Thresholds adapt from a decoupled rest floor and learned press
% height (see the "Decision Block" section of the docs). DEC_HOLD_S and
% DEC_MIN_PRESS_S are offline-only bookkeeping for the tap/hold summary.
CH_BUTTON = {'LMB (R1 attack)', 'Space (roll/sprint)', 'CH3 (unbound)', 'CH4 (unbound)'};

DEC_FLOOR_PCTL   = 20;    % rest-level percentile of the idle-only history
DEC_FRAC_HIGH    = 0.40;  % onset   at floor + this fraction of g
DEC_FRAC_LOW     = 0.20;  % release at floor + this fraction of g
DEC_MIN_GAP      = 100;   % min onset height above floor (noise/dead-ch guard)
DEC_ATTACK_S     = 0.015; % decision smoothing attack tau (fast key-down)
DEC_RELEASE_S    = 0.080; % decision smoothing release tau (stable holds)
DEC_HANGOVER_S   = 0.12;  % bridge envelope dips shorter than this (debounce)
DEC_CAL_WINDOW_S = 30;    % rest-time window the floor percentile sees
DEC_CAL_UPDATE_S = 1.0;   % how often to re-measure the floor / run recovery
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

DEC_MIN_PRESS_S  = 0.050; % offline: ignore activations shorter than this
DEC_HOLD_S       = 0.40;  % offline label: gate >= this = "hold", else "tap"

%% Load Recording
if isempty(INPUT_FILE)
    files = dir(fullfile(TESTS_DIR, '*.mat'));
    assert(~isempty(files), ...
        'No recordings found in %s. Run EMG_Raw_Recorder.m first.', TESTS_DIR);
    [~, newest] = max([files.datenum]);
    INPUT_FILE = fullfile(TESTS_DIR, files(newest).name);
end
fprintf('Processing %s\n', INPUT_FILE);

rec = load(INPUT_FILE);
emg_data = rec.emg_data;
Fs = double(rec.Fs);
CHUNK_SIZE = double(rec.CHUNK_SIZE);
CH_NUM = double(rec.CH_NUM);

% Rolling buffer
EMG_BUFFER_SIZE = CHUNK_SIZE * round(Fs / CHUNK_SIZE);
FIR_COEFFS = fir1(TAPS, [F0 F1] / (Fs / 2), 'bandpass');

%% Replay chunk-by-chunk
total_samples = size(emg_data, 1);
num_chunks = floor(total_samples / CHUNK_SIZE);

emg_buffers = zeros(EMG_BUFFER_SIZE, CH_NUM);

% Per-chunk envelope (newest sample) - the decision input.
env_per_chunk = zeros(num_chunks, CH_NUM);

% Decision timing converted from seconds to chunks (chunk rate = Fs/CHUNK_SIZE).
chunks_per_sec    = Fs / CHUNK_SIZE;
DEC_HANGOVER      = max(1, round(DEC_HANGOVER_S      * chunks_per_sec));
DEC_CAL_WINDOW    = max(1, round(DEC_CAL_WINDOW_S    * chunks_per_sec));
DEC_CAL_BOOT      = max(1, round(DEC_CAL_BOOT_S      * chunks_per_sec));
DEC_CAL_SKIP_CH   = max(1, round(DEC_CAL_SKIP_S      * chunks_per_sec));
DEC_CAL_UPDATE_CH = max(1, round(DEC_CAL_UPDATE_S    * chunks_per_sec));
DEC_LEARN_MIN_CH  = max(1, round(DEC_LEARN_MIN_S     * chunks_per_sec));
DEC_RECOVER_AFTER = max(1, round(DEC_RECOVER_AFTER_S * chunks_per_sec));
DEC_STUCK_CH      = max(1, round(DEC_STUCK_S         * chunks_per_sec));
DEC_RECOVER_WIN   = max(1, round(DEC_RECOVER_WIN_S / DEC_CAL_UPDATE_S));
DEC_MIN_PRESS     = max(1, round(DEC_MIN_PRESS_S     * chunks_per_sec));  % offline event debounce
DEC_HOLD_CH       = round(DEC_HOLD_S * chunks_per_sec);                   % offline tap/hold label

% Asymmetric decision smoothing coefficients (one step = one chunk).
A_ATT = 1 - exp(-(CHUNK_SIZE / Fs) / DEC_ATTACK_S);
A_REL = 1 - exp(-(CHUNK_SIZE / Fs) / DEC_RELEASE_S);

% Full-recording timelines for every processing step, rebuilt from the newest
% chunk produced at each iteration (so the offline plot can show all stages).
raw_dc_timeline   = zeros(num_chunks * CHUNK_SIZE, CH_NUM);
bandpass_timeline = zeros(num_chunks * CHUNK_SIZE, CH_NUM);
rect_timeline     = zeros(num_chunks * CHUNK_SIZE, CH_NUM);
env_timeline      = zeros(num_chunks * CHUNK_SIZE, CH_NUM);

%% Run the exact live DSP and capture the per-chunk envelope.
for k = 1:num_chunks
    % 1. Next incoming EMG chunk.
    idx = (k - 1) * CHUNK_SIZE + (1:CHUNK_SIZE);
    emg_chunk = emg_data(idx, :);
    chunk_size = size(emg_chunk, 1);

    % 2. Add the chunk to the rolling buffer.
    emg_buffers(1:end - chunk_size, :) = emg_buffers(chunk_size + 1:end, :);
    emg_buffers(end - chunk_size + 1:end, :) = emg_chunk;

    % 3-5. Filter -> rectify -> envelope (shared with the live script).
    [filt_emg_buffers_env, filt_emg_buffers, filt_emg_buffers_abs, emg_buffers_dc] = ...
        process_emg_buffer(emg_buffers, FIR_COEFFS, TAPS, Fs, ENVELOP_LP_FREQ);
    env_per_chunk(k, :) = filt_emg_buffers_env(end, :);

    % 6. Collect the newest chunk of every step to rebuild full timelines.
    %    Raw is delay-compensated by FIR_DELAY so it lines up with the
    %    band-pass output.
    dst = (k - 1) * chunk_size + (1:chunk_size);
    raw_dc_timeline(dst, :) = emg_buffers_dc(end - FIR_DELAY - chunk_size + 1:end - FIR_DELAY, :);
    bandpass_timeline(dst, :) = filt_emg_buffers(end - chunk_size + 1:end, :);
    rect_timeline(dst, :) = filt_emg_buffers_abs(end - chunk_size + 1:end, :);
    env_timeline(dst, :) = filt_emg_buffers_env(end - chunk_size + 1:end, :);
end

%% Decision state (mirrors the live EMG_BioInputs.m online algorithm)
% Per-channel decoupled estimates: a rest floor (percentile of idle-only
% samples) and a press height learned from finalized contractions. Thresholds
% sit a fraction of that height above the floor. See the "Decision Block"
% section of the docs for the full rationale.
floor_est   = nan(1, CH_NUM);   % eased rest floor (NaN = not yet measured)
press_est   = nan(1, CH_NUM);   % press height above floor (NaN = not learned)
floor_ready = false(1, CH_NUM);
[g_est, DEC_TH_HIGH, DEC_TH_LOW] = dec_thresholds(floor_est, press_est, ...
    DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);

env_dec      = zeros(1, CH_NUM);             % asymmetric-smoothed decision signal
rest_hist    = nan(DEC_CAL_WINDOW, CH_NUM);  % idle-only envelope history (ring)
rest_idx     = zeros(1, CH_NUM);
press_peak   = -inf(1, CH_NUM);
hold_chunks  = zeros(1, CH_NUM);
since_press  = zeros(1, CH_NUM);
interval_max = -inf(1, CH_NUM);
secmax_hist  = -inf(DEC_RECOVER_WIN, CH_NUM);
gate         = struct('mode', zeros(1, CH_NUM), 'onset', zeros(1, CH_NUM), ...
                      'off',  zeros(1, CH_NUM), 'gap',   zeros(1, CH_NUM));
keys_down    = false(1, CH_NUM);

% Per-chunk timelines for the plots (the thresholds vary over time now).
env_dec_tl  = zeros(num_chunks, CH_NUM);
th_high_tl  = nan(num_chunks, CH_NUM);
th_low_tl   = nan(num_chunks, CH_NUM);

% Detected events (filled as activations finish; offline tap/hold bookkeeping).
events = struct('ch', {}, 'type', {}, 'onset_s', {}, 'offset_s', {}, ...
    'dur_s', {}, 'peak', {});

%% Decision pass - online adaptive gate over the per-chunk envelope.
% Single causal pass identical to the live loop: asymmetric smoothing -> rest
% floor / press-height estimation -> hysteresis gate -> threshold update, with
% a calibration tick every DEC_CAL_UPDATE_CH chunks. emg_gate_step() is the
% same function the live script uses, so the offline decisions mirror live.
for k = 1:num_chunks
    env_now = env_per_chunk(k, :);
    loop_k = k;

    % Asymmetric causal smoothing (fast attack, slow release).
    rise = env_now > env_dec;
    env_dec(rise)  = env_dec(rise)  + A_ATT * (env_now(rise)  - env_dec(rise));
    env_dec(~rise) = env_dec(~rise) + A_REL * (env_now(~rise) - env_dec(~rise));

    % Rest history for the floor: ONLY idle, sub-release samples.
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

    % Finalize events and learn press_est from real contractions.
    since_press = since_press + 1;
    for ch = find(done)
        since_press(ch) = 0;
        events = finalize_event(events, ch, onset_k(ch), off_k(ch), ...
            env_per_chunk, CHUNK_SIZE / Fs, DEC_MIN_PRESS, DEC_HOLD_CH);
        if (off_k(ch) - onset_k(ch)) < DEC_LEARN_MIN_CH || ...
                hold_chunks(ch) > DEC_STUCK_CH
            continue;
        end
        h = press_peak(ch) - floor_est(ch);
        if isnan(press_est(ch))
            if h >= DEC_MIN_GAP
                press_est(ch) = h;
            end
        else
            h = min(max(h, DEC_PEAK_CLAMP(1) * press_est(ch)), ...
                    DEC_PEAK_CLAMP(2) * press_est(ch));
            press_est(ch) = press_est(ch) + DEC_PEAK_ALPHA * (h - press_est(ch));
        end
    end

    hold_chunks(keys_down)  = hold_chunks(keys_down) + 1;
    hold_chunks(~keys_down) = 0;
    for ch = find(hold_chunks == DEC_STUCK_CH + 1)
        rest_hist(:, ch) = NaN;
        rest_idx(ch) = 0;
    end

    [g_est, DEC_TH_HIGH, DEC_TH_LOW] = dec_thresholds(floor_est, ...
        press_est, DEC_MIN_GAP, DEC_FRAC_HIGH, DEC_FRAC_LOW);

    env_dec_tl(k, :) = env_dec;
    th_high_tl(k, :) = DEC_TH_HIGH;
    th_low_tl(k, :)  = DEC_TH_LOW;

    % Calibration tick (every DEC_CAL_UPDATE_CH chunks).
    if mod(loop_k, DEC_CAL_UPDATE_CH) == 0
        dt = DEC_CAL_UPDATE_S;

        for ch = 1:CH_NUM
            r = rest_hist(~isnan(rest_hist(:, ch)), ch);
            if numel(r) < DEC_CAL_BOOT
                continue;
            end
            floor_now = pctl(r, DEC_FLOOR_PCTL);
            if ~floor_ready(ch)
                floor_est(ch) = floor_now;
                floor_ready(ch) = true;
            else
                tau = DEC_ADAPT_S;
                if abs(floor_now - floor_est(ch)) > DEC_FAST_DEV * g_est(ch)
                    tau = DEC_ADAPT_FAST_S;
                end
                a = 1 - exp(-dt / tau);
                floor_est(ch) = floor_est(ch) + a * (floor_now - floor_est(ch));
            end
        end

        % Stuck-gate rescue: pull the floor up to the held level so it releases.
        for ch = find(hold_chunks > DEC_STUCK_CH)
            a = 1 - exp(-dt / DEC_ADAPT_FAST_S);
            floor_est(ch) = floor_est(ch) + a * (env_dec(ch) - floor_est(ch));
        end

        % press_est recovery toward clear sub-threshold efforts.
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
end

% Close out any activation still open at the end of the recording.
for ch = 1:CH_NUM
    if gate.mode(ch) ~= 0
        if gate.mode(ch) == 2
            last_off = gate.off(ch);
        else
            last_off = num_chunks;
        end
        events = finalize_event(events, ch, gate.onset(ch), last_off, ...
            env_per_chunk, CHUNK_SIZE / Fs, DEC_MIN_PRESS, DEC_HOLD_CH);
    end
end

%% Summary
fprintf('\nPer-channel final calibration (rest floor / learned press height):\n');
for ch = 1:CH_NUM
    if ~floor_ready(ch)
        fprintf('  Channel %d [%s]: floor not calibrated (insufficient rest data)\n', ...
            ch, CH_BUTTON{ch});
    elseif isnan(press_est(ch))
        fprintf(['  Channel %d [%s]: floor=%5.0f  press=  n/a  -> onset=%5.0f ' ...
            'release=%5.0f (no press learned; using min-gap)\n'], ...
            ch, CH_BUTTON{ch}, floor_est(ch), DEC_TH_HIGH(ch), DEC_TH_LOW(ch));
    else
        fprintf(['  Channel %d [%s]: floor=%5.0f  press=%5.0f  -> onset=%5.0f ' ...
            'release=%5.0f\n'], ...
            ch, CH_BUTTON{ch}, floor_est(ch), press_est(ch), DEC_TH_HIGH(ch), DEC_TH_LOW(ch));
    end
end

fprintf('\nButton activity (live port: key DOWN at onset, key UP at release):\n');
fprintf('  tap = quick press (roll / single attack); HOLD = sustained (sprint / held)\n');
for ch = 1:CH_NUM
    if ~floor_ready(ch)
        continue;   % never calibrated -> no button
    end
    sel = find([events.ch] == ch);
    fprintf('  Channel %d [%s]: %d press(es)\n', ch, CH_BUTTON{ch}, numel(sel));
    for i = sel
        if strcmp(events(i).type, 'hold')
            kind = 'HOLD';   % sustained -> sprint / held attack
        else
            kind = 'tap ';   % quick     -> roll / single attack
        end
        fprintf('     %s  down=%6.2f s  held=%5.0f ms  peak=%6.0f\n', ...
            kind, events(i).onset_s, events(i).dur_s * 1000, events(i).peak);
    end
end

%% Plot results over the whole recording
t_raw = (0:total_samples - 1) / Fs;
t_steps = (0:num_chunks * CHUNK_SIZE - 1) / Fs;   % time axis for the rebuilt steps
t_chunk = (1:num_chunks) * CHUNK_SIZE / Fs;       % time of each processed chunk
n_cols = ceil(CH_NUM / 2);

% Figure 1: raw recording.
figure('Name', 'Offline - Raw', 'NumberTitle', 'off', 'WindowState', 'maximized');
for ch = 1:CH_NUM
    subplot(2, n_cols, ch);
    plot(t_raw, emg_data(:, ch));
    title(sprintf('Channel %d - raw', ch));
    xlabel('Time (s)'); ylabel('ADC counts'); ylim([0, 65535]);
end

% Figure 2: all processing steps overlaid per channel (raw -> band-pass ->
% rectified -> envelope).
figure('Name', 'Offline - Processing Steps', 'NumberTitle', 'off', 'WindowState', 'maximized');
for ch = 1:CH_NUM
    subplot(2, n_cols, ch);
    hold on;
    h_raw  = plot(t_steps, raw_dc_timeline(:, ch));
    h_bp   = plot(t_steps, bandpass_timeline(:, ch));
    h_rect = plot(t_steps, rect_timeline(:, ch));
    h_env  = plot(t_steps, env_timeline(:, ch));
    hold off;
    title(sprintf('Channel %d', ch));
    xlabel('Time (s)'); ylabel('Amplitude');
    legend([h_raw h_bp h_rect h_env], ...
        {'raw (DC removed)', 'band-pass', 'rectified', 'envelope'}, ...
        'Location', 'northeast');
end

% Figure 3: detected decisions (press / hold) over the envelope.
figure('Name', 'Offline - Decisions', 'NumberTitle', 'off', 'WindowState', 'maximized');
for ch = 1:CH_NUM
    subplot(2, n_cols, ch);
    hold on;
    plot(t_chunk, env_per_chunk(:, ch), 'Color', [0.8 0.8 0.8]);
    plot(t_chunk, env_dec_tl(:, ch), 'Color', [0.4 0.4 0.4]);
    plot(t_chunk, th_high_tl(:, ch), '--', 'Color', [0.85 0.33 0.10]);
    plot(t_chunk, th_low_tl(:, ch), ':', 'Color', [0.85 0.33 0.10]);
    sel = find([events.ch] == ch);
    for i = sel
        if strcmp(events(i).type, 'hold')
            plot([events(i).onset_s, events(i).offset_s], ...
                [events(i).peak, events(i).peak], 'r-', 'LineWidth', 3);
            text(events(i).onset_s, events(i).peak, ' hold', 'Color', 'r', ...
                'VerticalAlignment', 'bottom');
        else
            plot(events(i).onset_s, events(i).peak, 'b^', 'MarkerFaceColor', 'b');
        end
    end
    hold off;
    np = sum(strcmp({events(sel).type}, 'press'));
    nh = sum(strcmp({events(sel).type}, 'hold'));
    title(sprintf('Channel %d [%s]: %d tap, %d hold', ch, CH_BUTTON{ch}, np, nh));
    xlabel('Time (s)'); ylabel('Envelope');
end

%% Functions
function events = finalize_event(events, ch, onset, off, env_ref, dt, min_press, hold_ch)
    % Append a finished activation to the event list, classified by duration.
    dur_ch = off - onset;
    if dur_ch < min_press
        return;   % too short -> debounce / ignore
    end

    if dur_ch >= hold_ch
        e.type = 'hold';
    else
        e.type = 'press';
    end
    e.ch = ch;
    e.onset_s = onset * dt;
    e.offset_s = off * dt;
    e.dur_s = dur_ch * dt;
    e.peak = max(env_ref(onset:off, ch));

    % Keep field order consistent with the events struct definition.
    e = orderfields(e, {'ch', 'type', 'onset_s', 'offset_s', 'dur_s', 'peak'});
    events(end + 1) = e;
end
