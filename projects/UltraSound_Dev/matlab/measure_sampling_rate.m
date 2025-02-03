
%% measure_sampling_rate
% RMS noise of the microphones

%% How to perform the measurement:
% 1. Take off the ultrasound shield
% 2. Connect wave gen to ADC of the board
% 3. Set proper settings for wave gen: 1V amplitude and 1V offset not to saturate the ADC.
% 3. Choose ITERATION_NUM
% 4. Boot this script

%%
clear;
close all;
%clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;

ITERATION_NUM = 50;
FREQUENCY = 1000; % 1kHz

MEASUREED_MIC = 1;

FILE_NAME = 'Sampling_Rate';

SAMPLING_FREQUENCY = 353e3; % recalculate if you change firmware! [S/sec]
                            % and change the filterDesigner coeffs

AIR_SOUND_SPEED = 343; % [m/sec]

PLOT_RAW_INPUT_DATA = true;

%% Arduino Setup + Config
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate

write(arduino, 'c', "char"); % trigger arduino config
serial_rx_data = read(arduino, 4, 'uint8'); % 4 bytes of data length

data_length = serial_rx_data(1)*2^24 + serial_rx_data(2)*2^16 + serial_rx_data(3)*2^8 + serial_rx_data(4);

%% Readings Loop

mic_periods = zeros(1, ITERATION_NUM);

for i = 1:ITERATION_NUM
    % progress bar
    if mod(i,10) == 0
        fprintf('Iteration #%d/%d\n', i, ITERATION_NUM);
    end

    % Data readings
    write(arduino, 'r', "char"); % trigger arduino measurement
    [mic1_data, mic2_data, mic3_data] = read_16bit_serial_measurements(arduino, data_length);
    mic_matrix = [mic1_data, mic2_data, mic3_data];
    
    data = mic_matrix(:, MEASUREED_MIC);

    % Data processing
    data = adc2voltage(data);
    
    % plot
    if PLOT_RAW_INPUT_DATA
        plot_raw_input_data(data);
    end

    % periods
    mic_periods(i) = calculate_mean_period(data);
    %mic_periods(i)
end

% combine all iteration data
mic_sampling_rate = mean(mic_periods)*FREQUENCY/1000;

% print and save results
fprintf('Sampling rate for mic #%i: %fkS/sec\n', MEASUREED_MIC, mic_sampling_rate);

file_name = sprintf('%s_%s.mat', FILE_NAME, datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "mic_sampling_rate");

% set COM port back free
arduino = [];

%% functions
function plot_raw_input_data(data)
    plot(data);
    title('Constant sine wave to ADC')
    grid on;
end

function mean_period_samples = calculate_mean_period(data)

    % Find zero-crossings
    zero_crossings = find(diff(sign(data)) ~= 0);
    full_period_indices = zero_crossings(1:2:end);
    
    % Ensure there are enough zero-crossings to calculate at least one full period
    if length(full_period_indices) < 2
        error("Not enough samples for one period of a sine wave to determine the sample rate")
    end

    % Calculate the periods between zero-crossings in samples
    periods = diff(full_period_indices);

    % Calculate the mean period in samples
    mean_period_samples = mean(periods);
end

function voltage_data = adc2voltage(adc_data)
    % 0:65535 -> 0:3.3V
    voltage_data = (adc_data ./ 65536) .* 3.3; % 16bit mode

    % 0:3.3V -> -1.65V:1.65V
    voltage_data = voltage_data - 1.65;

    % DC removal
    voltage_data = voltage_data - mean(voltage_data);
end
