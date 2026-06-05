%% Audio_Recording.m
% Host script: drive the start/stop handshake, read framed segments over
% USB CDC from Audio_Recording.ino, save WAV, plot waveform + FFT.

clear;
close all;

%% User settings
ARDUINO_PORT           = 'COM16';
ARDUINO_BAUDRATE       = 2000000;   % Cosmetic for USB CDC
RECORDING_DURATION_SEC = 40;
ENABLE_PLAYBACK        = true;

%% Firmware constants
Fs                   = 44100;
SEGMENT_SECONDS      = 30;
SEG_MAGIC            = uint32(hex2dec('5345474D'));
SEG_TAIL_MAGIC       = uint32(hex2dec('53454754'));
ACK_MAGIC            = uint32(hex2dec('41434B21'));
SEG_HDR_BYTES        = 20;
SEG_TAIL_BYTES       = 8;
ACK_BYTES            = 16;
FLAG_OVERRUN_DROPPED = uint32(1);

%% Timeouts
HEADER_WAIT_SEC = SEGMENT_SECONDS + 10;
PAYLOAD_WAIT_SEC = SEGMENT_SECONDS + 10;
ACK_WAIT_SEC = 5;

% First pause 'p' may need to drain the entire prior session.
FIRST_ACK_WAIT_SEC = 15;

%% Derived
SEGMENT_SAMPLES = SEGMENT_SECONDS * Fs;
SEGMENTS_TO_RECORD = ceil(RECORDING_DURATION_SEC / SEGMENT_SECONDS);

% Worst case: one full stale segment payload.
RESYNC_MAX_BYTES = 2 * SEGMENT_SAMPLES * 2 + SEG_HDR_BYTES;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);
arduino.Timeout = 5;
cleanup = onCleanup(@() safe_close(arduino));
flush(arduino);

%% Reset firmware to IDLE state
write(arduino, uint8('p'), 'uint8');
read_ack(arduino, 'p', FIRST_ACK_WAIT_SEC, RESYNC_MAX_BYTES, ACK_MAGIC, ACK_BYTES);

%% Start new session
write(arduino, uint8('s'), 'uint8');
start_ack = read_ack(arduino, 's', ACK_WAIT_SEC, ACK_BYTES * 2, ACK_MAGIC, ACK_BYTES);
session_id = start_ack.session_id;

fprintf('Session %u: recording %d s in %d segment(s) of %d s...\n', ...
        session_id, RECORDING_DURATION_SEC, SEGMENTS_TO_RECORD, SEGMENT_SECONDS);

%% Read segments
data_full = zeros(1, SEGMENTS_TO_RECORD * SEGMENT_SAMPLES);
write_pos = 0;
last_seq_id = -1;
overrun_segments = 0;
partial_recording = false;

for seg = 1:SEGMENTS_TO_RECORD
    hdr = read_segment_header(arduino, HEADER_WAIT_SEC, RESYNC_MAX_BYTES, ...
                              SEG_MAGIC, SEG_HDR_BYTES, session_id, SEGMENT_SAMPLES);

    if double(hdr.sequence_id) ~= (last_seq_id + 1)
        warning(['Segment sequence_id discontinuity: expected %d, got %u.\n' ...
                 'Stopping early and keeping data captured so far.'], ...
                last_seq_id + 1, hdr.sequence_id);
        partial_recording = true;
        break;
    end
    last_seq_id = double(hdr.sequence_id);

    is_overrun = bitand(hdr.flags, FLAG_OVERRUN_DROPPED) ~= 0;
    if is_overrun
        overrun_segments = overrun_segments + 1;
    end

    samples = read_samples(arduino, double(hdr.sample_count), PAYLOAD_WAIT_SEC);

    tail_ok = read_segment_tail(arduino, PAYLOAD_WAIT_SEC, SEG_TAIL_MAGIC, ...
                                SEG_TAIL_BYTES, hdr.sequence_id);

    data_full(write_pos + 1 : write_pos + numel(samples)) = samples;
    write_pos = write_pos + numel(samples);

    if is_overrun
        fprintf('  segment %u: %u samples [OVERRUN: gap before this segment]\n', ...
                hdr.sequence_id, hdr.sample_count);
    else
        fprintf('  segment %u: %u samples\n', hdr.sequence_id, hdr.sample_count);
    end

    if ~tail_ok
        warning(['Segment tail mismatch after seq %u; stream misaligned.\n' ...
                 'Stopping early and keeping data captured so far.'], ...
                hdr.sequence_id);
        partial_recording = true;
        break;
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

if partial_recording
    actual_duration_sec = numel(data_full) / Fs;
    fprintf('Requested duration: %.1f s | Actual duration: %.1f s (%.1f%%)\n', ...
            RECORDING_DURATION_SEC, actual_duration_sec, ...
            100 * actual_duration_sec / RECORDING_DURATION_SEC);
    warning('Recording is shorter than requested by %.1f s due to early stop.', ...
            RECORDING_DURATION_SEC - actual_duration_sec);
end

if overrun_segments > 0
    warning('Recording completed with %d overrun segment(s); audio has gaps.', overrun_segments);
end

%% Save WAV
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
ylim([-1, 1]);
xlim([0, t(end)]);
grid on;

%% Optional playback
if ENABLE_PLAYBACK
    player = audioplayer(y, Fs);
    play(player);
end

%% Functions

