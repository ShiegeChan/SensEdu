%% ADC_1CH_DMA_Circular.m
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM9';
ARDUINO_BAUDRATE = 2000000;
ITERATIONS = 200;

BUF_SIZE = 64;
HALF_BUF_SIZE = BUF_SIZE/2;

CHUNK_SIZE = 64; % Bytes per USB request

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);
flush(arduino);

%% Readings Loop
data = zeros(HALF_BUF_SIZE, ITERATIONS);
time_axis = zeros(1, ITERATIONS);

% Trigger the measurement
write(arduino, 't', "char");

for it = 1:ITERATIONS
    data(:,it) = read_data(arduino, HALF_BUF_SIZE, CHUNK_SIZE);
end

plot_dataset(data);
plot_boundaries(data, HALF_BUF_SIZE, ITERATIONS);

% set COM port back free
arduino = [];

%% Functions
function data = read_data(arduino, buf_size, chunk_size)
    % 2 bytes per sample
    total_byte_length = buf_size * 2;
    serial_rx_data = zeros(1, total_byte_length, 'uint8');
    bytes_read = 0;
    while bytes_read < total_byte_length 
        transfer_size = min(chunk_size, total_byte_length - bytes_read);
        serial_rx_data(bytes_read + 1 : bytes_read + transfer_size) = read(arduino, transfer_size, 'uint8');
        bytes_read = bytes_read + transfer_size;
    end
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end

function plot_dataset(data)
    OneDArray = reshape(data, 1, []);
    plot(OneDArray)
    ylabel("ADC 16bit value");
    grid on;
    ylim([0, 65535]);
end

function plot_boundaries(buf, buf_size, buf_num)
    hold on;
    package_idxs = [buf_size, buf_size+1];
    for it = 2:buf_num
        package_idxs = [package_idxs, buf_size*it, ((buf_size*it)+1)];
    end
    package_idxs = package_idxs(1:(end-1));
    scatter(package_idxs, buf(package_idxs));
end