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

% save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('Measurements/%s_%s.mat', "measurements", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "data_mat");

%% saving .wav format
if ~exist("Recordings", 'dir')
    mkdir("Recordings");
end
file_name = sprintf('Recordings/%s_%s.wav', "recorded_audio", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');

% append all data collected
data_full = reshape(data_mat.', 1, []);

% since we use 'double' type for data it needs to be normalized [-1, 1]
y = data_full/65535;
y = 2*y - 1; 

% center data around 0
y = y - mean(y);

% write to the file
audiowrite(file_name, y, Fs);

% play the recorded audio
clear y Fs
[y, Fs] = audioread(file_name);
sound(y, Fs);

%% plot the sound wave
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
