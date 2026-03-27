fs = 25600;  % Sampling frequency (Hz)

% Specification parameters
Fstop1 = 10;  % First stopband frequency
Fpass1 = 30;  % First passband frequency
Fpass2 = 300; % Second passband frequency
Fstop2 = 400; % Second stopband frequency
Astop1 = 60;  % Stopband 1 attenuation (dB)
Apass = 0.9;  % Passband ripple (dB)
Astop2 = 90;  % Stopband 2 attenuation (dB)

% Normalize frequencies to Nyquist frequency
freqs = [Fstop1 Fpass1 Fpass2 Fstop2] / (fs/2);
amps = [0 1 0];  % Desired amplitudes: 0 in stopbands, 1 in passband
dev = [10^(-Astop1/20) (10^(Apass/20)-1)/(10^(Apass/20)+1) 10^(-Astop2/20)];

% Estimate minimum filter order
[N, Fo, Ao, W] = firpmord(freqs, amps, dev, fs);
disp(['Estimated order: ', num2str(N)]);