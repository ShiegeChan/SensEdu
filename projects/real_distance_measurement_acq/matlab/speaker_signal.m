close all;
clear;
signal_perfect_hex = ["0x0000","0x000A","0x0027","0x0058","0x009C","0x00F2","0x0159","0x01D1","0x0258","0x02ED","0x038E","0x043A","0x04F0","0x05AD","0x0670","0x0737",...
"0x0800","0x08C8","0x098F","0x0A52","0x0B0F","0x0BC5","0x0C71","0x0D12","0x0DA7","0x0E2E","0x0EA6","0x0F0D","0x0F63","0x0FA7","0x0FD8","0x0FF5",...
"0x0FFF","0x0FF5","0x0FD8","0x0FA7","0x0F63","0x0F0D","0x0EA6","0x0E2E","0x0DA7","0x0D12","0x0C71","0x0BC5","0x0B0F","0x0A52","0x098F","0x08C8",...
"0x0800","0x0737","0x0670","0x05AD","0x04F0","0x043A","0x038E","0x02ED","0x0258","0x01D1","0x0159","0x00F2","0x009C","0x0058","0x0027","0x000A"];

Fs = 64*32000;
t = 0:1/Fs:0.2e-3;
signal_perfect = (3.3/2) * sin(32e3*t-pi/2);

figure;
set(gcf, 'Position', get(0, 'Screensize'));
subplot(2,2,1);
plot(t, signal_perfect);
title("The perfect sine wave sent to the speaker");
grid on;
ylabel("[V]")
xlabel("sample #")

dac_cycle_num = 10;

signal_full = zeros(1, 10*4100);
signal_filtered = zeros(1, 10*4100);
signal_full(1:4100) = repmat(signal_perfect, 1, dac_cycle_num);
signal_full = signal_full;

tnew = linspace(0, dac_cycle_num*t(end), size(signal_full, 2));

subplot(2,2,2);
plot(tnew, signal_full);
title("The full perfect sine wave sent to the speaker");
grid on;
ylabel("[V]")
xlabel("sample #")


% parameters
sigma = 2050; % standard deviation
filter_size = 4100; % in samples

% create gaussian kernel
gaussian_kernel = fspecial("gaussian", [1, filter_size], sigma);

% normalize the kernel
gaussian_kernel = gaussian_kernel / sum(gaussian_kernel);


% apply to the signal using convolution
signal_filtered = conv(signal_full, gaussian_kernel, "same");
%signal_filtered = (signal_filtered) / (max(signal_filtered) - min(signal_filtered));

subplot(2,2,3);
plot(tnew, signal_filtered);
title("The convoluted signal normalized");
grid on;
ylabel("[V]")
xlabel("sample #")

signal_filtered = signal_filtered .* signal_full;

% multiply
%signal_filtered(1:4100) = signal_filtered(1:4100) .* signal_full(1:4100);

subplot(2,2,4);
plot(tnew, signal_filtered);
title("The signal coming out of the speaker");
grid on;
ylabel("[V]")
xlabel("sample #")

signal_repeated = repmat(signal_full, 1, 4);
subplot(2,2,2);
tnew_new = linspace(0, 4*tnew(end), size(signal_repeated, 2));

figure
subplot(2,1,1)
plot(tnew_new, signal_repeated);
title("The full perfect sine wave sent to the speaker");
grid on;
ylabel("[V]")
xlabel("sample #")

signal_filtered_repeated = conv(signal_repeated, gaussian_kernel, "same");
signal_filtered_repeated =(signal_filtered_repeated) / (max(signal_filtered_repeated) - min(signal_filtered_repeated));
signal_filtered_repeated = signal_filtered_repeated .* signal_repeated;

subplot(2,1,2)
plot(tnew_new, signal_filtered_repeated);
title("The full perfect sine wave sent to the speaker");
grid on;
ylabel("[V]")
xlabel("sample #")