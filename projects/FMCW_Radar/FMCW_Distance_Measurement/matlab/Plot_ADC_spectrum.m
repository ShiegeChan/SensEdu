%% Plot_ADC_FFT.m (Optimized for Real-Time Plotting and PSD)
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 10000;           % Number of real-time ADC measurements
SAMPLING_RATE = 250000;       % ADC Sampling rate
ACTIVATE_PLOTS = true;        % Toggle plotting on/off

% Connect to Arduino
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Initialize Arduino serial communication

%% Real-Time ADC Data Acquisition and PSD
fprintf("Starting real-time ADC data acquisition with power spectrum...\n");

% Initialize Figure for Real-Time Raw Data Plot in Full Screen
figure('Name', 'Real-Time ADC Signal and power spectrum', 'Color', 'k', 'WindowState', 'maximized');

subplot(2, 2, 1); % Top left subplot for ADC Signal
real_time_plot = plot(nan(1, 1), 'r')
xlim([0, 2048]);
ylim([0, 65535]);        % For 16-bit ADC values
xlabel("Sample #");
ylabel("Amplitude");
title("Real-Time ADC Microphone Data");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

subplot(2, 2, 2); % Top right plot for HP filtered ADC signal
real_time_plot_filt = plot(nan(1, 1), 'r')
xlim([0, 2048]);
ylim([-32818, 32818]);        % For 16-bit ADC values
xlabel("Sample #");
ylabel("Amplitude");
title("Real-Time ADC Microphone Data filtered");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';


subplot(2,2,[3,4]); % Bottom subplot for PSD
PSD_plot = plot(nan(1, 1), 'r')
xticks(0:5000:250000); % Set tick positions at 1 kHz intervals (adjust as needed)
xlabel("Frequency (Hz)"); % Update the X-axis label to indicate Hz
ylabel("Power (dB)");
title("Real-Time power spectrum");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

for it = 1:ITERATIONS
    % 1. Trigger ADC data acquisition
    write(arduino, 't', "char");
    
    % 2. Retrieve ADC data length dynamically from the header
    adc_byte_length = read_total_length(arduino);        % Read header
    ADC_DATA_LENGTH = adc_byte_length / 2;               % Number of samples = number of bytes / 2 (16-bit samples)
    
    % 3. Retrieve ADC data
    adc_data = read_data(arduino, ADC_DATA_LENGTH);      % Retrieve ADC data

    adc_data_filt = highpass(adc_data,30000,SAMPLING_RATE);

    % 4. Compute PSD
    %[p,f]=periodogram(adc_data, rectwin(ADC_DATA_LENGTH), [], SAMPLING_RATE);
    [p,f]=pspectrum(adc_data_filt, SAMPLING_RATE);


    % 5. Update plots in real time
    if ACTIVATE_PLOTS
        % Update raw data plot
        set(real_time_plot, 'YData', adc_data); % Update Y-axis data for raw signal

        set(real_time_plot_filt, 'YData', adc_data_filt); % Update Y-axis data for raw signal

        set(PSD_plot,'XData', f);
        set(PSD_plot,'YData', p);   

        % Refresh the plots
        drawnow limitrate;
    end
end

%% Close Serial Port
clear arduino;

fprintf("ADC acquisition and plotting completed.\n");

%% Supporting Functions
function total_byte_length = read_total_length(arduino)
    % Reads the 4-byte total length (in bytes) of incoming data from Arduino
    len_bytes = read(arduino, 4, 'uint8'); % Read header
    total_byte_length = typecast(uint8(len_bytes), 'uint32'); % Convert to uint32
end

function data = read_data(arduino, data_length)
    % Retrieve `data_length` samples from Arduino
    total_byte_length = data_length * 2;      % Convert samples to bytes (16-bit values)
    serial_data = zeros(1, total_byte_length, 'uint8'); % Preallocate array
    
    % Read data in chunks of 32 bytes (matches Arduino chunking)
    for i = 1:(total_byte_length / 32)
        serial_data((32 * i - 31):(32 * i)) = read(arduino, 32, 'uint8');
    end

    % Convert received bytes to uint16 samples
    data = double(typecast(serial_data, 'uint16')); % Cast from raw bytes to 16-bit integers
end
