%% AnalyzeFSK.m
clear;
close all;
clc;

%% Settings
filename = "tarnished-dac.mat";

fs = 264e3;

fs_tx = 528e3;
N_tx = 200; % samples per bit in TX

f0 = 16e3;
f1 = 32e3;

threshold = 1e11;

%%
load(filename);
x = data(383850:500000);

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

%% Slide Goertzel
N = round(N_tx * (fs/fs_tx));
f = [f0, f1];
k = (f/fs)*N + 1;
L = length(x_filt);

num_steps = floor((L - N)/N);
decision = zeros(1, num_steps);

diff = zeros(1, num_steps);
plot_x = zeros(1, num_steps);

%figure;
for i = 1:num_steps
    idx = (i-1)*N + 1;
    plot_x(i) = idx + (idx+N - 1 - idx)/2;

    segment = x_filt(idx : idx+N - 1);
    %plot(idx : idx+N - 1, segment)
    %ylim([-65535, 65535])
    
    dtft1 = abs(goertzel(segment, k(1)))^2;
    dtft2 = abs(goertzel(segment, k(2)))^2;

    diff(i) = dtft2 - dtft1;
    if abs(diff(i)) > threshold
        decision(i) = sign(diff(i));
    end

    %hold on;
    %stem(plot_x(i), diff(i)/1e6);
    %hold off;
end

figure;
plot(x_filt);
hold on;
stem(plot_x, 65535.*decision);

%% Find Preamble
preamble = [1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0];
bits = decision > 0;

figure;
while (true)
    c = xcorr(bits, fliplr(preamble));
    c = c(length(bits):end);
    
    plot(c);
    
    [preamble_val, preamble_pos] = max(c);
    
    if preamble_val < 1 || length(bits) < length(preamble)
        break;
    end

    i = preamble_pos + length(preamble);
    msg = "";
    while (true)
        ascii = bits(i:i+7);
        ascii_char = char(bit2int(ascii', 8));
        msg = msg + ascii_char;
    
        if (ascii == 0)
            bits = bits(i:end);
            break;
        end
        i = i + 8;
    end
    
    disp(msg);

end
