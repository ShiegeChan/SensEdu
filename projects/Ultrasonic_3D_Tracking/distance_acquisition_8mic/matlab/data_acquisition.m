%% main.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;
addpath("plot scripts\", "kf algorithm\");
%% Data Acquisition parameters
ITERATIONS = 100; 
MIC_NUM = 8*3;
mic_name = {"MIC 1", "MIC 2","MIC 3", "MIC 4", "MIC 8", "MIC 6", "MIC 5", "MIC 7"};
DATA_LENGTH = 2048;
dist_matrix = zeros(MIC_NUM, ITERATIONS); % preallocation of data array
time_axis = zeros(1, ITERATIONS); % preallocation of time array

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM10';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

%% Readings Loop
pause(2);
tic;
for it = 1:ITERATIONS
    % Start the acquisition
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    pom = read_distance_data(arduino, MIC_NUM);
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
figure
for i = 1:24
    scatter(1:ITERATIONS, dist_matrix(i, :)); hold on;
end
%ylim([0 1])
%xlim([0 time_axis(end)])
grid on
xlabel("time [s]");
ylabel("distance [m]")
title("Microphone distance measurements")
beautify_plot(gcf, 1);
%% Plotting the data

figure
for i = 1:MIC_NUM
    subplot(MIC_NUM, 1, i);
    plot(dist_matrix(i, :), 'LineWidth', 2)
    %ylim([0 1])
    %xlim([0 time_axis(end)])
    grid on
    xlabel("time [s]");
    ylabel("distance [m]")
    title(mic_name(i));
end

beautify_plot(gcf, 1);


%%
% figure
% for i = 1:MIC_NUM
%     plot(dist_matrix(i, :), 'LineWidth', 2); hold on;
% end
% hold off;
% ylim([0 1])
% xlim([0 time_axis(end)])
% grid on
% xlabel("time [s]");
% ylabel("distance [m]")
% legend(mic_name);
% title("Microphone distance measurements")
% 
% beautify_plot(gcf, 1);

%%
figure; 
for i = 1:3:24
    scatter(1:ITERATIONS, dist_matrix(i, :)); hold on;
end
title("full data");
%dist_matrix_pom = dist_matrix(1:3:24, :);
dist_matrix_or=outlier_rejection(dist_matrix_pom);

figure; 
for i = 1:3
    scatter(1:ITERATIONS, dist_matrix_pom(i, :)); hold on;
end
title("max peak")

figure; 
for i = 1:3
    scatter(1:ITERATIONS, dist_matrix_or(i, :)); hold on;
end
title("after outlier rejection")