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

%% Decision Settings
% Each channel drives ONE game button. The decision is a GATE, not a
% classifier: a contraction turns the button ON, relaxing turns it OFF.
%   CH4 -> "B"  (Dark Souls: tap = roll/dodge, hold = sprint)
%   CH3 -> "R1" (light attack; repeated taps = combo)
% Dark Souls itself decides roll-vs-sprint from how long B is held, so the
% controller only mirrors the muscle: key DOWN at contraction onset, key UP at
% release. The live port must press the key at the ONSET (state -> active) -
% that is the latency-critical moment - and this offline script records that
% same onset time (onset_s) so the timing here matches the live behaviour.
% DEC_HOLD_S below is only an offline LABEL (tap vs hold) for inspection; the
% live gate simply holds the key for as long as the contraction lasts.
CH_BUTTON = {'-', '-', 'R1', 'B'};   % per-channel game button (ch1..ch4)

DEC_BASE_PCTL   = 20;     % percentile of the envelope used as rest baseline B
DEC_SPAN_PCTL   = 95;     % percentile used as activation level (S = P_high - B)
DEC_FRAC_HIGH   = 0.45;   % onset   threshold = B + DEC_FRAC_HIGH * S
DEC_FRAC_LOW    = 0.18;   % release threshold = B + DEC_FRAC_LOW  * S
DEC_MIN_SPAN    = 25;     % channels with span < this are treated as inactive
DEC_SMOOTH_S    = 0.05;   % causal smoothing of the decision input (s); small
                          % to keep key-down latency low (hysteresis stops
                          % chatter, so heavy smoothing is not needed)
DEC_HANGOVER_S  = 0.12;   % bridge dips shorter than this; small enough that
                          % moderate R1 mashing stays as separate presses
DEC_MIN_PRESS_S = 0.050;  % ignore activations shorter than this (debounce)
DEC_HOLD_S      = 0.40;   % offline label only: gate >= this = "hold" (sprint /
                          % held attack), shorter = "tap" (roll / single attack)
DEC_WARMUP_S    = 1.05;   % ignore the rolling-buffer warm-up at the very start
DEC_CAL_SKIP_S  = 1.5;    % ignore the startup transient when calibrating

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
chunks_per_sec  = Fs / CHUNK_SIZE;
DEC_HANGOVER    = max(1, round(DEC_HANGOVER_S  * chunks_per_sec));
DEC_MIN_PRESS   = max(1, round(DEC_MIN_PRESS_S * chunks_per_sec));
DEC_HOLD_CH     = round(DEC_HOLD_S   * chunks_per_sec);
DEC_WARMUP_CH   = round(DEC_WARMUP_S * chunks_per_sec);
DEC_SMOOTH_CH   = max(1, round(DEC_SMOOTH_S   * chunks_per_sec));
DEC_CAL_SKIP_CH = max(1, round(DEC_CAL_SKIP_S * chunks_per_sec));

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

%% Auto-calibrate per-channel onset / release thresholds from the envelope.
% Robustness to muscle fatigue / electrode position: each channel's thresholds
% are placed between its own rest baseline and activation level, so they scale
% automatically when the envelope is several times larger or smaller.
cal_range = (DEC_CAL_SKIP_CH + 1):num_chunks;   % skip the startup transient
cal_p = struct('base_pctl', DEC_BASE_PCTL, 'span_pctl', DEC_SPAN_PCTL, ...
               'frac_high', DEC_FRAC_HIGH, 'frac_low', DEC_FRAC_LOW, ...
               'min_span',  DEC_MIN_SPAN);
[DEC_TH_HIGH, DEC_TH_LOW, dec_base, dec_span] = ...
    calibrate_thresholds(env_per_chunk(cal_range, :), cal_p);

% Decision input: optionally smooth the per-chunk envelope with a short causal
% moving average to suppress sub-movement ripple without shifting onsets much.
if DEC_SMOOTH_CH > 1
    env_dec = filter(ones(DEC_SMOOTH_CH, 1) / DEC_SMOOTH_CH, 1, env_per_chunk);
else
    env_dec = env_per_chunk;
end

%% Pass 2 - per-channel online decision gate over the envelope.
% Uses the SAME emg_gate_step() the live script uses, so the offline result
% mirrors the live behaviour exactly; this script only adds event bookkeeping
% (tap / hold labelling) for inspection. The warm-up period is skipped so the
% rolling-buffer transient at the very start cannot trigger a press.
gate = struct('mode', zeros(1, CH_NUM), 'onset', zeros(1, CH_NUM), ...
              'off',  zeros(1, CH_NUM), 'gap',   zeros(1, CH_NUM));

% Detected events (filled as activations finish).
events = struct('ch', {}, 'type', {}, 'onset_s', {}, 'offset_s', {}, ...
    'dur_s', {}, 'peak', {});

for k = 1:num_chunks
    if k <= DEC_WARMUP_CH
        continue;   % ignore the rolling-buffer warm-up at the very start
    end
    [gate, ~, done, onset, off] = emg_gate_step(env_dec(k, :), k, gate, ...
        DEC_TH_HIGH, DEC_TH_LOW, DEC_HANGOVER);
    for ch = find(done)
        events = finalize_event(events, ch, onset(ch), off(ch), ...
            env_per_chunk, CHUNK_SIZE / Fs, DEC_MIN_PRESS, DEC_HOLD_CH);
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
fprintf('\nPer-channel auto-calibration (baseline / span -> onset / release):\n');
for ch = 1:CH_NUM
    if isfinite(DEC_TH_HIGH(ch))
        fprintf('  Channel %d: base=%5.0f span=%5.0f -> onset=%5.0f release=%5.0f\n', ...
            ch, dec_base(ch), dec_span(ch), DEC_TH_HIGH(ch), DEC_TH_LOW(ch));
    else
        fprintf('  Channel %d: span=%5.0f < DEC_MIN_SPAN(%d) -> inactive (no electrode)\n', ...
            ch, dec_span(ch), DEC_MIN_SPAN);
    end
end

fprintf('\nButton activity (live port: key DOWN at onset, key UP at release):\n');
fprintf('  tap = quick press (roll / single attack); HOLD = sustained (sprint / held)\n');
for ch = 1:CH_NUM
    if ~isfinite(DEC_TH_HIGH(ch))
        continue;   % inactive channel (no electrode) -> no button
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

% Figure 2: all processing steps overlaid per channel, with the decision
% thresholds marked (matches the live "Processing Steps" view).
figure('Name', 'Offline - Processing Steps', 'NumberTitle', 'off', 'WindowState', 'maximized');
for ch = 1:CH_NUM
    subplot(2, n_cols, ch);
    hold on;
    h_raw  = plot(t_steps, raw_dc_timeline(:, ch));
    h_bp   = plot(t_steps, bandpass_timeline(:, ch));
    h_rect = plot(t_steps, rect_timeline(:, ch));
    h_env  = plot(t_steps, env_timeline(:, ch));
    if isfinite(DEC_TH_HIGH(ch))
        yline(DEC_TH_HIGH(ch), '--', 'onset');
        yline(DEC_TH_LOW(ch), ':', 'release');
    end
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
    plot(t_chunk, env_dec(:, ch), 'Color', [0.4 0.4 0.4]);
    if isfinite(DEC_TH_HIGH(ch))
        yline(DEC_TH_HIGH(ch), '--k', 'onset');
        yline(DEC_TH_LOW(ch), ':k', 'release');
    end
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
