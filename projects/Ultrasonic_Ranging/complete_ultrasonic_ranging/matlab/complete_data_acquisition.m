%% complete_data_acquisition.m
% triggers ultrasonic recording
% receives the data
% plots distances along with processing steps
% handles multi-peak tracking and detailed/non-detailed data
clear;
% close all;
addpath("plot scripts\");

%% Parameters
ITERATIONS = 750; 
MIC_NUM = 4; 
MAX_PEAKS = 2; % Match this value in Peaks.h
MIC_NAMES = {"MIC 1", "MIC 2","MIC 3", "MIC 4"};
DATA_LENGTH = 2048; % Match this value in main code
PROCESSING_STEPS = 3; % raw, fitlered, xcorr
ENABLE_DETAILED_DATA = false; % Match this value in the main code
ENABLE_LIVE_PLOTS = false; % Match this value in the main code

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

%% Arrays
dist_matrix = zeros(MIC_NUM*MAX_PEAKS, ITERATIONS); % distance matrix
processing_matrix = zeros(ITERATIONS, MIC_NUM, PROCESSING_STEPS, DATA_LENGTH); % all processing steps data
processing_matrix_size = size(processing_matrix);
time_axis = zeros(1, ITERATIONS); %  time array
y_vec = zeros(4,ITERATIONS);
%% Prepare Figure
if ENABLE_LIVE_PLOTS == true
    figure("Position",[250, 250, 1500, 1000]);
end

%% Prepare Distance Live Plot
num_rows = MIC_NUM * MAX_PEAKS;
markers = {'o', 's', 'd', '^', 'v', '>', '<', 'p', 'h', '+', '*', 'x'};
if num_rows == 4
    dist_legend = arrayfun(@(m) sprintf('MIC %d', m), 1:MIC_NUM, 'UniformOutput', false);
else
    dist_legend = cell(1, num_rows);
    for m = 1:MIC_NUM
        for p = 1:MAX_PEAKS
            dist_legend{(m-1)*MAX_PEAKS + p} = sprintf('MIC %d_%d', m, p);
        end
    end
end
fig_dist = figure('Name', 'Distance Measurements');
dist_handles = gobjects(1, num_rows);
for r = 1:num_rows
    dist_handles(r) = plot(NaN, NaN, markers{mod(r-1, numel(markers))+1}, 'LineStyle', 'none');
    hold on;
end
title('Distance Measurements');
xlabel('Iteration');
ylabel('Distance [m]');
legend(dist_legend, 'Location', 'best');
grid on;

%% Readings Loop
pause(3);
tic;
for it = 1:ITERATIONS
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    if ENABLE_DETAILED_DATA
        for i = 1:MIC_NUM
            processing_matrix(it, i, 1, :) = read_16bit_data(arduino, DATA_LENGTH);
            processing_matrix(it, i, 2, :) = read_float_data(arduino, DATA_LENGTH);
            processing_matrix(it, i, 3, :) = read_float_data(arduino, DATA_LENGTH);
        end
    end
    pom = mpt_read_distance_data(arduino, MIC_NUM, MAX_PEAKS);
    % Reading the distance measurements
    dist_matrix(:, it) = pom;
    % dist_matrix(:, it) = read_distance_data(arduino, MIC_NUM);
    % Update live distance plot
    for r = 1:num_rows
        set(dist_handles(r), 'XData', 1:it, 'YData', dist_matrix(r, 1:it));
    end
    drawnow limitrate;
    if ENABLE_LIVE_PLOTS == true && ENABLE_DETAILED_DATA == true
        plot_live_data(reshape(processing_matrix(it,:,:,:), processing_matrix_size(2:end)), dist_matrix(:,:),MAX_PEAKS);
    end
    it

    
    % Use this code to test the best peak selection algorithm like in the
    % EKF codes
    if it == 1
        y = [dist_matrix(1:MAX_PEAKS:MIC_NUM*MAX_PEAKS,it)]; % initially take the 1st peak
        prev_best = y; % it's the best for now
    else              
        thr_peaks = 0.08; % we assume the target will not move more than this value between steps
        for m = 1:MIC_NUM
            for j = 1:MAX_PEAKS
                % We want to check which among the peaks is the best one,
                % i.e., the one closer to the previous estimate. This will
                % be sent to the filter as measurement (y).>
               if (abs(dist_matrix(MAX_PEAKS*(m-1)+j,it) - y_vec(m,it-1)) <= thr_peaks)
                   y(m) = dist_matrix(MAX_PEAKS*(m-1)+j,it);
                   prev_best(m) = y(m);
                   break; % already done for the mic m
               else
                    y(m) = prev_best(1);
               end
            end
        end
    end    
    y_vec(:, it) = y;





end
acquisition_time = toc;

% save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('%s_%s.mat', "Measurements/dataset", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %fsec\n", acquisition_time);

% close serial connection
arduino = [];

% %% Plotting distances 1
% mpt_plot_measurements(dist_matrix, MAX_PEAKS);
% 
% %% Plotting distances 2
% first_peak = dist_matrix(1,:);
% second_peak = dist_matrix(2,:);
% third_peak = dist_matrix(3,:);
% used_D = y_vec(1,:);
% figure,
% plot(first_peak, 'ro','LineStyle','none'); hold on;
% plot(second_peak, 'bo','LineStyle','none'); hold on;
% plot(third_peak, 'go','LineStyle','none'); hold on;
% plot(used_D, 'k-','LineWidth',1.5); hold on;

%% Functions
function plot_live_data(steps_matrix, distance_array, max_peaks)
    [mic_num, processing_steps, data_length] = size(steps_matrix);
    x_plots_num = processing_steps + 1;
    y_plots_num = mic_num;
    plot_idx = 1;
    for j = 1:mic_num
        for i = 1:processing_steps
            subplot(y_plots_num, x_plots_num, plot_idx);
            plot_detailed_data(j, i, squeeze(steps_matrix(j, i, :)));
            plot_idx = plot_idx + 1;
        end
        subplot(y_plots_num, x_plots_num, plot_idx);
        plot_distance_data(j, squeeze(distance_array(max_peaks*j-(max_peaks-1), :)))
        plot_idx = plot_idx + 1;
    end
    % beautify_plot(gcf, 1);
end

function plot_distance_data(mic, data)
    plot(data);
    title("MIC #" + string(mic) + ": Estimated Distance");
    ylabel("Distance [m]");
    xlabel("Iteration");
    ylim([0, 2.5]);
    grid on;
end

function plot_detailed_data(mic, step, data)
    switch step
        case 1 % Raw
            plot(data);
            title("MIC #" + string(mic) + ": Raw ADC Data");
            ylabel("ADC Value");
            ylim([0, 65535]);
            xlim([1, length(data)]);
        case 2 % Filtered
            plot(data);
            title("MIC #" + string(mic) + ": Filtered Data w/o Coupling");
            ylim([-8, 8]);
            xlim([1, length(data)]);
        case 3 % XCorr
            plot(data)
            title("MIC #" + string(mic) + ": Cross-Correlation Result");
            xlim([1, length(data)]);
    end
end

function data = read_16bit_data(arduino, data_length)
    chunk_size = 32; % in bytes
    data_length_byte = data_length*2; % multiplied by sizeof(type)

    raw_data_8bit = zeros(data_length_byte/chunk_size, chunk_size);
    raw_data_16bit = zeros(data_length_byte/chunk_size, chunk_size/2);
    
    for i = 1:(data_length_byte/chunk_size)
        raw_data_8bit(i, :) = read(arduino, chunk_size, 'uint8');
        raw_data_16bit(i, :) = typecast_uint8_uint16(raw_data_8bit(i, :));
    end
    
    % rearrange by mic
    data = reshape(raw_data_16bit', 1, []);
end

function data = read_float_data(arduino, data_length)
    chunk_size = 32; % in bytes
    data_length_byte = data_length*4; % multiplied by sizeof(type)

    raw_data_8bit = zeros(data_length_byte/chunk_size, chunk_size);
    raw_data_float = zeros(data_length_byte/chunk_size, chunk_size/4);
    
    for i = 1:(data_length_byte/chunk_size)
        raw_data_8bit(i, :) = read(arduino, chunk_size, 'uint8');
        raw_data_float(i, :) = typecast_uint8_float(raw_data_8bit(i, :));
    end
    
    % rearrange by mic
    data = reshape(raw_data_float', 1, []);
end

function casted_data = typecast_uint8_uint16(data)
    reshaped_data = reshape(data, 2, []);
    casted_data = bitshift(uint16(reshaped_data(2, :)), 8) + uint16(reshaped_data(1, :));
end

function casted_data = typecast_uint8_float(data)
    reshaped_data = reshape(data, 4, []);
    casted_data = uint32(reshaped_data(1, :));
    for i = 2:4
        casted_data = casted_data + bitshift(uint32(reshaped_data(i, :)), 8*(i-1));
    end
    casted_data = typecast(casted_data, 'single');
end