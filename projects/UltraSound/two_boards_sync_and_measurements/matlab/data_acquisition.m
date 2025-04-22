%% main.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;

%% Data Acquisition parameters
ITERATIONS = 1000; 
MIC_NUM = 4;
DATA_LENGTH = 64 * 32;
dist_matrix = zeros(MIC_NUM, ITERATIONS); % preallocation of data array
time_axis = zeros(1, ITERATIONS); % preallocation of time array

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT_TX = 'COM18';
ARDUINO_PORT_RX = 'COM16';
ARDUINO_BAUDRATE = 115200;
arduino_tx = serialport(ARDUINO_PORT_TX, ARDUINO_BAUDRATE); % select port and baudrate 
arduino_rx = serialport(ARDUINO_PORT_RX, ARDUINO_BAUDRATE); % select port and baudrate 

%% Readings Loop
tic;
for it = 1:ITERATIONS
    % Start the acquisition
    write(arduino_tx, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    pom = read_distance_data(arduino_rx, MIC_NUM);
    if (any(pom < 0.2) && it > 1)
        pom = dist_matrix(:, it-1);
    end
    % Reading the distance measurements
    dist_matrix(:, it) = pom;
end
acquisition_time = toc;

% save measurements
file_name = sprintf('%s_%s.mat', "distance_data", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %fsec\n", acquisition_time);

% Close serial connection
arduino_tx = [];
arduino_rx = [];

%% Plotting the data
figure
for i = 1:MIC_NUM
    subplot(MIC_NUM, 1, i);
    plot(time_axis, dist_matrix(i, :))
    ylim([0 1])
    xlim([0 time_axis(end)])
    grid on

end
