%% Record_Audio.m
% Reference host script for the Record_Audio firmware example.
%
% The firmware fills SDRAM "segments" of a fixed size (SEGMENT_SECONDS in
% Record_Audio.ino) and streams each filled segment over USB CDC. This
% script only needs to know the desired total recording duration; it
% computes how many segments to consume and trims to the requested length.
%
% Pipeline:
%   1. Open the serial port. onCleanup guarantees we send 'p' and close
%      the port on any failure path.
%   2. Send 'p' (idempotent stop). Read the framed ACK. Any in-flight
%      transfer from a prior run is implicitly drained by the resync.
%   3. Send 's'. Read the framed ACK and capture session_id.
%   4. For each segment: read the framed header, validate magic +
%      session_id + sequence continuity, then read sample_count uint16s.
%   5. Send 'p', read the framed ACK.
%   6. Trim to RECORDING_DURATION_SEC, save WAV, plot waveform + FFT.
%
% Protocol (must match Record_Audio.ino):
%
%   ACK frame (16 bytes):
%     uint32 magic      = 0x41434B21
%     uint8  cmd        's' | 'p' | '?'
%     uint8  state      0=IDLE, 1=RECORDING
%     uint16 pad
%     uint32 session_id
%     uint32 info       command-specific
%
%   Segment header (20 bytes), precedes each segment payload:
%     uint32 magic        = 0x5345474D
%     uint32 session_id
%     uint32 sequence_id  0-based within the session
%     uint32 sample_count uint16 samples that follow
%     uint32 flags        bit 0: OVERRUN_DROPPED

clear;
close all;
clc;

%% User settings
ARDUINO_PORT           = 'COM16';
ARDUINO_BAUDRATE       = 2000000;   % cosmetic for USB CDC
RECORDING_DURATION_SEC = 40;        % desired total audio length
ENABLE_PLAYBACK        = false;

%% Firmware-coupled constants
Fs                   = 44100;
SEGMENT_SECONDS      = 30;
SEG_MAGIC            = uint32(hex2dec('5345474D'));
ACK_MAGIC            = uint32(hex2dec('41434B21'));
SEG_HDR_BYTES        = 20;
ACK_BYTES            = 16;
FLAG_OVERRUN_DROPPED = uint32(1);

%% Derived
SEGMENT_SAMPLES    = SEGMENT_SECONDS * Fs;
SEGMENTS_TO_RECORD = ceil(RECORDING_DURATION_SEC / SEGMENT_SECONDS);

% Generous timeouts:
%   * a slot fills in SEGMENT_SECONDS, so headers arrive at that cadence
%   * USB-FS at ~1 MB/s transfers a 2.6 MB slot in ~3 s
HEADER_WAIT_SEC  = SEGMENT_SECONDS + 10;
PAYLOAD_WAIT_SEC = SEGMENT_SECONDS + 10;

% First 'p' may have to consume a full prior-session slot of stale data
% before the ACK arrives; subsequent ACKs come back promptly.
FIRST_ACK_WAIT_SEC = 15;
ACK_WAIT_SEC       = 5;

% Worst-case resync window: up to one full slot of stale payload + a header.
% A factor of 2 absorbs any USB FIFO tail.
RESYNC_MAX_BYTES = 2 * SEGMENT_SAMPLES * 2 + SEG_HDR_BYTES;

%% Open port (with safe teardown on any error path)
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);
arduino.Timeout = 5;
cleanup_obj = onCleanup(@() safe_close(arduino)); %#ok<NASGU>
flush(arduino);

%% Reset firmware to a known IDLE state
% 'p' is idempotent in firmware. It cancels any in-flight transfer and
% aborts capture so the next 's' starts from a known baseline.
write(arduino, uint8('p'), 'uint8');
read_ack(arduino, 'p', FIRST_ACK_WAIT_SEC, RESYNC_MAX_BYTES, ...
         ACK_MAGIC, ACK_BYTES);

%% Start a fresh session
write(arduino, uint8('s'), 'uint8');
start_ack = read_ack(arduino, 's', ACK_WAIT_SEC, ACK_BYTES * 2, ...
                     ACK_MAGIC, ACK_BYTES);
session_id = start_ack.session_id;

fprintf('Session %u: recording %d s in %d segment(s) of %d s...\n', ...
        session_id, RECORDING_DURATION_SEC, SEGMENTS_TO_RECORD, SEGMENT_SECONDS);

