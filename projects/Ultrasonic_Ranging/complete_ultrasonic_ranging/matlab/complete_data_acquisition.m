%% complete_data_acquisition.m
% triggers ultrasonic recording
% receives the data
% plots distances along with processing steps
% handles multi-peak tracking and detailed/non-detailed data
clear;
close all;
addpath("plot scripts\");

%% Parameters
ITERATIONS = 150; 
MIC_NUM = 4; 
MAX_PEAKS = 3; % Match this value in Peaks.h
MIC_NAMES = {"MIC 1", "MIC 2","MIC 3", "MIC 4"};
DATA_LENGTH = 2048;
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

%% Prepare Figure
if ENABLE_LIVE_PLOTS == true
    figure("Position",[250, 250, 1500, 1000]);
end

%% Readings Loop
pause(1);
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
    if ENABLE_LIVE_PLOTS == true && ENABLE_DETAILED_DATA == true
        plot_live_data(reshape(processing_matrix(it,:,:,:), processing_matrix_size(2:end)), dist_matrix(:,:),MAX_PEAKS);
    end
    it
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

%% Plotting 1
mpt_plot_measurements(dist_matrix, MAX_PEAKS);

% %% Plotting 2
% figure
% for i = 1:MIC_NUM
%     subplot(MIC_NUM, 1, i);
%     plot(time_axis, dist_matrix(i, :), 'LineWidth', 2)
%     ylim([0 1])
%     xlim([0 time_axis(end)])
%     grid on
%     xlabel("time [s]");
%     ylabel("distance [m]")
%     title(MIC_NAMES(i));
% end
% beautify_plot(gcf, 1);

%% Functions
function plot_live_data(steps_matrix, distance_array,max_peaks)
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

% function dist_vector = read_distance_data(arduino, mic_num)
%     dist_vector = zeros(mic_num, 1);
%     for i = 1:mic_num
%         serial_rx_data = read(arduino, 4, 'uint8'); % 32bit per one distance measurement
%         dist_vector(i, 1) = double(typecast(uint8(serial_rx_data), 'uint32'))/1e6; % expected in micrometers
%     end
% end

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