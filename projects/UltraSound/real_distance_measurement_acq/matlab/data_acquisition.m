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
threshold_level = 0.2;
%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM16';
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
    
    outliers = [];
    if it > 1
        outlier_mask = abs(pom - dist_matrix(:, it-1)) > threshold_level;
        outliers = find(outlier_mask)
    end

    if isempty(outliers)
        dist_matrix(:, it) = pom;
    elseif numel(outliers) == MIC_NUM
        dist_matrix(:, it) = dist_matrix(:, it-1);
    else
        good_mics = setdiff(1:MIC_NUM, outliers);
        pom(outliers) = median(pom(good_mics));
        dist_matrix(:, it) = pom;
    end
    % cnt = 1;
    % for i = 1:length(pom)
    %     if (it > 1 && abs(pom(i, 1) - dist_matrix(i, it-1)) > threshold_level)
    %         outliers(cnt) = i;
    %         cnt = cnt+1;
    %     end
    % end
    % if cnt == 1
    %     % Reading the distance measurements
    %     dist_matrix(:, it) = pom;
    % elseif (cnt == MIC_NUM && it > 1)
    %     % set it to the previous value
    %     dist_matrix(:, it) = dist_matrix(:, it-1);
    % else 
    %     good_mic = setdiff(1:4, outliers)
    %     % set the value of faulty mic to the value of non faulty one
    %     for j=1:cnt-1
    %         pom(outliers(j), 1) = pom(good_mic(1), 1);
    %     end
    %     dist_matrix(:, it) = pom;
    % end
    % cnt
end
acquisition_time = toc;

% save measurements
file_name = sprintf('%s_%s.mat', "outlier_rejection", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %fsec\n", acquisition_time);

% Close serial connection
arduino = [];

%% Plotting the data
figure
for i = 1:MIC_NUM
    subplot(MIC_NUM, 1, i);
    plot(time_axis, dist_matrix(i, :))
    ylim([0 1])
    xlim([0 time_axis(end)])
    grid on
    xlabel("time [s]");
    ylabel("distance [m]")
    title("Real distance measurements: MIC");
end

