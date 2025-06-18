% This script generates a DAC signal used for cross-correlation. 
close all;
Fs = 64*32000; % DAC sampling frequency
t = 0:1/Fs:0.2e-3; % time 
signal_perfect = (3.3/2) * sin(32e3*t-pi/2); % perfect 32 kHz sine wave

dac_cycle_num = 10; % number of cycles of the sine-wave
signal_full = repmat(signal_perfect, 1, dac_cycle_num); % complete signal
tnew = linspace(0, dac_cycle_num*t(end), size(signal_full, 2)); % complete time

% gaussian filter parameter - standard deviation
sigma = 0.4 * dac_cycle_num; % adjusting sigma to match the length of the signal

% range of x values for the signal
x = linspace(-dac_cycle_num, dac_cycle_num, length(signal_full));

% compute the values for gaussian window
gaussian_window = exp(- (x.^2) / (2 * sigma^2));

% normalize
gaussian_window = gaussian_window / max(gaussian_window);

% apply the window to the signal
filtered_signal = signal_full .* gaussian_window;

%% Visualization
figure;
set(gcf, 'Position', get(0, 'Screensize')); % full screen
subplot(2,2,1);
plot(t, signal_perfect, 'LineWidth', 2);
title("The perfect sine wave sent to the speaker");
grid on;
ylabel("[V]");
xlabel("[s]");

subplot(2,2,2);
plot(tnew, signal_full, 'LineWidth', 2);
title("The full perfect sine wave sent to the speaker");
grid on;
ylabel("[V]");
xlabel("[s]");

% Plot the Gaussian window function
subplot(2,2,3);
plot(tnew, gaussian_window, 'LineWidth', 2);
title('Gaussian Window Function');
xlabel('[s]');
ylabel('Amplitude');
grid on;

subplot(2,2,4);
plot(tnew, filtered_signal, 'LineWidth', 2);
title('Filtered Signal with Gaussian Window');
xlabel('[s]');
ylabel('Filtered Signal');
grid on;