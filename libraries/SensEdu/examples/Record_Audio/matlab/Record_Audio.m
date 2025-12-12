%% Record_Audio.m
% Recording audio data, saving it to the .wav file and plotting the data
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM22';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 500; % match this number with `LOOP_COUNT` in firmware
DATA_LENGTH = 2048; % match this number with `mic_data_size` in firmware
Fs = 44100; % sampling frequency (in Hz) for .wav file

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate

%% Recording Loop
data = zeros(1,ITERATIONS);
data_mat = zeros(ITERATIONS, DATA_LENGTH);
disp('Recording started...');
write(arduino, 't', "char"); % trigger arduino measurement
for it = 1:ITERATIONS
    data = read_data(arduino, DATA_LENGTH);
    data_mat(it, :) = data;
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
data_full = reshape(data_mat.', 1, []);

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
    t_now = player.CurrentSample / Fs ; 

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
package_idxs = [DATA_LENGTH, DATA_LENGTH+1];
for it = 2:ITERATIONS
    package_idxs = [package_idxs, DATA_LENGTH*it, ((DATA_LENGTH*it)+1)];
end
package_idxs = package_idxs(1:(end-1));
scatter(package_idxs, y(package_idxs));
