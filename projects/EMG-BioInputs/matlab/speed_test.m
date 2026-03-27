%% keyboard_driver.m
clear;
%close all;
clc;

%% Include
addpath(genpath('./processing/'));
addpath(genpath('./keys/'));
addpath(genpath('./plotting/'));

%% Settings
% Arduino
ARDUINO_PORT = 'COM16';
ARDUINO_BAUDRATE = 115200;
 
%% Arduino Connection
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);

%% Configuration
write(arduino, 'c', "char"); % config
mem_size = typecast_uint8(read(arduino, 2, 'uint8'));

channel_n = typecast_uint8(read(arduino, 2, 'uint8'));
fs = double(typecast_uint8(read(arduino, 2, 'uint8')));

data_length = double(mem_size/channel_n);
meas_duration_ms = 1000*(data_length/fs);

%% Main Loop
while(true)
    % Trigger
    write(arduino, 'm', "char"); % measurement

    % Measurements
    new_meas = read_data(arduino, channel_n, data_length);
    toc;
    tic;
end

% set COM port back free
arduino = [];

%% functions
function data_by_channel = read_data(arduino, channel_n, data_length)
    chunk_size = 64; % in bytes
    total_length = channel_n*data_length*2; % in bytes

    raw_data_8bit = zeros(total_length/chunk_size, chunk_size);
    raw_data_16bit = zeros(total_length/chunk_size, chunk_size/2);
    
    % readings
    for i = 1:(total_length/chunk_size)
        raw_data_8bit(i, :) = read(arduino, chunk_size, 'uint8');
        raw_data_16bit(i, :) = typecast_uint8(raw_data_8bit(i, :));
    end
    

    % rearrange by channel
    combined_raw_data = reshape(raw_data_16bit', 1, []);
    data_by_channel = reshape(combined_raw_data, channel_n, []);
end

function casted_data = typecast_uint8(data)
    reshaped_data = reshape(data, 2, []);
    casted_data = bitshift(uint16(reshaped_data(2, :)), 8) + uint16(reshaped_data(1, :));
end

function check_firmware_error(data, channel_n)
    % error code is 0x0000FFFF
    % LSB first, so expected 0xFFFF, then 0x0000
    % due to channel data rearrangement if channel_n > 1, second part
    % expected to be in different column

    error_flag = 0;
    if data(1,1) == 0xFFFF
        error_flag = 1;
    end
    
    if error_flag == 1
        if channel_n == 1
            if data(1,2) ~= 0x0000
                error_flag = 0;
            end
        else
            if data(2,1) ~= 0x0000
                error_flag = 0;
            end
        end
    end

    if error_flag == 1
        error(['Internal Firmware Error. Code: %s. Reset the board before proceeding.\n' ...
            'https://shiegechan.github.io/SensEdu/Library/ADC/#errors\nLook for corresponding error code in the wiki.'], string(dec2hex(data(1,3))));
    end
end

function prepare_save_folders(foldername, setname, is_overwrite)
    if ~isfolder(foldername)
        mkdir(foldername);
    end

    subfolder_path = sprintf("%s\\%s", foldername, setname);
    if ~isfolder(subfolder_path)
        mkdir(subfolder_path);
    elseif is_overwrite == true
        rmdir(subfolder_path, 's');
        mkdir(subfolder_path);
    else
        error("Measurement Set with this name already exists.\nChange SETNAME or put IS_OVERWRITE to 'true'", subfolder_path);
    end
end

function save_data(foldername, setname, buffer_num, raw_data, all_history_max, last_1min_max_values, last_1sec_max_values)
    subfolder_path = sprintf("%s\\%s", foldername, setname);
    full_filename = sprintf("%s\\%d_%s.mat", subfolder_path, buffer_num, datetime("now"));
    full_filename = strrep(full_filename, ' ', '_');
    full_filename = strrep(full_filename, ':', '-');
    save(full_filename, "raw_data", "all_history_max", "last_1min_max_values", "last_1sec_max_values");
end
