%% FMCW_Ranging.m 
% adc3 = DAC (to ADC) data
% adc1 = MIC DATA
clear;
close all;
clc;

%% FMCW system parameters
% Need to be the same as chirp parameters for correct distance computation!
f_start = 30500;         % Start frequency of transmitted chirp (Hz)
f_end = 35500;           % End frequency of transmitted chirp (Hz)
t_chirp = 0.040;         % Duration of one chirp (s)
c = 343;                 % Speed of sound in air for T=300K (m/s)
%% High-Pass FIR filter for coupling signal between Tx and Rx
Fstop = 50;              % Stopband Frequency
Fpass = 300;             % Passband Frequency
Dstop = 0.01;            % Stopband Attenuation
Dpass = 0.057501127785;  % Passband Ripple
dens  = 20;              % Density Factor
%% Settings
ARDUINO_PORT = 'COM41';
ARDUINO_BAUDRATE = 115200;
ITERATIONS = 100;           % Number of real-time ADC measurements
Fs = 250000;                % ADC Sampling rate
ACTIVATE_PLOTS = true;      % Toggle plotting on/off
CHUNK_SIZE = 32;            % Matches the memory chunk size in firmware

% Connect to Arduino
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE);


%% Real-Time ADC Data Acquisition and PSD
fprintf("Starting real-time dual ADC data acquisition...\n");

if ACTIVATE_PLOTS == true
    [mixed_signal_plot, adc1_filt_plot, adc3_filt_plot, mixed_signal_filt_plot, mixed_signal_filt_PS, distanceText, fbeatText] = plotting();
end

flush(arduino);

for it = 1:ITERATIONS
    % Trigger ADC data acquisition
    write(arduino, 't', "char");

    % Retrieve size header for ADC data
    adc_byte_length = read_total_length(arduino);     
    ADC_DATA_LENGTH = adc_byte_length / 2;   

    % Retrieve DAC to ADC data
    adc3_data = read_data(arduino, ADC_DATA_LENGTH, CHUNK_SIZE);

    % Retrieve Mic ADC data
    adc1_data = read_data(arduino, ADC_DATA_LENGTH, CHUNK_SIZE); 
    
    % High-Pass Filter on adc1 and adc3 Data
    adc3_data_filt = highpass(adc3_data, 30000, Fs);
    adc1_data_filt = highpass(adc1_data, 30000, Fs);

    % Frequency mixing (multiply Tx and Rx signals)
    mixed_signal = adc3_data_filt .* adc1_data_filt;
    
    % Low-pass filter to remove high frequency component
    mixed_signal_filt = lowpass(mixed_signal, 5000, Fs);

    % High-Pass FIR filter for coupling signal between Tx and Rx
    [N, Fo, Ao, W] = firpmord([Fstop, Fpass]/(Fs/2), [0 1], [Dstop, Dpass]);
    b  = firpm(N, Fo, Ao, W, {dens});
    HP = dfilt.dffir(b);
    mixed_signal_filt = filter(HP, mixed_signal_filt);

    % Compute PSD
    [p_mix, f_mix] = periodogram(mixed_signal, [], [], Fs);
    [p_mix_filt, f_mix_filt] = periodogram(mixed_signal_filt, hamming(length(mixed_signal_filt)),[], Fs);

    % Extract beat frequency
    [p_fbeat, fbeat] = findpeaks(p_mix_filt, f_mix_filt, NPeaks=1, SortStr="descend");

    d = (fbeat * t_chirp * c) / (2*(f_end - f_start));  % in meters
    disp("Distance: ");
    disp(d);

    if ACTIVATE_PLOTS == true
        % Update the distance & beat frequency 
        set(distanceText, 'String', sprintf('Distance = %.0f cm', d*100));
        set(fbeatText, 'String', sprintf('Beat Frequency = %.0f Hz', fbeat));
        
        % Update Mixed Signal plot
        set(mixed_signal_plot, 'YData', mixed_signal);

        % Update DAC to ADC Data
        set(adc3_filt_plot, 'YData', adc3_data_filt);

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

