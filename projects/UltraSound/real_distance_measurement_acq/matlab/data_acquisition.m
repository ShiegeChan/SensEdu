%% main.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;

%% Data Acquisition parameters
ITERATIONS = 300; 
MIC_NUM = 6;
mic_name = {"MIC 1", "MIC 2", "MIC 4", "MIC 8", "MIC 6", "MIC 7"};
DATA_LENGTH = 64 * 32;
dist_matrix = zeros(MIC_NUM, ITERATIONS); % preallocation of data array
time_axis = zeros(1, ITERATIONS); % preallocation of time array

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM20';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

%% Readings Loop
pause(3);
tic;
for it = 1:ITERATIONS
    % Start the acquisition
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    pom = read_distance_data(arduino, MIC_NUM);
    % if (it > 1 && pom())
    %     pom = dist_matrix(:, it-1);
    % end
    % Reading the distance measurements
    dist_matrix(:, it) = pom;
end
acquisition_time = toc;

% save measurements
file_name = sprintf('%s_%s.mat', "Measurements\xxlboard_box", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %fsec\n", acquisition_time);

% Close serial connection
arduino = [];

%% Plotting the data
close all
figure
for i = 1:MIC_NUM
    subplot(MIC_NUM, 1, i);
    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2)
    ylim([0 1])
    xlim([0 time_axis(end)])
    grid on
    xlabel("time [s]");
    ylabel("distance [m]")
    title(mic_name(i));
end

beautify_plot(gcf, 1);
save_plot

%%
figure
for i = 1:MIC_NUM
    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2); hold on;
end
hold off;

%%
ylim([0 1])
xlim([0 time_axis(end)])
grid on
xlabel("time [s]");
ylabel("distance [m]")
legend(mic_name);
title("Microphone distance measurements")

beautify_plot(gcf, 1);
