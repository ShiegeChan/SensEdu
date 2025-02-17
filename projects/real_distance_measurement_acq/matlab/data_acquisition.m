%% main.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;
clc;

% close existing serial connections
if ~isempty(instrfind)
    fclose(instrfind);
    delete(instrfind);
end

%% Data Acquisition parameters
ITERATIONS = 500; 
MIC_NUM = 4;
DATA_LENGTH = 64 * 32;
dist_matrix = zeros(MIC_NUM, ITERATIONS); % preallocation of data array

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM9';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

% Configure serial port
configureTerminator(arduino, "LF"); % set terminator to the new line
flush(arduino); % clear the serial buffer

%% Readings Loop
tic;
for it = 1:ITERATIONS
    % Start the acquisition
    write(arduino, 't', "char"); % trigger arduino measurement
    
    % Reading the distance measurements
    dist_matrix(:, it) = read_distance_data(arduino, MIC_NUM);
end
acquisition_time = toc;

% save measurements
file_name = sprintf('%s_%s.mat', "distance_data", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %fsec\n", acquisition_time);

% Close serial connection
clear arduino;

