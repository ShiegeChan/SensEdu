% Generate a test signal with 32 kHz + noise
t = 0:1/Fs:0.01; % 10 ms duration
signal = sin(2*pi*32e3*t) + 0.5*randn(size(t)); 
filtered_signal = filter(coefficients, 8, signal);
plot(t, signal, t, filtered_signal); % Compare raw vs. filtered