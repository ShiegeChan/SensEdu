% FIR Bandpass Filter Coefficient Generator
% ------------------------------------------
% Design a bandpass filter around a target frequency (e.g., 32 kHz)
% Adjust parameters based on your ADC sampling rate and desired bandwidth.

clc;
clear;
close all;

%% 1. Define Filter Specifications
Fs = 244e3;         % Sampling rate (Hz) - must match your ADC's rate!
f_low = 30e3;       % Lower cutoff frequency (Hz)
f_high = 34e3;      % Upper cutoff frequency (Hz)
filter_order = 63;  % Filter order (higher = sharper transition, but slower)

% Normalized frequencies for MATLAB
nyquist = Fs/2;
f_pass = [f_low, f_high] / nyquist;

%% 2. Design the Filter
% Method 1: Equiripple (Parks-McClellan) - Optimal but requires Signal Processing Toolbox
coefficients = firpm(filter_order, [0 f_pass(1)-0.05 f_pass f_pass(2)+0.05 1], [0 0 1 1 0 0]);

% Method 2: Windowed (Hamming window) - Simpler
% coefficients = fir1(filter_order, f_pass, 'bandpass', hamming(filter_order+1));

%% 3. Analyze Frequency Response
freqz(coefficients, 1, 4096, Fs);  % Plot magnitude/phase response
title('Bandpass Filter Frequency Response');

%% 4. Export Coefficients for C/C++ (Arduino)
% Normalize coefficients (if needed) and format as a C array
coefficients = coefficients / max(abs(coefficients)); % Optional scaling

% Save to a .h file for CMSIS-DSP or Arduino
fid = fopen('bandpass_coeffs.h', 'w');
fprintf(fid, '// Bandpass Filter Coefficients (Fs = %.1f kHz, %.1f-%.1f kHz)\n', Fs/1e3, f_low/1e3, f_high/1e3);
fprintf(fid, 'const float32_t filterCoefficients[%d] = {\n', length(coefficients));
fprintf(fid, '    %.8ff, %.8ff, %.8ff, %.8ff,\n', coefficients(1:end-1));
fprintf(fid, '    %.8ff\n};\n', coefficients(end));
fclose(fid);

disp('Coefficients saved to bandpass_coeffs.h');