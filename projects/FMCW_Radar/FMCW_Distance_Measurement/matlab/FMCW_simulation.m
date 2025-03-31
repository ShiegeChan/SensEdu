% Parameters for the transmitted chirp (Tx)
f1_start = 30000;         % Start frequency of transmitted chirp (Hz)
f1_end = 40000;           % End frequency of transmitted chirp (Hz)

% Radar system parameters
c = 343;                  % Speed of sound in air (m/s)
distance = 0.52;             % Distance to object (meters)
fs = 250000;              % Sampling frequency (Hz)
T = 0.02;                  % Duration of one chirp (s)
num_chirps = 4;           % Number of chirps in the overall signal

% Derived parameters for delay
delay_time = 2 * distance / c;         % Time delay (seconds)
delay_samples = round(delay_time * fs);% Convert delay into number of samples

% Generate time vector for one chirp
t = 0:1/fs:(T - 1/fs);                 % Time vector for one chirp

% Generate the transmitted chirp signal for one chirp
one_chirp = chirp(t, f1_start, T, f1_end, 'linear'); % Linear frequency sweep

% Repeat the chirp signal "num_chirps" times to create overall transmitted signal
Tx_chirp = repmat(one_chirp, 1, num_chirps);  
n_samples = length(Tx_chirp);            % Total number of samples

% Apply cyclic delay to simulate the received chirp
Rx_chirp = circshift(Tx_chirp, delay_samples); % Received signal delayed w.r.t. transmitted signal

% Frequency mixing (multiply Tx and Rx chirps)
mixed_signal = Tx_chirp .* Rx_chirp;

% Filtering the mixed signal with a low-pass filter
cutoff_frequency = 12000; % Low-pass filter cutoff
filtered_signal = lowpass(mixed_signal, cutoff_frequency, fs);

% Perform FFT to analyze the mixed and filtered signals
N = length(mixed_signal);    % Number of samples for FFT
disp(N);
frequencies = (0:N-1)*(fs/N);% Frequency axis
Y = fft(mixed_signal);       % FFT of mixed signal
Y_filtered = fft(filtered_signal); % FFT of filtered signal

% Generate time vector for plotting
t_total = (0:n_samples-1) / fs; % Time vector for all chirps

% --- Plot Tx and Rx on the same figure ---
figure(1);
set(gcf, 'WindowState', 'maximized'); % Fullscreen plot
clf;

% Transmitted Chirp (Tx)
subplot(2, 1, 1);
plot(t_total, Tx_chirp, 'b', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Amplitude');
title('Transmitted Chirp (Tx)');
grid on;

% Received Chirp (Rx)
subplot(2, 1, 2);
plot(t_total, Rx_chirp, 'r', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Amplitude');
title(['Received Chirp (Rx) with Delay (distance = ', num2str(distance), ' m)']);
grid on;

% --- Plot mixed signal and filtered mixed signal on another figure ---
figure(2);
set(gcf, 'WindowState', 'maximized'); % Fullscreen plot
clf;

% Mixed Signal (Unfiltered)
subplot(2, 1, 1);
plot(t_total, mixed_signal, 'k', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Amplitude');
title('Mixed Signal (Unfiltered)');
grid on;

% Filtered Mixed Signal
subplot(2, 1, 2);
plot(t_total, filtered_signal, 'g', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Amplitude');
title('Filtered Mixed Signal (Low-Pass, 12 kHz Cutoff)');
grid on;

% --- Plot FFTs of the mixed signal (unfiltered and filtered) on another figure ---
figure(3);
set(gcf, 'WindowState', 'maximized'); % Fullscreen plot
clf;

% FFT of Mixed Signal (Unfiltered)
subplot(2, 1, 1);
plot(frequencies(1:floor(N/2)), abs(Y(1:floor(N/2))), 'b', 'LineWidth', 1.5);
xlabel('Frequency (Hz)');
ylabel('|FFT(Mixed Signal)|');
xlim([0 100000]);
title('FFT of Mixed Signal (Unfiltered)');
grid on;

% FFT of Filtered Mixed Signal
subplot(2, 1, 2);
plot(frequencies(1:floor(N/2)), abs(Y_filtered(1:floor(N/2))), 'r', 'LineWidth', 1.5);
xlabel('Frequency (Hz)');
ylabel('|FFT(Filtered Signal)|');
xlim([0 10000]);
title('FFT of Filtered Mixed Signal (12 kHz Cutoff)');
grid on;

% Display the detected beat frequency from the filtered signal
[~, peak_index] = max(abs(Y_filtered(1:floor(N/2))));
beat_frequency = frequencies(peak_index);
calculated_distance = (beat_frequency * T * c) / (2 * (f1_end - f1_start));

disp(['Beat Frequency Detected (Filtered): ', num2str(beat_frequency), ' Hz']);
disp(['Calculated Distance: ', num2str(calculated_distance), ' meters']);
