%% AnalyzeFSK.m
% Decodes FSK-modulated ultrasonic data
clear;
close all;
clc;

%% Settings
FILENAME = "tarnished-air.mat";

% Sampling Rates
Fs_tx = 480e3;  % TX SR
Fs = 240e3;     % RX SR (must be a multiple of TX)

% Samples per bit
N_tx = 200;
N = round(N_tx * (Fs/Fs_tx));

% FSK Encoding frequencies
% Must be the multiples of fs/N!
F0 = 31200;
F1 = 36000;
F = [F0, F1];

% Framing Correction Offset Step
HOP = N/20;

% Preamble 0xFF00FF00
% Must match with TX Arduino sketch
PREAMBLE = [1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0];

% Bandpass Filter Toggle
IS_FILTER_ON = false;

%% Data Load
load(FILENAME);
x = data;

%% Bandpass Filter
[B,A] = butter(2, [F0-5e3 F1+5e3]/(Fs/2));
x_filt = filtfilt(B, A, x);

figure;
hold on;
title("Recorded Ultrasonic Wave")
plot(x);
plot(x_filt);
legend(["Original Wave", "Bandpass +-5kHz Filtered Wave"]);
xlabel("Sample Index");
ylabel("Amplitude");
hold off;

%% FFT
y = fft(x);
l = numel(x);
f = (-l/2:l/2-1) * (Fs/l);

figure;
subplot(2,1,1)
title("FFT (Original Wave)");
plot(f, abs(fftshift(y))/l, 'LineWidth', 3);
xlabel('Frequency (Hz)');
ylabel("Magnitude");
yscale log

y = fft(x_filt);
l = numel(x_filt);
f = (-l/2:l/2-1) * (Fs/l);

subplot(2,1,2)
title("FFT (Filtered Wave)");
plot(f, abs(fftshift(y))/l, 'LineWidth', 3);
xlabel('Frequency (Hz)');
ylabel("Magnitude");
yscale log

%% Decoding Logic
if IS_FILTER_ON == false
    x_filt = x;
    disp("Filter is off. Using the original signal for decoding.");
end

% Goertzel Coefficient (must be integers)
k = (F/Fs)*N + 1;

% 1. Apply Goertzel to calculate each bit frequency energy.
%    Whole dataset is processed multiple times with N/HOP offsets for further frame correction.
[energy_diff, energy_x_labels] = run_goertzel(x_filt, HOP, N, k);

% 2. Find best preamble position and frame offset
[best_hop, preamble_pos] = analyze_preamble(energy_diff, PREAMBLE);
best_energy_diff = energy_diff(best_hop, :);

% 3. Plot best convolution with preamble
plot_conv(best_energy_diff, PREAMBLE, preamble_pos);

% 4. Visualize Goertzel decisions with frame markers
plot_goertzel(x_filt, best_energy_diff, energy_x_labels(best_hop, :), N);

% 5. Decode bitstream to ASCII message
bits = energy2bits(best_energy_diff);
decode_bitstream(bits, PREAMBLE, preamble_pos);

%% functions
function [energy_diff, x_labels] = run_goertzel(data, hop, N, k)
    bit_num = floor((length(data) - N)/N);
    hop_num = N/hop;

    energy_diff = zeros(hop_num, bit_num);
    x_labels = zeros(hop_num, bit_num);

    for j = 1:hop_num
        for i = 1:bit_num
            idx = (j-1)*hop + (i-1)*N + 1;
            segment = data(idx : idx+N - 1);
    
            dtft1 = abs(goertzel(segment, k(1)))^2;
            dtft2 = abs(goertzel(segment, k(2)))^2;
        
            energy_diff(j, i) = dtft2 - dtft1;

            x_labels(j, i) = idx + N/2;
        end
    end
end

function [best_hop, best_preamble_pos] = analyze_preamble(energy_diff, preamble)
    hop_num = size(energy_diff, 1);
    correlations = zeros(1, hop_num);
    preamble_pos = zeros(1, hop_num);

    for i = 1:length(correlations)
        data = energy_diff(i, :);
        c = abs(conv(data, fliplr(preamble*2 - 1)));
        [correlations(i), idx] = max(c);
        preamble_pos(i) = idx - length(preamble) + 1;
    end

    [~, best_hop] = max(correlations);
    best_preamble_pos = preamble_pos(best_hop);
end

function bitstream = energy2bits(energy)
    bitstream = energy > 0;
end

function msg = decode_bitstream(bits, preamble, preamble_pos)

    % i is the first sample of the payload
    i = preamble_pos + length(preamble);
    msg = "";

    while (i + 7 <= length(bits))
        ascii = bits(i:i+7);
        ascii_val = bit2int(ascii', 8);
                
        if (ascii_val == 0)
            break;
        end

        msg = msg + char(ascii_val);
        i = i + 8;
    end

    disp("Decoded Message:");
    disp(msg);

end

function plot_conv(energy_diff, preamble, preamble_pos)
    c = conv(energy_diff, fliplr(preamble*2 - 1));
    figure;
    hold on;
    title("Preamble Convolution");
    plot(c);
    stem(energy_diff);
    stem(preamble_pos, max(energy_diff), 'g', 'LineWidth', 1.5);
    legend(["Convolution", "Goertzel Energy Difference", "Detected Preamble"]);
    xlabel("Bit Index");
    ylabel("Energy / Correlation");
    hold off;
end

function plot_goertzel(original_data, energy_array, energy_x_labels, N)
    figure;
    hold on;
    title("Goertzel Decision Frames")

    wave = original_data./max(abs(original_data));
    wave = wave - mean(wave);
    plot(wave, 'LineWidth', 1);

    energy_norm = energy_array ./ max(abs(energy_array));
    stem(energy_x_labels, energy_norm, 'LineWidth', 1.5);

    symbol_starts  = energy_x_labels - N/2;
    stem(symbol_starts, 0.5.*ones(1,length(symbol_starts)), '--', 'LineWidth', 0.5);
    
    legend(["Normalized Wave", "Frame Centers", "Frame Boundaries"]);
    xlabel("Sample Index");
    ylabel("Normalized Amplitude");
    hold off;
end
