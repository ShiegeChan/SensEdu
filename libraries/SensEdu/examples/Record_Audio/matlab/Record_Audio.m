%% Record_Audio.m
% Recording audio data, saving it to the .wav file and plotting the data
clear;
close all;
clc;
%% Settings
ARDUINO_PORT = 'COM22';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 500;
DATA_LENGTH = 2000; % make sure to match this number with firmware
Fs = 44100; % sampling frequency (in Hz) for .wav file
%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate
%% Recording Loop
data = zeros(1,ITERATIONS);
time_axis = zeros(1,ITERATIONS);
data_mat = zeros(ITERATIONS, DATA_LENGTH);
tic;
for it = 1:ITERATIONS
    % Data readings
    write(arduino, 't', "char"); % trigger arduino measurement
    if (it == 1) 
        disp('Recording started...');
    end
    time_axis(it) = toc;
    tic
    data = read_data(arduino, DATA_LENGTH);
    data_mat(it, :) = data;
    toc
end
disp('Recording ended.')

% set COM port back free
arduino = [];

% save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('Measurements/%s_%s.mat', "measurements", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "data", "time_axis");

% calculate average time between measurements
buf = time_axis(2) - time_axis(1);
for i = 2:(length(time_axis) - 1)
    buf = mean([buf, (time_axis(i+1) - time_axis(i))]);
end

fprintf("average time between measurements: %fsec\n", abs(buf));


%% saving .wav format
filename = 'recorded_audio.wav';

% append all data collected
data_full = reshape(data_mat.', 1, []);

% since we use 'double' type for data it needs to be normalized [-1, 1]
y = (65535.0 - data_full) / 65535.0; 

% center data around 0
y = y - mean(y);

% write to the file
audiowrite(filename, y, Fs);

% play the recorded audio
clear y Fs
[y, Fs] = audioread(filename);
sound(y, Fs);

% plot the sound wave
figure; 
plot(y);
title("Recorded Audio Wave");
ylabel("Normalized ADC Output");
xlabel("Samples");

