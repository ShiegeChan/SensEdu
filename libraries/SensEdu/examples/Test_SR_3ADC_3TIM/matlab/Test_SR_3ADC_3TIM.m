% Reads config data and ADC measurements from Arduino
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 10000;

ACTIVATE_PLOTS = true;

DATA_LENGTH = 16 * 128; % Ensure this matches with firmware
SINE_WAVE_FREQ = 1000; % Frequency of the input sine wave in Hz

% Sampling rates for ADCs
ADC1_SAMPLING_RATE = 250000; % 250 kHz
ADC2_SAMPLING_RATE = 100000; % 100 kHz
ADC3_SAMPLING_RATE = 65000; % 65 kHz

%% Arduino Setup
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Select port and baud rate

%% Readings Loop
for it = 1:ITERATIONS
    % Trigger ADC sampling on Arduino
    write(arduino, 't', "char"); % Trigger Arduino via 't' character

    % Read ADC data
    fprintf("Iteration: %d\n", it);
    data_adc1 = read_data(arduino, DATA_LENGTH, "ADC1");
    data_adc2 = read_data(arduino, DATA_LENGTH, "ADC2");
    data_adc3 = read_data(arduino, DATA_LENGTH, "ADC3");

    % Plot data showing differences in sample rates and samples per cycle
    if ACTIVATE_PLOTS
        plot_adc_with_samples_per_cycle(data_adc1, data_adc2, data_adc3, ...
            ADC1_SAMPLING_RATE, ADC2_SAMPLING_RATE, ADC3_SAMPLING_RATE, ...
            SINE_WAVE_FREQ);
    end
end

% Release the serial port
arduino = [];

%% Function to Read ADC Data
function data_adc = read_data(arduino, data_length, adc_name)
    % Function to read `data_length` samples for a specific ADC from Arduino

    fprintf("Waiting for %s data...\n", adc_name);

    % Total data size in bytes (assuming uint16 = 2 bytes)
    total_byte_length = data_length * 2;
    serial_rx_data = zeros(1, total_byte_length, 'uint8'); 

    try
        % Read data in chunks of 32 bytes
        for i = 1:(total_byte_length / 32)
            chunk = read(arduino, 32, 'uint8'); % Receive 32 bytes
            if length(chunk) ~= 32
                error("%s: Received incomplete chunk. Expected 32 bytes, got %d bytes.\n", ...
                    adc_name, length(chunk));
            end
            serial_rx_data((32 * i - 31):(32 * i)) = chunk; % Append to buffer
        end

        % Convert received uint8 data into uint16 values
        data_adc = double(typecast(serial_rx_data, 'uint16'));
        fprintf("%s data received successfully: %d samples.\n", adc_name, length(data_adc));
    catch ME
        warning("%s: Error reading data. %s", adc_name, ME.message);
        data_adc = zeros(1, data_length); % Return zero-filled array to avoid crashing
    end
end

%% Function to Plot ADC Data with Samples per Cycle
function plot_adc_with_samples_per_cycle(data_adc1, data_adc2, data_adc3, ...
        sr_adc1, sr_adc2, sr_adc3, sine_wave_freq)
    % Calculate the number of samples per cycle for each ADC
    samples_per_cycle_adc1 = sr_adc1 / sine_wave_freq;
    samples_per_cycle_adc2 = sr_adc2 / sine_wave_freq;
    samples_per_cycle_adc3 = sr_adc3 / sine_wave_freq;

    % Create time vectors for each ADC based on their sample counts and rates
    time_adc1 = (0:length(data_adc1)-1) * (1 / sr_adc1); % Time for ADC1
    time_adc2 = (0:length(data_adc2)-1) * (1 / sr_adc2); % Time for ADC2
    time_adc3 = (0:length(data_adc3)-1) * (1 / sr_adc3); % Time for ADC3

    % Display samples per cycle in the MATLAB console
    fprintf("Samples per Cycle:\n");
    fprintf("  ADC1 (%.0f kHz): %.2f samples/cycle\n", sr_adc1 / 1e3, samples_per_cycle_adc1);
    fprintf("  ADC2 (%.0f kHz): %.2f samples/cycle\n", sr_adc2 / 1e3, samples_per_cycle_adc2);
    fprintf("  ADC3 (%.0f kHz): %.2f samples/cycle\n", sr_adc3 / 1e3, samples_per_cycle_adc3);

    % Create a figure with three subplots for ADCs and one for superimposed data
    figure(1);
    clf; % Clear the figure for consecutive iterations

    % Plot ADC1 data: use time_adc1 on x-axis
    subplot(4, 1, 1);
    plot(time_adc1, data_adc1, 'r');
    xlabel('Time (s)');
    ylabel('ADC1 Value');
    title(sprintf('ADC1 (%.0f kHz) - %.2f Samples per Cycle', sr_adc1 / 1e3, samples_per_cycle_adc1));
    grid on;

    % Plot ADC2 data: use time_adc2 on x-axis
    subplot(4, 1, 2);
    plot(time_adc2, data_adc2, 'g');
    xlabel('Time (s)');
    ylabel('ADC2 Value');
    title(sprintf('ADC2 (%.0f kHz) - %.2f Samples per Cycle', sr_adc2 / 1e3, samples_per_cycle_adc2));
    grid on;

    % Plot ADC3 data: use time_adc3 on x-axis
    subplot(4, 1, 3);
    plot(time_adc3, data_adc3, 'b');
    xlabel('Time (s)');
    ylabel('ADC3 Value');
    title(sprintf('ADC3 (%.0f kHz) - %.2f Samples per Cycle', sr_adc3 / 1e3, samples_per_cycle_adc3));
    grid on;

    % Create a combined superimposed plot to compare sample rates
    subplot(4, 1, 4);
    hold on;
    plot(time_adc1, data_adc1, '-o', 'DisplayName', sprintf('ADC1 (%.0f kHz)', sr_adc1 / 1e3), 'Color', 'r');
    plot(time_adc2, data_adc2, '-x', 'DisplayName', sprintf('ADC2 (%.0f kHz)', sr_adc2 / 1e3), 'Color', 'g');
    plot(time_adc3, data_adc3, '-d', 'DisplayName', sprintf('ADC3 (%.0f kHz)', sr_adc3 / 1e3), 'Color', 'b');
    hold off;
    xlabel('Time (s)');
    ylabel('ADC Value');
    title('Superimposed ADC Signals');
    legend('Location', 'best');
    grid on;

    % Pause to allow visualization in real time
    pause(0.1);
end