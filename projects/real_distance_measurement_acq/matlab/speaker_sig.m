
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
xlabel("[s]")

dac_cycle_num = 10;


signal_full = repmat(signal_perfect, 1, dac_cycle_num);

tnew = linspace(0, dac_cycle_num*t(end), size(signal_full, 2));

subplot(2,2,2);
plot(tnew, signal_full);
title("The full perfect sine wave sent to the speaker");
grid on;
ylabel("[V]")
xlabel("[s]")


% parameters
sigma = 600; % standard deviation
filter_size = 4100; % in samples

% create gaussian kernel
gaussian_kernel = fspecial("gaussian", [1, filter_size], sigma);

% normalize the kernel
gaussian_kernel = gaussian_kernel / sum(gaussian_kernel);

% apply to the signal using convolution
signal_filtered = conv(signal_full, gaussian_kernel, "same");
signal_filtered = (max(signal_filtered)-signal_filtered) / (max(signal_filtered) - min(signal_filtered));

subplot(2,2,3);
plot(tnew, signal_filtered);
title("The convoluted signal normalized");
grid on;
ylabel("[V]")
xlabel("[s]")

signal_filtered = signal_filtered .* signal_full;

subplot(2,2,4);
plot(tnew, signal_filtered);
title("The signal coming out of the speaker");
grid on;
ylabel("[V]")
xlabel("[s]")

