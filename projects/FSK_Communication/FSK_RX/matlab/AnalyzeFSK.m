%% AnalyzeFSK.m
clear;
close all;
clc;

%% Settings
filename = "tarnished-air.mat";

fs = 240e3;

fs_tx = 480e3;
N_tx = 200; % samples per bit in TX

f0 = 31200; % multiple of fs/N
f1 = 36000;

threshold = 1e6;

%%
load(filename);
x = data;

%% Filter
[B,A] = butter(2, [f0-5e3 f1+5e3]/(fs/2));
x_filt = filtfilt(B, A, x);

figure;
plot(x);
hold on;
plot(x_filt);

%% FFT
y = fft(x);
L = numel(x);
f = (-L/2:L/2-1) * (fs/L);

figure;
subplot(2,1,1)
plot(f, abs(fftshift(y))/L, 'LineWidth', 3);
xlabel('Frequency (Hz)');
ylabel('Amplitude');
yscale log

y = fft(x_filt);
L = numel(x_filt);
f = (-L/2:L/2-1) * (fs/L);

subplot(2,1,2)
plot(f, abs(fftshift(y))/L, 'LineWidth', 3);
xlabel('Frequency (Hz)');
ylabel('Amplitude');
yscale log

%% Logic
N = round(N_tx * (fs/fs_tx));
hop = N/20;
f = [f0, f1];
k = (f/fs)*N + 1;

[energy_diff, energy_x_labels] = run_goertzel(x_filt, hop, N, k);

preamble = [1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0];
[best_hop, preamble_pos] = analyze_preamble(energy_diff, preamble);

c = conv(energy_diff(best_hop, :), fliplr(preamble*2 - 1));
plot(c);
title("Preamble Convolution");
hold on;
stem(energy_diff(best_hop, :));
stem(preamble_pos, max(energy_diff(best_hop, :)));
legend(["Convolution", "Goertzel Decisions", "Detected Preamble Position"]);
hold off;

plot_goertzel(x_filt, energy_diff(best_hop, :), energy_x_labels(best_hop, :), N);

bits = energy2bits(energy_diff(best_hop, :), threshold);

decode_bitstream(bits, preamble, preamble_pos);

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
    correlations = zeros(1, size(energy_diff, 1));
    preamble_pos = zeros(size(correlations));

    figure;
    for i = 1:length(correlations)
        data = energy_diff(i, :);
        c = abs(conv(data, fliplr(preamble*2 - 1)));
        plot(c);
        title("Preamble Convolution");
        [correlations(i), idx] = max(c);
        preamble_pos(i) = idx - length(preamble) + 1;
    end
    [~, best_hop] = max(correlations);
    %best_hop = 18;
    best_preamble_pos = preamble_pos(best_hop);
end

function bitstream = energy2bits(energy, threshold)
    bitstream = energy > 0;
end

function msg = decode_bitstream(bits, preamble, preamble_pos)

    % first sample of the payload
    i = preamble_pos + length(preamble);
    msg = "";
    while (true)
        ascii = bits(i:i+7);
        ascii_char = char(bit2int(ascii', 8));
        
        % remove the msg before the dot
        if (bit2int(ascii', 8) == 0)
            bits = bits(i:end);
            break;
        end

        msg = msg + ascii_char;
        i = i + 8;
    end
    
    disp(msg);
    
end

function plot_goertzel(original_data, energy_array, energy_x_labels, N)
    figure;
    hold on;

    wave = original_data./max(abs(original_data));
    wave = wave - mean(wave);
    plot(wave, 'LineWidth', 1);

    energy = energy_array ./ max(abs(energy_array));
    stem(energy_x_labels, energy, 'LineWidth', 1.5);

    symbol_starts  = energy_x_labels - N/2;

    %yl = ylim;

    stem(symbol_starts, 0.5.*ones(1,length(symbol_starts)), '--', 'LineWidth', 0.5); %'--', 'Color', [0.2 0.6 1], 'LineWidth', 0.5
    
    hold off;
end