% Parameters
f0 = 30500;          % Start frequency (Hz)
f1 = 35500;          % End frequency (Hz)
T = 0.001;             % Duration of sweep (seconds)
fs = 10 * f1;        % High sampling rate for smooth visualization (355 kHz)
t = 0:1/fs:T;        % Time vector

% Generate Chirp
y = chirp(t, f0, T, f1);

% Plotting for effective visualization
figure;
subplot(2,1,1);
plot(t, y);
title('Full Chirp Signal (30.5 kHz to 35.5 kHz)');
xlabel('Time (s)'); ylabel('Amplitude');

% Zoom into a small window to see the waveform
subplot(2,1,2);
plot(t, y);
xlim([0.0, T/2]); % Zoom into 1ms window in the middle
title('Zoomed View: Waveform Detail (No Aliasing)');
xlabel('Time (s)'); ylabel('Amplitude');
grid on;
