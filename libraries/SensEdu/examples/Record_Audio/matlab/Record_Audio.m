%% Record_Audio.m
% Recording audio data, saving it to the .wav file and plotting the data
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM22';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 500; % Match this number with `LOOP_COUNT` in firmware
DATA_LENGTH = 2048; % Match this number with `mic_data_size` in firmware
CHUNK_SIZE = 32; % Match this number with number of byte chunks in firmware
Fs = 44100; % Sampling frequency (in Hz) for .wav file

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Select port and baudrate

%% Recording Loop
data = zeros(1,ITERATIONS);
data_mat = zeros(ITERATIONS, DATA_LENGTH);
disp('Recording started...');
write(arduino, 't', "char"); % Trigger arduino measurement
for it = 1:ITERATIONS
    data = read_data(arduino, DATA_LENGTH, CHUNK_SIZE);
    data_mat(it, :) = data;
end
disp('Recording ended.');

% Set COM port back free
arduino = [];

%% Saving .wav format
if ~exist("Recordings", 'dir')
    mkdir("Recordings");
end
file_name = sprintf('Recordings/%s_%s.wav', "recorded_audio", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');

% Append all data collected
data_full = reshape(data_mat.', 1, []);

% Data normalization [-1, 1]
y = data_full/65535;
y = 2*y - 1; 

% Center data around 0
y = y - mean(y);

% Convert samples to time 
t = linspace(0, length(y)/Fs, length(y));

% Write to the file 
audiowrite(file_name, y, Fs);

% Read from the file
clear y Fs
[y, Fs] = audioread(file_name);

%% Visualization 
figure; 
plot(t, y); hold on;
title("Recorded Audio Signal");
xlabel("time [s]"); ylabel("Normalized ADC Output");
ylim([-1 1]);
xlim([0 t(end)]);

% Dynamic marker
playback_marker = line([0 0], [-0.5 0.5], 'Color', [0.4 0.12 0.54], 'LineWidth', 1.4, 'Marker', '.');
legend("Microphone signal", " ");

% Audio player for playback 
player = audioplayer(y, Fs);

% Start audio 
play(player);

% Update the marker in real time
while isplaying(player)
    % Current time in audio
    t_now = player.CurrentSample / Fs ; 

    % Update the marker position
    set(playback_marker, 'XData', [t_now t_now]); 

    % Neccesarry pause for updates
    pause(0.01);
end

%% Plot the sound wave - debugging purposes
figure; 
plot(y);
title("Recorded Audio Wave");
ylabel("Normalized ADC Output");
xlabel("Samples");

% Plot USB package transitions
hold on;
package_idxs = [DATA_LENGTH, DATA_LENGTH+1];
for it = 2:ITERATIONS
    package_idxs = [package_idxs, DATA_LENGTH*it, ((DATA_LENGTH*it)+1)];
end
package_idxs = package_idxs(1:(end-1));
scatter(package_idxs, y(package_idxs));
