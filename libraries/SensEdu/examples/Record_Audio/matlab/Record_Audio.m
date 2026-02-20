%% Record_Audio.m
% Records the audio data, saves it to the .wav file and plots the data
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM9';
ARDUINO_BAUDRATE = 2000000;

RECORDING_DURATION_SEC = 30;
Fs = 44100;

BUF_SIZE = 64;
HALF_BUF_SIZE = BUF_SIZE/2;

CHUNK_SIZE = 64; % Bytes per USB request

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate
flush(arduino);

%% Recording Loop
iteration_num = round(RECORDING_DURATION_SEC * Fs / HALF_BUF_SIZE) + 1;
data = zeros(HALF_BUF_SIZE, iteration_num);

% Trigger the measurement
write(arduino, 't', "char"); % trigger arduino measurement
disp('Recording started...');

% Send the loop config
write(arduino, uint32(iteration_num), "uint32");

for it = 1:iteration_num
    data(:, it) = read_data(arduino, HALF_BUF_SIZE, CHUNK_SIZE);
end

disp('Recording ended.');

% set COM port back free
arduino = [];

%% saving .wav format
if ~exist("Recordings", 'dir')
    mkdir("Recordings");
end
file_name = sprintf('Recordings/%s_%s.wav', "recorded_audio", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');

% append all data collected
data_full = reshape(data, 1, []);

% data normalization [-1, 1]
y = data_full/65535;
y = 2*y - 1; 

% center data around 0
y = y - mean(y);

% convert samples to time 
t = linspace(0, length(y)/Fs, length(y));

% write to the file 
audiowrite(file_name, y, Fs);

% read from the file
clear y Fs
[y, Fs] = audioread(file_name);

%% FFT
data_full = double(reshape(data.', 1, []));
data_full = data_full - mean(data_full);
Y = fft(data_full);
L = numel(data_full);
f = (-L/2:L/2-1) * (Fs/L);

figure;
semilogy(f, abs(fftshift(Y))/L, 'LineWidth', 3);
xlabel('Frequency (Hz)');
ylabel('Amplitude');
title('FFT of concatenated data');

%% Visualization 
figure; 
plot(t, y); hold on;
title("Recorded Audio Signal");
xlabel("time [s]"); ylabel("Normalized ADC Output");
ylim([-1 1]);
xlim([0 t(end)]);

% dynamic marker
playback_marker = line([0 0], [-0.5 0.5], 'Color', [0.4 0.12 0.54], 'LineWidth', 1.4, 'Marker', '.');
legend("Microphone signal", " ");

% audio player for playback
player = audioplayer(y, Fs);

% start audio
play(player);

% update the marker in real time
while isplaying(player)
    % current time in audio
    t_now = player.CurrentSample / Fs;
    
    % update the marker position
    set(playback_marker, 'XData', [t_now t_now]);
    
    % neccesarry pause for updates
    pause(0.01);
end

%% plot the sound wave - debugging purposes
figure;
plot(y);
title("Recorded Audio Wave");
ylabel("Normalized ADC Output");
xlabel("Samples");

% plot USB package transitions
hold on;
package_idxs = zeros(1, 2 * (iteration_num - 1)); 
package_idxs = [HALF_BUF_SIZE, HALF_BUF_SIZE+1];
for it = 2:iteration_num
    package_idxs = [package_idxs, HALF_BUF_SIZE*it, ((HALF_BUF_SIZE*it)+1)];
end
package_idxs = package_idxs(1:(end-1));
scatter(package_idxs, y(package_idxs));

%% Functions
function data = read_data(arduino, buf_size, chunk_size)
    % 2 bytes per sample
    total_byte_length = buf_size * 2;
    serial_rx_data = zeros(1, total_byte_length, 'uint8');
    bytes_read = 0;
    while bytes_read < total_byte_length 
        transfer_size = min(chunk_size, total_byte_length - bytes_read);
        serial_rx_data(bytes_read + 1 : bytes_read + transfer_size) = read(arduino, transfer_size, 'uint8');
        bytes_read = bytes_read + transfer_size;
    end
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end