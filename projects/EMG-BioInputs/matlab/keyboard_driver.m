%% TODO:
% 1. error detection

%% keyboard_driver.m
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM12';
ARDUINO_BAUDRATE = 115200;

ITERATION = 500;

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate

%% Configuration Readings
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8_data(read(arduino, 2, 'uint8'));
channel_count = typecast_uint8_data(read(arduino, 1, 'uint8'));

data_length = mem_size/channel_count;

%% Data Readings Loop
data = zeros(channel_count, data_length, ITERATION);
for it = 1:ITERATION
    write(arduino, 'm', "char"); % measurement

    for j = 1:channel_count
        for i = 1:data_length
            data(j, i, it) = typecast_uint8_data(read(arduino, 2, 'uint8'));
        end
    end

    plot_whole_dataset(data(:,:,it));
end

save_data(data)

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

function plot_whole_dataset(data)
    subplot(2,2,1);
    plot_one_electrode(data(1,:));

    subplot(2,2,2);
    plot_one_electrode(data(2,:));

    subplot(2,2,3);
    plot_one_electrode(data(3,:));
    
    subplot(2,2,4);
    plot_one_electrode(data(4,:));
end

function plot_one_electrode(data)
    plot(data);
    xlim([-200, 1200]);
    %ylim([0, 65535]);
    xlabel("sample")
    ylabel("ADC value")
end

function save_data(data_collection)
    % save measurements
    file_name = sprintf('%s_%s.mat', "muscles_set", datetime("now"));
    file_name = strrep(file_name, ' ', '_');
    file_name = strrep(file_name, ':', '-');

    save(file_name, "data_collection");
end