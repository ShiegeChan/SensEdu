%% main.m
% reads config data and then ADC mics meassurements from Arduino
clear;
close all;

%% Data Acquisition parameters
ITERATIONS = 500; 
MIC_NUM = 6;
mic_name = {"MIC 1", "MIC 2", "MIC 4", "MIC 8", "MIC 6", "MIC 7"};
DATA_LENGTH = 64 * 32;
dist_matrix = zeros(MIC_NUM, ITERATIONS); % preallocation of data array
time_axis = zeros(1, ITERATIONS); % preallocation of time array

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM4';
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
file_name = sprintf('%s_%s.mat', "xxlboard", datetime("now"));
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

%% MIC 1 double distance
figure;
mic1_d = dist_matrix(1,:);
findchangepts(mic1_d,MaxNumChanges=3,Statistic='rms')

%%
figure
for i = 1:MIC_NUM
    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2); hold on;
end

hold on;
%%
% Define the region to shade (8 to 10 seconds)
x1 = 8;
x2 = 14;
ylimits = ylim; % Get current y-axis limits

% Create patch coordinates
x_patch = [x1, x2, x2, x1];
y_patch = [ylimits(1), ylimits(1), ylimits(2), ylimits(2)];

% Add the shaded region (red with 20% transparency)
patch(x_patch, y_patch, 'red', 'FaceAlpha', 0.2, 'EdgeColor', 'none');

hold off;
hold on;

% Define the region to shade (8 to 10 seconds)
x3 = 17;
x4 = 23;

% Create patch coordinates
x_patch = [x3, x4, x4, x3];
y_patch = [ylimits(1), ylimits(1), ylimits(2), ylimits(2)];

% Add the shaded region (red with 20% transparency)
patch(x_patch, y_patch, 'blue', 'FaceAlpha', 0.2, 'EdgeColor', 'none');

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