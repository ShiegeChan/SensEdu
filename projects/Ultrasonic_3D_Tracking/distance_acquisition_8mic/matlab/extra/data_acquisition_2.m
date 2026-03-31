%% main.m
% Reads config data and then ADC mics measurements from Arduino
clear;
close all;

%% Data Acquisition parameters
ACQUISITION_DURATION = 2*60;  % Duration in seconds
SAMPLING_RATE = 40;         % 40 measurements per second
MIC_NUM = 4;

% Initialize variables
dist_matrix = [];  % Dynamic array to store measurements
time_axis = [];    % Dynamic array to store timestamps

%% Arduino Setup + Config
% Serial port configuration
ARDUINO_PORT = 'COM13';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);  % Select port and baudrate

%% Readings Loop
tic;
while toc < ACQUISITION_DURATION
    % Start the acquisition
    write(arduino, 't', "char");  % Trigger Arduino measurement
    
    % Read distance data
    pom = read_distance_data(arduino, MIC_NUM);
    
    % Validate the data
    if any(pom < 0.2) && ~isempty(dist_matrix)
        pom = dist_matrix(:, end);  % Use the last valid measurement
    end
    
    % Store the measurement and timestamp
    dist_matrix = [dist_matrix, pom];
    time_axis = [time_axis, toc];
    
    % Pause to maintain the sampling rate
    pause(1 / SAMPLING_RATE);
end
acquisition_time = toc;

% Save measurements
file_name = sprintf('%s_%s.mat', "distance_data", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %f sec\n", acquisition_time);

% Close serial connection
clear arduino;

%% Plotting the data
figure;
for i = 1:MIC_NUM
    subplot(MIC_NUM, 1, i);
    plot(time_axis, dist_matrix(i, :));
    ylim([0 1.2]);
    xlim([0 time_axis(end)]);
    grid on;
    title(sprintf('Microphone %d', i));
    xlabel('Time (s)');
    ylabel('Distance (m)');
end