% Reads a magic-prefixed frame.
% On mismatch, reads a chunk and scans it for magic, then validates the frame.
% Bounded by max_resync_bytes total bytes consumed.
function raw = read_framed(arduino, frame_bytes, timeout_sec, max_resync_bytes, magic, validator)
    if nargin < 6
        validator = @(buf) true;
    end

    arduino.Timeout = timeout_sec;

    % Bulk scan chunk size.
    SCAN_CHUNK = 65536;

    magic_bytes = typecast(uint32(magic), 'uint8');

    buf = read_exact(arduino, frame_bytes);
    consumed = 0;
    while true
        % Fast-path: frame at buf(1:frame_bytes).
        w = typecast(uint8(buf(1:4)), 'uint32');
        if w == magic && validator(buf)
            raw = buf;
            return;
        end

        if consumed >= max_resync_bytes
            error('Failed to resynchronize on frame magic within %d bytes.', max_resync_bytes);
        end

        % Bulk-read: magic search through the entire buf.
        to_read = min(SCAN_CHUNK, max_resync_bytes - consumed);
        chunk = read_exact(arduino, to_read);
        search_space = uint8([buf(2:end), chunk]);

        % Find first occurrence of magic_bytes in search_space.
        idx = find_pattern(search_space, magic_bytes);

        if isempty(idx)
            buf = search_space(end - frame_bytes + 2 : end);
            consumed = consumed + to_read;
        else
            available = numel(search_space) - idx + 1;
            if available < frame_bytes
                extra = read_exact(arduino, frame_bytes - available);
                search_space = [search_space, extra];
            end
            buf = search_space(idx : idx + frame_bytes - 1);
            consumed = consumed + (idx - 1);
        end
    end
end

% Locates the first occurrence of a short byte pattern in a uint8 array.
% Returns [] if not found.
function idx = find_pattern(data, pattern)
    n = numel(pattern);
    if numel(data) < n
        idx = [];
        return;
    end
    candidates = find(data(1:end - n + 1) == pattern(1));
    for k = 1:numel(candidates)
        c = candidates(k);
        if isequal(data(c : c + n - 1), pattern)
            idx = c;
            return;
        end
    end
    idx = [];
end

% Reads specified number of bytes.
function out = read_exact(arduino, n)
    out = read(arduino, n, 'uint8');
    if numel(out) < n
        error('Timed out reading %d bytes (got %d).', n, numel(out));
    end
end

% Reads and validates an ACK frame.
function ack = read_ack(arduino, expected_cmd, timeout_sec, max_resync_bytes, magic, ack_bytes)
    validator = @(buf) ack_is_valid(buf, expected_cmd);
    raw = read_framed(arduino, ack_bytes, timeout_sec, max_resync_bytes, magic, validator);

    ack.magic      = typecast(uint8(raw(1:4)),   'uint32');
    ack.cmd        = char(raw(5));
    ack.state      = uint8(raw(6));
    ack.pad        = typecast(uint8(raw(7:8)),   'uint16');
    ack.session_id = typecast(uint8(raw(9:12)),  'uint32');
    ack.info       = typecast(uint8(raw(13:16)), 'uint32');
end

% Validates ACK frame fields.
function ok = ack_is_valid(buf, expected_cmd)
    cmd_char = char(buf(5));
    state = buf(6);
    pad = typecast(uint8(buf(7:8)), 'uint16');
    ok = (cmd_char == expected_cmd) && (state == 0 || state == 1) && (pad == 0);
end

% Reads and validates a segment header.
function hdr = read_segment_header(arduino, timeout_sec, max_resync_bytes, ...
                                   magic, hdr_bytes, expected_session_id, max_samples)
    validator = @(buf) seg_is_valid(buf, expected_session_id, max_samples);
    raw = read_framed(arduino, hdr_bytes, timeout_sec, max_resync_bytes, magic, validator);

    hdr.magic        = typecast(uint8(raw(1:4)),   'uint32');
    hdr.session_id   = typecast(uint8(raw(5:8)),   'uint32');
    hdr.sequence_id  = typecast(uint8(raw(9:12)),  'uint32');
    hdr.sample_count = typecast(uint8(raw(13:16)), 'uint32');
    hdr.flags        = typecast(uint8(raw(17:20)), 'uint32');
end

% Validates segment header fields.
function ok = seg_is_valid(buf, expected_session_id, max_samples)
    session_id   = typecast(uint8(buf(5:8)),   'uint32');
    sample_count = typecast(uint8(buf(13:16)), 'uint32');
    flags        = typecast(uint8(buf(17:20)), 'uint32');
    ok = (session_id == expected_session_id) && (sample_count > 0) ...
         && (sample_count <= max_samples) && (flags <= 1);
end

% Reads and validates a segment tail.
function ok = read_segment_tail(arduino, timeout_sec, magic, tail_bytes, expected_sequence_id)
    arduino.Timeout = timeout_sec;
    raw = read(arduino, tail_bytes, 'uint8');
    if numel(raw) < tail_bytes
        error('Timed out reading segment tail (got %d / %d bytes).', ...
              numel(raw), tail_bytes);
    end
    got_magic = typecast(uint8(raw(1:4)), 'uint32');
    got_seq   = typecast(uint8(raw(5:8)), 'uint32');
    ok = (got_magic == magic) && (got_seq == expected_sequence_id);
end

% Read sample_count*2 bytes and reinterpret as uint16.
function data = read_samples(arduino, sample_count, timeout_sec)
    arduino.Timeout = timeout_sec;

    n_bytes = sample_count * 2;
    raw_bytes = read(arduino, n_bytes, 'uint8');
    if numel(raw_bytes) < n_bytes
        error('Timed out reading segment payload (got %d / %d bytes) after %.1f s.', ...
              numel(raw_bytes), n_bytes, timeout_sec);
    end
    data = double(typecast(uint8(raw_bytes), 'uint16'));
end

% Stop + port release (called by onCleanup or script exit).
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
