%% demo file to see what needs to be implemented via interface
%% see actual code at matlab -> apps -> design app -> open -> WiFi_GUI.mlapp
clear;
close all;
addpath("plot scripts\");

%% Parameters
ITERATIONS = 25; 
MIC_NUM = 1;
MIC_NAMES = {"MIC 1"};
DATA_LENGTH = 32 * 32;
PROCESSING_STEPS = 3; % raw, fitlered, xcorr
ENABLE_LIVE_PLOTS = true;

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_IP = "192.168.56.128"; % match to Arduino IP
ARDUINO_PORT = 80; % match to port of Arduino server

arduino = tcpclient(ARDUINO_IP, ARDUINO_PORT, "Timeout",30, "ConnectTimeout",30); % connect to Arduino

%% Arrays
dist_matrix = zeros(MIC_NUM, ITERATIONS); % distance matrix
processing_matrix = zeros(ITERATIONS, MIC_NUM, PROCESSING_STEPS, DATA_LENGTH); % all processing steps data
processing_matrix_size = size(processing_matrix);
time_axis = zeros(1, ITERATIONS); %  time array

%% Prepare Figure
if ENABLE_LIVE_PLOTS == true
    figure("Position",[500, 500, 1250, 300]);
end

%% Readings Loop
pause(1);
tic;
for it = 1:ITERATIONS
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;

    for i = 1:MIC_NUM
        processing_matrix(it, i, 1, :) = read_16bit_data(arduino, DATA_LENGTH);
        processing_matrix(it, i, 2, :) = read_float_data(arduino, DATA_LENGTH);
        processing_matrix(it, i, 3, :) = read_float_data(arduino, DATA_LENGTH);
    end
    dist_matrix(:, it) = read_distance_data(arduino, MIC_NUM);

    if ENABLE_LIVE_PLOTS == true
        plot_live_data(reshape(processing_matrix(it,:,:,:), processing_matrix_size(2:end)), dist_matrix(:,:));
    end
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
%arduino = [];

%% Plotting 1
figure
for i = 1:MIC_NUM
    switch i
        case 1
            m = "o";
        case 2
            m = "^";
        case 3
            m = "square";
        case 4
            m = "diamond";
        case 5
            m = "v";
        case 6
            m = "hexagram";
        case 7
            m = "pentagram";
        case 8
            m = ">";
    end

    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2, 'Marker', m); hold on;

end
ylim([0 1])
xlim([0 time_axis(end)])
grid on
xlabel("time [s]");
ylabel("distance [m]")
legend(MIC_NAMES);
title("Microphone distance measurements")
beautify_plot(gcf, 1);

%% Functions
function plot_live_data(steps_matrix, distance_array)
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
        plot_distance_data(j, squeeze(distance_array(j, :)))
        plot_idx = plot_idx + 1;
    end
    % beautify_plot(gcf, 1);
end

function plot_distance_data(mic, data)
    plot(data);
    title("MIC #" + string(mic) + ": Estimated Distance");
    ylabel("Distance [m]");
    xlabel("Iteration");
    ylim([0, 1]);
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

function dist_vector = read_distance_data(arduino, mic_num)
    dist_vector = zeros(mic_num, 1);
    for i = 1:mic_num
        serial_rx_data = read(arduino, 4, 'uint8'); % 32bit per one distance measurement
        dist_vector(i, 1) = double(typecast(uint8(serial_rx_data), 'uint32'))/1e6; % expected in micrometers
    end
end

function data = read_16bit_data(arduino, data_length)
    raw_data = read(arduino, data_length * 2, 'uint8');
    data = typecast_uint8_uint16(raw_data);
end

function data = read_float_data(arduino, data_length)
    raw_data = read(arduino, data_length * 4, 'uint8');
    data = typecast_uint8_float(raw_data);
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