function data = read_data(arduino, data_length, chunk_size)
    total_byte_length = data_length * 2; % 2 bytes per sample
    serial_rx_data = zeros(1, total_byte_length, 'uint8');
    bytes_read = 0;
    while bytes_read < total_byte_length 
        transfer_size = min(chunk_size, total_byte_length - bytes_read);
        serial_rx_data(bytes_read + 1 : bytes_read + transfer_size) = read(arduino, transfer_size, 'uint8');
        bytes_read = bytes_read + transfer_size;
    end
    data = double(typecast(uint8(serial_rx_data), 'uint16'));
end

function [mixed_signal_plot, adc1_filt_plot, adc3_filt_plot, mixed_signal_filt_plot, mixed_signal_filt_PS, distanceText, fbeatText] = plotting()

    figure('Name', 'Real-Time ADC Signals and Power Spectra', 'Color', 'w', 'WindowState', 'maximized');
    
    % Subplot for Mixed Signal(Middle Left)
    subplot(3, 2, 3); 
    mixed_signal_plot = plot(nan(1, 1), "Color", [0.9451 0.76863 0.058824]);
    xlabel("Sample #");
    ylabel("Amplitude");
    title("Mixed Signal");
    grid on;
    fontsize(16,"points");
    
    % Subplot for Mic (adc1) High-Passed Signal (Top Left)
    subplot(3, 2, 1); 
    adc1_filt_plot = plot(nan(1, 1), "Color", [0.13333 0.4549 0.64706]);
    xlabel("Sample #");
    ylabel("Amplitude");
    title("Mic Data (Rx)");
    grid on;
    fontsize(16,"points");
    
    % Subplot for DAC (to adc3) High-Passed Signal (Top Right)
    subplot(3, 2, 2);
    adc3_filt_plot = plot(nan(1, 1), "Color", [0.96863 0.36078 0.011765]);
    xlabel("Sample #");
    ylabel("Amplitude");
    title("DAC Data (Tx)");
    grid on;
    fontsize(16,"points");
    
    % Subplot for Filtered Mixed Signal (Middle Left)
    subplot(3, 2, 4); 
    mixed_signal_filt_plot = plot(nan(1, 1), "Color", [0.9451 0.76863 0.058824]);
    xlabel("Sample #");
    ylabel("Amplitude");
    title("Filtered Mixed Signal");
    grid on;
    fontsize(16,"points");
    
    % Subplot for Filtered Mixed Signal Power Spectrum(Bottom Right)
    subplot(3, 2, [5,6]); 
    mixed_signal_filt_PS = plot(nan(1, 1), "Color", [0.9451 0.76863 0.058824], "LineWidth", 2);
    xlabel("Frequency (Hz)");
    ylabel("Power/Hz");
    xlim([0 10e3]);
    title("Power Spectrum of Filtered Mixed Signal");
    grid on;
    fontsize(16,"points");
    
    % Add distance & beat frequency display
    distanceText = uicontrol('Style', 'text', ...
                             'Units', 'normalized', ...
                             'Position', [0.4, 0.95, 0.2, 0.03], ... 
                             'String', 'Distance = 0 cm', ... 
                             'FontSize', 20, ...
                             'FontWeight', 'bold', ...
                             'ForegroundColor', 'k', ...
                             'BackgroundColor', 'w', ...
                             'HorizontalAlignment', 'center');
    
    fbeatText = uicontrol('Style', 'text', ...
                             'Units', 'normalized', ...
                             'Position', [0, 0.05, 0.2, 0.03], ... 
                             'String', 'Beat Frequency= 0 kHz', ...
                             'FontSize', 20, ...
                             'FontWeight', 'bold', ...
                             'ForegroundColor', 'k', ...
                             'BackgroundColor', 'w', ...
                             'HorizontalAlignment', 'center');
end