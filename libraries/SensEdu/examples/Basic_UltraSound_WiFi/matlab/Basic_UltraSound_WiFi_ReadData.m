%% Basic_UltraSound_WiFi_ReadData.m
%
% Same as Basic_UltraSound_ReadData, but connects to the Arduino TCP server over
% WiFi instead of the serial port.
%
% Set ARDUINO_IP to the address printed by the board; DATA_LENGTH must match
% the firmware.

clear;
close all;
clc;

%% Settings
ARDUINO_IP = "XXX.XXX.XXX.XXX"; % Must match the Arduino IP
ARDUINO_PORT = 80; % Must match the port of the Arduino server
ITERATIONS = 10000;

ACTIVATE_PLOTS = true;

DATA_LENGTH = 2000; % Must match the firmware

%% Arduino Setup
arduino_server = tcpclient(ARDUINO_IP, ARDUINO_PORT); % Connect to Arduino

%% Readings Loop
data = zeros(1,ITERATIONS);
time_axis = zeros(1,ITERATIONS);

for it = 1:ITERATIONS
    % Data readings
    write(arduino_server, 't', "char"); % Trigger arduino measurement
    time_axis(it) = toc;
    tic
    data = read_data(arduino_server, DATA_LENGTH);
    toc
    plot_data(data);
end

% Save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('Measurements/%s_%s.mat', "measurements", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "data", "time_axis");

% Calculate average time between measurements
buf = time_axis(2) - time_axis(1);
for i = 2:(length(time_axis) - 1)
    buf = mean([buf, (time_axis(i+1) - time_axis(i))]);
end
fprintf("Plots are activated: %s\n", mat2str(ACTIVATE_PLOTS));
fprintf("average time between measurements: %fsec\n", buf);

%% Functions
function data = read_data(arduino_server, data_length)
   total_byte_length = data_length * 2; % 2 bytes per sample
   serial_rx_data = read(arduino_server, total_byte_length, 'uint8');
   data = double(typecast(uint8(serial_rx_data), 'uint16'));
end

function plot_data(data)
    plot(data);
    ylim([0, 65535]);
    xlabel("Sample #");
    ylabel("ADC 16bit value");
    grid on;
end
