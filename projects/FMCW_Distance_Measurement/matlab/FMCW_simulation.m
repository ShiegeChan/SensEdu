% Parameters for the transmitted chirp (Tx)
f1_start = 30000;         % Start frequency of transmitted chirp (Hz)
f1_end = 35000;           % End frequency of transmitted chirp (Hz)

% Radar system parameters
c = 343;                  % Speed of sound in air (m/s)
distance = 3.5;             % Distance to object (meters)
fs = 150000;              % Sampling frequency (Hz)
T = 0.025;                 % Duration of one chirp (s)
num_chirps = 1;           % Number of chirps in the overall signal

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
Rx_chirp = circshift(Tx_chirp, delay_samples); % Received signal (delayed Tx)

% Frequency mixing (multiply Tx and Rx chirps)
mixed_signal = Tx_chirp .* Rx_chirp;

% Filtering the mixed signal with a low-pass filter
cutoff_frequency = 6000; % Low-pass filter cutoff /!\ Needs to be higher than the bandwidth
filtered_signal = lowpass(mixed_signal, cutoff_frequency, fs);

% Perform FFT to analyze the mixed and filtered signals
N = length(mixed_signal);    % Number of samples for FFT
disp(N);
frequencies = (0:N-1)*(fs/N);% Frequency axis
Y = fft(mixed_signal);       % DFT of mixed signal
Y_filtered = fft(filtered_signal); % DFT of filtered signal

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
title('$\bf{Transmitted \ Chirp - T_{x}(t)}$','interpreter','latex');
grid on;

% Received Chirp (Rx)
subplot(2, 1, 2);
plot(t_total, Rx_chirp, 'r', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Amplitude');
title('$\bf{Received \ Chirp - R_{x}(t)}$','Interpreter','latex');
grid on;
fontsize(16,"points");



% --- Plot mixed signal and filtered mixed signal on another figure ---
figure(2);
set(gcf, 'WindowState', 'maximized'); % Fullscreen plot
clf;

% Mixed Signal (Unfiltered)
subplot(2, 1, 1);
plot(t_total, mixed_signal, 'k', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Amplitude');
title('$\bf{Mixed \ Signal - \chi(t)}$','Interpreter','latex');
grid on;
fontsize(16,"points");

% Filtered Mixed Signal
subplot(2, 1, 2);
plot(t_total, filtered_signal, 'g', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Amplitude');
title('$\bf{Filtered \ Mixed \ Signal - \chi_{filt}(t)}$','Interpreter','latex');
grid on;
fontsize(16,"points");
orient('landscape');
%print('Mix_simulation.pdf','-dpdf', '-fillpage');






% --- Plot DFTs of the mixed signal (unfiltered and filtered) on another figure ---
figure(3);
set(gcf, 'WindowState', 'maximized'); % Fullscreen plot
clf;

% DFT of Mixed Signal (Unfiltered)
subplot(2, 1, 1);
plot(frequencies(1:floor(N/2)), abs(Y(1:floor(N/2))), 'b', 'LineWidth', 1.5);
xlabel('Frequency (Hz)');
ylabel('$|DFT(\chi(t))|$','Interpreter','latex');
%xlim([0 100000]);
title('$\bf{DFT\ of\ \ Mixed \ Signal - \chi(t)}$','Interpreter','latex');
grid on;
fontsize(16,"points");

% DFT of Filtered Mixed Signal
subplot(2, 1, 2);
plot(frequencies(1:floor(N/2)), abs(Y_filtered(1:floor(N/2))), 'r', 'LineWidth', 1.5);
xlabel('Frequency (Hz)');
ylabel('$|DFT(\chi_{filt}(t))|$','Interpreter','latex');
xlim([0 5000]);
title('$\bf{DFT\ of\ Filtered \ Mixed \ Signal - \chi_{filt}(t)}$','Interpreter','latex');
grid on;
fontsize(16,"points");
print('DFT_simulation_bad.pdf','-dpdf', '-fillpage');



% Display the detected beat frequency from the filtered signal
[~, peak_index] = max(abs(Y_filtered(1:floor(N/2))));
beat_frequency = frequencies(peak_index);
calculated_distance = (beat_frequency * T * c) / (2 * (f1_end - f1_start));

disp(['Beat Frequency Detected (Filtered): ', num2str(beat_frequency), ' Hz']);
disp(['Calculated Distance: ', num2str(calculated_distance), ' meters']);
