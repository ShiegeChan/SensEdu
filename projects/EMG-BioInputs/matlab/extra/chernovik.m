% Define parameters
fs = 25600;               % Sampling rate (25.6kHz)
buffer_duration = 1;      % History buffer (1 second)
samples_per_buffer = round(buffer_duration * fs);
window_duration = 40e-3;  % Decision window (40ms)
samples_per_window = round(window_duration * fs);

% Initialize buffer
buffer = zeros(samples_per_buffer, 1);

% Define filters
[b_bandpass, a_bandpass] = butter(4, [10, 500] / (fs / 2), 'bandpass');
[b_lowpass, a_lowpass] = butter(4, 5 / (fs / 2), 'low');

% Simulated real-time loop
for i = 1:num_iterations
    % Simulate receiving new data (40ms chunk)
    new_measurement = receive_emg_data();  % Replace with actual data input
    
    % Update buffer
    buffer = [buffer(samples_per_window+1:end); new_measurement];
    
    % Apply filtering to the entire 1-second buffer
    filtered_signal = filtfilt(b_bandpass, a_bandpass, buffer);
    
    % Envelope detection
    rectified_signal = abs(filtered_signal);
    envelope_signal = filtfilt(b_lowpass, a_lowpass, rectified_signal);
    
    % Extract decision window (last 40ms of the envelope)
    latest_envelope = envelope_signal(end-samples_per_window+1:end);
    
    % Decision logic (e.g., threshold detection)
    if max(latest_envelope) > THRESHOLD
        disp('Muscle Activation Detected');
    end
end

low_cutoff = 10;  % Bandpass filter low cutoff (Hz)
high_cutoff = 500; % Bandpass filter high cutoff (Hz)
[b, a] = butter(4, [low_cutoff, high_cutoff] / (fs / 2), 'bandpass');

% Apply bandpass filter with zero-phase filtering
filtered_buffer = filtfilt(b, a, buffer);