%% Receive segments
data_full   = zeros(1, SEGMENTS_TO_RECORD * SEGMENT_SAMPLES);
write_pos   = 0;
last_seq_id = -1;
overrun_segments = 0;

for seg = 1:SEGMENTS_TO_RECORD
    hdr = read_segment_header(arduino, HEADER_WAIT_SEC, ...
                              RESYNC_MAX_BYTES, SEG_MAGIC, SEG_HDR_BYTES);

    if hdr.session_id ~= session_id
        error('Record_Audio:sessionMismatch', ...
              'Segment session_id mismatch: expected %u, got %u.', ...
              session_id, hdr.session_id);
    end
    if double(hdr.sequence_id) ~= (last_seq_id + 1)
        error('Record_Audio:sequenceGap', ...
              'Segment sequence_id discontinuity: expected %d, got %u.', ...
              last_seq_id + 1, hdr.sequence_id);
    end
    last_seq_id = double(hdr.sequence_id);

    is_overrun = bitand(hdr.flags, FLAG_OVERRUN_DROPPED) ~= 0;
    if is_overrun
        overrun_segments = overrun_segments + 1;
    end

    samples = read_samples(arduino, double(hdr.sample_count), PAYLOAD_WAIT_SEC);
    data_full(write_pos + 1 : write_pos + numel(samples)) = samples;
    write_pos = write_pos + numel(samples);

    if is_overrun
        fprintf('  segment %u: %u samples [OVERRUN: gap before this segment]\n', ...
                hdr.sequence_id, hdr.sample_count);
    else
        fprintf('  segment %u: %u samples\n', hdr.sequence_id, hdr.sample_count);
    end
end

%% Stop the session
write(arduino, uint8('p'), 'uint8');
try
    read_ack(arduino, 'p', ACK_WAIT_SEC, RESYNC_MAX_BYTES, ACK_MAGIC, ACK_BYTES);
catch err
    warning(err.identifier, '%s', err.message);
end

disp('Recording ended.');

%% Trim to requested duration
data_full = data_full(1:write_pos);
target_samples = RECORDING_DURATION_SEC * Fs;
if numel(data_full) > target_samples
    data_full = data_full(1:target_samples);
end

if overrun_segments > 0
    warning('Record_Audio:overrun', ...
            'Recording completed with %d overrun segment(s); audio has gaps.', ...
            overrun_segments);
end

%% Save WAV (ADC is 16-bit; normalize and center)
if ~exist('Recordings', 'dir')
    mkdir('Recordings');
end
ts = char(datetime('now', 'Format', 'yyyyMMdd_HHmmss'));
file_name = sprintf('Recordings/recorded_audio_%s.wav', ts);

y = data_full / 65535;
y = 2 * y - 1;
y = y - mean(y);
t = (0:numel(y)-1) / Fs;

audiowrite(file_name, y, Fs);
fprintf('Saved: %s\n', file_name);

%% FFT
fft_in = data_full - mean(data_full);
Y = fft(fft_in);
L = numel(fft_in);
f = (-L/2:L/2-1) * (Fs/L);

figure;
semilogy(f, abs(fftshift(Y))/L, 'LineWidth', 2);
xlabel('Frequency (Hz)');
ylabel('Amplitude');
title('FFT of recorded data');
grid on;

%% Time-domain
figure;
plot(t, y);
title('Recorded Audio Signal');
xlabel('time [s]');
ylabel('Normalized ADC Output');
ylim([-1 1]);
xlim([0 t(end)]);
grid on;

%% Optional playback
if ENABLE_PLAYBACK
    player = audioplayer(y, Fs); %#ok<NASGU>
    play(player);
end

%% Functions

% read_ack
% Resync onto ACK_MAGIC and validate structural fields (cmd, state, pad)
% before accepting. If the magic matches but the frame fails validation,
% keep searching: a 4-byte slice of stale audio data has a non-zero chance
% of coincidentally matching the magic and must not be mistaken for an ACK.
function ack = read_ack(arduino, expected_cmd, timeout_sec, max_resync_bytes, magic, ack_bytes)
    validator = @(buf) ack_is_valid(buf, expected_cmd);
    raw = read_framed(arduino, ack_bytes, timeout_sec, max_resync_bytes, ...
                      magic, validator);

    ack.magic      = typecast(uint8(raw(1:4)),   'uint32');
    ack.cmd        = char(raw(5));
    ack.state      = uint8(raw(6));
    ack.pad        = typecast(uint8(raw(7:8)),   'uint16');
    ack.session_id = typecast(uint8(raw(9:12)),  'uint32');
    ack.info       = typecast(uint8(raw(13:16)), 'uint32');
