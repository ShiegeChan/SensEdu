%% FMCW_Distance_Measurement.m 
%% adc3 = DAC (to ADC) data
%% adc1 = MIC DATA
clear;
close all;
clc;

%% Radar system parameters
%Need to be the same as chirp parameters for correct distance computation!

f_start = 30000;         % Start frequency of transmitted chirp (Hz)
f_end = 35000;           % End frequency of transmitted chirp (Hz)
Tc = 0.01;              % Duration of one chirp (s)
c = 343;                 % Speed of sound in air for T=300K (m/s)
%d_cor = -0.05;            % Distance correction (m)   

%% Settings
ARDUINO_PORT = 'COM13';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 10000;        % Number of real-time ADC measurements
SAMPLING_RATE = 250000;    % ADC Sampling rate
ACTIVATE_PLOTS = true;     % Toggle plotting on/off

% Connect to Arduino
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % Initialize Arduino serial communication

%% Real-Time ADC Data Acquisition and PSD
fprintf("Starting real-time dual ADC data acquisition...\n");


% Initialize Figure for Real-Time Data Plots in Full Screen
figure('Name', 'Real-Time ADC Signals and Power Spectra', 'Color', 'w', 'WindowState', 'maximized');

% Subplot for Mixed Signal(Middle Left)
subplot(3, 2, 3); 
mixed_signal_plot = plot(nan(1, 1), 'r');
%xlim([0, 2048]);
xlabel("Sample #");
ylabel("Amplitude");
title("Mixed Tx & Rx Signal");
grid on;
%ax = gca;
%ax.XColor = 'w';
%ax.YColor = 'w';
%ax.Title.Color = 'w';
%ax.XLabel.Color = 'w';
%ax.YLabel.Color = 'w';

% Subplot for Mic (adc1) High-Passed Signal (Top Left)
subplot(3, 2, 1); 
adc1_filt_plot = plot(nan(1, 1), 'r');
%xlim([0, 2048]);
 ylim([-32818, 40000]); % High-pass filtered values
xlabel("Sample #");
ylabel("Amplitude");
title("High-Passed Mic Data");
grid on;
%ax = gca;
%ax.XColor = 'w';
%ax.YColor = 'w';
%ax.Title.Color = 'w';
%ax.XLabel.Color = 'w';
%ax.YLabel.Color = 'w';

% Subplot for DAC (to adc3) High-Passed Signal (Top Right)
subplot(3, 2, 2); 
adc3_filt_plot = plot(nan(1, 1), 'g');
%xlim([0, 2048]);
ylim([-32818, 32818]); % High-pass filtered values
xlabel("Sample #");
ylabel("Amplitude");
title("High-Passed DAC ADC Data");
grid on;
%ax = gca;
%ax.XColor = 'w';
%ax.YColor = 'w';
%ax.Title.Color = 'w';
%ax.XLabel.Color = 'w';
%ax.YLabel.Color = 'w';

% Subplot for Filtered Mixed Signal (Middle Left)
subplot(3, 2, 4); 
mixed_signal_filt_plot = plot(nan(1, 1), 'r');
%xticks(0:5000:100000);
%xlim([0, 2048]);
xlabel("Samples");
ylabel("Amplitude");
title("Filtered Mixed Signal");
grid on;
%ax = gca;
%ax.XColor = 'w';
%ax.YColor = 'w';
%ax.Title.Color = 'w';
%ax.XLabel.Color = 'w';
%x.YLabel.Color = 'w';

% Subplot for Filtered Mixed Signal Power Spectrum(Bottom Right)
subplot(3, 2, [5,6]); 
mixed_signal_filt_PS = plot(nan(1, 1), 'g');
%xticks(0:5000:250000);
xlim([0, 5000]);
xlabel("Frequency (Hz)");
ylabel("Power (dB)");
title("Power Spectrum of Filtered Mixed Signal");
grid on;
%ax = gca;
%ax.XColor = 'w';
%ax.YColor = 'w';
%ax.Title.Color = 'w';
%ax.XLabel.Color = 'w';
%ax.YLabel.Color = 'w';

% Add distance display
distanceText = uicontrol('Style', 'text', ...
                         'Units', 'normalized', ...
                         'Position', [0.4, 0.95, 0.2, 0.03], ... % Center horizontally and vertically
                         'String', 'Measured Distance = 0 cm', ... % Initial message
                         'FontSize', 20, ...
                         'FontWeight', 'bold', ...
                         'ForegroundColor', 'k', ...
                         'BackgroundColor', 'w', ...
                         'HorizontalAlignment', 'center');


for it = 1:ITERATIONS
    % Trigger ADC data acquisition
    write(arduino, 't', "char");
    
    % Retrieve size header for ADC data
    adc_byte_length = read_total_length(arduino);      % Total length of ADC data in bytes
    ADC_DATA_LENGTH = adc_byte_length / 2;             % Total number of ADC samples

    % Retrieve DAC to ADC data
    adc3_data = read_data(arduino, ADC_DATA_LENGTH);

    % Retrieve Mic ADC data
    adc1_data = read_data(arduino, ADC_DATA_LENGTH); 
    
    % High-Pass Filter on adc1 and adc3 Data
    adc3_data_filt = highpass(adc3_data, 30000, SAMPLING_RATE);
    adc1_data_filt = highpass(adc1_data, 30000, SAMPLING_RATE);

    % Frequency mixing (multiply Tx and Rx signals)
    mixed_signal = adc3_data_filt .* adc1_data_filt;
    mixed_signal_filt = lowpass(mixed_signal, 5000, SAMPLING_RATE);

    % Compute PSD using Periodogram for both adc1 and adc3
    [p_adc3, f_adc3] = periodogram(adc3_data_filt, rectwin(length(adc1_data_filt)), [], SAMPLING_RATE);
    [p_adc1, f_adc1] = periodogram(adc1_data_filt, rectwin(length(adc3_data_filt)), [], SAMPLING_RATE);
    [p_mix, f_mix] = periodogram(mixed_signal, [], [], SAMPLING_RATE);
    [p_mix_filt, f_mix_filt] = periodogram(mixed_signal_filt, hamming(length(mixed_signal_filt)),[], SAMPLING_RATE);
    %[p_mix_filt, f_mix_filt] = pspectrum(mixed_signal_filt, SAMPLING_RATE);

    % Extract beat frequency
    [p_fbeat,fbeat] = findpeaks(p_mix_filt,f_mix_filt,NPeaks=1,SortStr="descend")

    % Calculate distance (distance is for one way and not roundtrip like
    % usual FMCW radar since we use 2 boards here)
    d = (fbeat * Tc * c) / (f_end - f_start);

    %Real distance (with distance correction)
   % d_real = d + d_cor;
    
    % Update the distance display with the new value
    set(distanceText, 'String', sprintf('Measured Distance = %.0f cm', d*100));

    % Update plots in real time
    if ACTIVATE_PLOTS
        % Update Mixed Signal plot
        set(mixed_signal_plot, 'YData', mixed_signal);

        % Update DAC to ADC Data
        set(adc3_filt_plot, 'YData', adc3_data_filt);

        % Update Mixed Signal Power Spectrum
        %set(mixed_signal_PS, 'XData', f_mix,'YData', p_mix);

        % Update Mic Data plot
        set(adc1_filt_plot, 'YData', adc1_data_filt);

        % Update Filtered Mixed Signal plot
        set(mixed_signal_filt_plot, 'YData', mixed_signal_filt);

        % Update Filtered Mixed Signal Power Spectrum
        set(mixed_signal_filt_PS, 'XData', f_mix_filt, 'YData', p_mix_filt);

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
