%% Plot_Tx_Rx.m Plot Mic ADC and DAC to ADC Data with power spectrum
%% ADC1 = DAC (to ADC) data
%% ADC2 = MIC DATA
clear;
close all;
clc;

%% Settings
ARDUINO_PORT = 'COM4';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 10000;        % Number of real-time ADC measurements
SAMPLING_RATE = 250000;    % ADC Sampling rate
ACTIVATE_PLOTS = true;     % Toggle plotting on/off

% Connect to Arduino
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Initialize Arduino serial communication

%% Real-Time ADC Data Acquisition and PSD
fprintf("Starting real-time dual ADC data acquisition...\n");

% Initialize Figure for Real-Time Data Plots in Full Screen
figure('Name', 'Real-Time ADC Signals and Power Spectra', 'Color', 'k', 'WindowState', 'maximized');

% Subplot for Mic Signal (Top Left)
subplot(3, 2, 1); 
Mic_ADC_plot = plot(nan(1, 1), 'r');
xlim([0, 2048]);
ylim([0, 65535]); % For 16-bit ADC values
xlabel("Sample #");
ylabel("Amplitude");
title("Mic ADC");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

% Subplot for Mic High-Passed Signal (Top Right)
subplot(3, 2, 2); 
adc2_filt_plot = plot(nan(1, 1), 'r');
xlim([0, 2048]);
ylim([-32818, 32818]); % High-pass filtered values
xlabel("Sample #");
ylabel("Amplitude");
title("High-Passed Mic Data");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

% Subplot for DAC to ADC Signal (Middle Left)
subplot(3, 2, 3); 
DAC_ADC_plot = plot(nan(1, 1), 'g');
xlim([0, 2048]);
ylim([0, 65535]); % For 16-bit ADC values
xlabel("Sample #");
ylabel("Amplitude");
title("DAC ADC");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

% Subplot for DAC to ADC High-Passed Signal (Middle Right)
subplot(3, 2, 4); 
adc1_filt_plot = plot(nan(1, 1), 'g');
xlim([0, 2048]);
ylim([-32818, 32818]); % High-pass filtered values
xlabel("Sample #");
ylabel("Amplitude");
title("High-Passed DAC ADC Data");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

% Subplot for Mic Signal Power Spectrum(Bottom Left)
subplot(3, 2, 5); 
adc2_psd_plot = plot(nan(1, 1), 'r');
xticks(0:5000:250000);
xlabel("Frequency (Hz)");
ylabel("Power (dB)");
title("Power Spectrum of Mic Data");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

% Subplot for Dac to ADC Signal Power Spectrum(Bottom Right)
subplot(3, 2, 6); 
adc1_psd_plot = plot(nan(1, 1), 'g');
xticks(0:5000:250000);
xlabel("Frequency (Hz)");
ylabel("Power (dB)");
title("Power Spectrum of DAC ADC Data");
grid on;
ax = gca;
ax.XColor = 'w';
ax.YColor = 'w';
ax.Title.Color = 'w';
ax.XLabel.Color = 'w';
ax.YLabel.Color = 'w';

for it = 1:ITERATIONS
    % Trigger ADC data acquisition
    write(arduino, 't', "char");
    
    % Retrieve size header for ADC data
    adc_byte_length = read_total_length(arduino);    % Total length of ADC data in bytes
    ADC_DATA_LENGTH = adc_byte_length / 2;           % Total number of ADC samples

    % Retrieve ADC1 data
    adc1_data = read_data(arduino, ADC_DATA_LENGTH);

    % Retrieve ADC2 data
    adc2_data = read_data(arduino, ADC_DATA_LENGTH);
    
    % High-Pass Filter on ADC1 and ADC2 data
    adc1_data_filt = highpass(adc1_data, 30000, SAMPLING_RATE);
    adc2_data_filt = highpass(adc2_data, 30000, SAMPLING_RATE);

    % Compute PSD using Periodogram for both ADC1 and ADC2
    [p_adc1, f_adc1] = periodogram(adc1_data_filt, rectwin(length(adc2_data_filt)), [], SAMPLING_RATE);
    [p_adc2, f_adc2] = periodogram(adc2_data_filt, rectwin(length(adc1_data_filt)), [], SAMPLING_RATE);


    % Update plots in real time
    if ACTIVATE_PLOTS
        % Update DAC to ADC data plot
        set(DAC_ADC_plot, 'YData', adc1_data);

        % Update DAC to ADC high-passed data plot
        set(adc1_filt_plot, 'YData', adc1_data_filt);

        % Update Mic data plot
        set(Mic_ADC_plot, 'YData', adc2_data);

        % Update Mic high-passed data plot
        set(adc2_filt_plot, 'YData', adc2_data_filt);

        % Update DAC to ADC Power Spectrum plot
        set(adc1_psd_plot, 'XData', f_adc1, 'YData', p_adc1);

        % Update Mic Power Spectrum plot
        set(adc2_psd_plot, 'XData', f_adc2, 'YData', p_adc2);

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
