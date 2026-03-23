%% main.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;
addpath("plot scripts\", "kf algorithm\");
%% Data Acquisition parameters
ITERATIONS = 300; 
MIC_NUM = 4;
mic_name = {"MIC 1", "MIC 2","MIC 3", "MIC 4"};
DATA_LENGTH = 4096;
dist_matrix = zeros(MIC_NUM*3, ITERATIONS); % preallocation of data array
time_axis = zeros(1, ITERATIONS); % preallocation of time array

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

%% Readings Loop
pause(1);
tic;
for it = 1:ITERATIONS
    % Start the acquisition
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    pom = barna_read_distance_data(arduino, MIC_NUM);
    % Reading the distance measurements
    dist_matrix(:, it) = pom;
end
acquisition_time = toc;

% save measurements
file_name = sprintf('%s_%s.mat', "distance_measurements_", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %fsec\n", acquisition_time);

% Close serial connection
arduino = [];

%%
% dist_matrix=outlier_rejection(dist_matrix);
barna_plot_measurements(dist_matrix);
