%% Basic_UltraSound_WiFi_ReadData.m
% reads config data and then ADC mics meassurements from Arduino using WiFi
clear;
close all;
clc;

%% Settings
ARDUINO_IP = "192.168.151.127"; % match to Arduino IP
ARDUINO_PORT = 80; % match to port of Arduino server
ITERATIONS = 10000;

ACTIVATE_PLOTS = true;

DATA_LENGTH = 16*128; % make sure to match this number with firmware

%% Arduino Setup
arduino_server = tcpclient(ARDUINO_IP, ARDUINO_PORT); % connect to Arduino

%% Readings Loop
data = zeros(1,ITERATIONS);
time_axis = zeros(1,ITERATIONS);

for it = 1:ITERATIONS
    % Data readings
    write(arduino_server, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    tic
    data = read_data(arduino_server, DATA_LENGTH);
    toc
    plot_data(data);
end

% save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('Measurements/%s_%s.mat', "measurements", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "data", "time_axis");

% calculate average time between measurements
buf = time_axis(2) - time_axis(1);
for i = 2:(length(time_axis) - 1)
    buf = mean([buf, (time_axis(i+1) - time_axis(i))]);
end
fprintf("Plots are activated: %s\n", mat2str(ACTIVATE_PLOTS));
fprintf("average time between measurements: %fsec\n", buf);

%% functions
function data = read_data(arduino_server, data_length)
    total_byte_length = data_length * 2; % 2 bytes per sample
    serial_rx_data = zeros(1, total_byte_length);

    for i = 1:(total_byte_length/32) % 32 byte chunk size
        serial_rx_data((32*i - 31):(32*i)) = read(arduino_server, 32, 'uint8');
    end
    
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end

function plot_data(data)
    plot(data);
    ylim([0, 65535]);
    xlabel("Sample #");
    ylabel("ADC 16bit value");
    grid on;
end
