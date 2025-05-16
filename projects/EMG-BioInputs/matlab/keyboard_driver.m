%% TODO:
% 1. error detection

%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM10';
ARDUINO_BAUDRATE = 115200;

QUIT_FLAG = false;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate

%% Configuration Readings
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8_data(read(arduino, 2, 'uint8'));
channel_count = typecast_uint8_data(read(arduino, 1, 'uint8'));

data_length = mem_size/channel_count;

%% Data Readings Loop
while(~QUIT_FLAG)
    write(arduino, 'm', "char"); % measurement

    data = zeros(channel_count, data_length);
    for j = 1:channel_count
        for i = 1:data_length
            data(j, i) = typecast_uint8_data(read(arduino, 2, 'uint8'));
        end
    end

    plot_data(data);
end

% set COM port back free
arduino = [];

%% functions
function casted_data = typecast_uint8_data(data)
    
    % recalculated data by byte position
    byte_length = length(data);
    shifts = 0:(byte_length - 1);
    shifted_data = data .* 2.^(shifts * 8);
    
    % sum separate bytes to calculate single number
    casted_data = sum(shifted_data);
end

function plot_data(data)
    subplot(2,2,1);
    plot(data(1,:));

    subplot(2,2,2);
    plot(data(2,:));

    subplot(2,2,3);
    plot(data(3,:));

    subplot(2,2,4);
    plot(data(4,:));
end