end

function ok = ack_is_valid(buf, expected_cmd)
    cmd_char = char(buf(5));
    state_b  = buf(6);
    pad_w    = typecast(uint8(buf(7:8)), 'uint16');
    ok = (cmd_char == expected_cmd) ...
         && (state_b == 0 || state_b == 1) ...
         && (pad_w == 0);
end

% read_segment_header
function hdr = read_segment_header(arduino, timeout_sec, max_resync_bytes, magic, hdr_bytes)
    validator = @(buf) seg_is_valid(buf);
    raw = read_framed(arduino, hdr_bytes, timeout_sec, max_resync_bytes, ...
                      magic, validator);

    hdr.magic        = typecast(uint8(raw(1:4)),   'uint32');
    hdr.session_id   = typecast(uint8(raw(5:8)),   'uint32');
    hdr.sequence_id  = typecast(uint8(raw(9:12)),  'uint32');
    hdr.sample_count = typecast(uint8(raw(13:16)), 'uint32');
    hdr.flags        = typecast(uint8(raw(17:20)), 'uint32');
end

function ok = seg_is_valid(buf)
    % SEGMENT_SAMPLES upper bound for sample_count + flags must be 0 or 1
    sample_count = typecast(uint8(buf(13:16)), 'uint32');
    flags        = typecast(uint8(buf(17:20)), 'uint32');
    ok = (sample_count > 0) && (sample_count <= 44100 * 60) && (flags <= 1);
end

% read_framed
% Reads frame_bytes starting with the 4-byte little-endian magic. If the
% initial read does not begin with the magic, OR the optional validator
% rejects the frame, slides one byte at a time (fast: from serialport's
% internal buffer, no per-byte USB round trip). Resync budget is bounded.
function raw = read_framed(arduino, frame_bytes, timeout_sec, max_resync_bytes, magic, validator)
    if nargin < 6
        validator = @(buf) true;
    end
    prev_timeout = arduino.Timeout;
    arduino.Timeout = timeout_sec;
    cleanup = onCleanup(@() set_timeout(arduino, prev_timeout)); %#ok<NASGU>

    buf = read_exact(arduino, frame_bytes);
    consumed = 0;
    while true
        w = typecast(uint8(buf(1:4)), 'uint32');
        if w == magic && validator(buf)
            raw = buf;
            return;
        end
        if consumed >= max_resync_bytes
            error('Record_Audio:resyncFailed', ...
                  'Failed to resynchronize on frame magic within %d bytes.', ...
                  max_resync_bytes);
        end
        next_byte = read_exact(arduino, 1);
        buf = [buf(2:end), next_byte]; %#ok<AGROW>
        consumed = consumed + 1;
    end
end

% read_exact
function out = read_exact(arduino, n)
    out = read(arduino, n, 'uint8');
    if numel(out) < n
        error('Record_Audio:readTimeout', ...
              'Timed out reading %d bytes (got %d).', n, numel(out));
    end
end

% read_samples
function data = read_samples(arduino, sample_count, timeout_sec)
    prev_timeout = arduino.Timeout;
    arduino.Timeout = timeout_sec;
    cleanup = onCleanup(@() set_timeout(arduino, prev_timeout)); %#ok<NASGU>

    raw = read(arduino, sample_count, 'uint16');
    if numel(raw) < sample_count
        error('Record_Audio:payloadTimeout', ...
              'Timed out reading segment payload (got %d / %d samples) after %.1f s.', ...
              numel(raw), sample_count, timeout_sec);
    end
    data = double(raw);
end

% safe_close
% Called by onCleanup on script exit (normal or error). Best-effort stops
% the firmware and releases the port; failures are swallowed because the
% port may already be in a bad state.
function safe_close(arduino)
    if ~isvalid(arduino)
        return;
    end
    try
        write(arduino, uint8('p'), 'uint8');
    catch
    end
    delete(arduino);
end

% set_timeout
% Helper for onCleanup; must be a function because property assignment is
% not a valid anonymous-function expression.
function set_timeout(arduino, value)
    if isvalid(arduino)
        arduino.Timeout = value;
    end
end
