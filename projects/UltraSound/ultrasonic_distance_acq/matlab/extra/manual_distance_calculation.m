% Define constants
ACTUAL_SAMPLING_RATE = 250e3; % Example value in Hz
air_speed = 343; % Speed of sound in air, in m/s

% Convert sampling rate into kilosamples per second
sr = ACTUAL_SAMPLING_RATE / 1000; % kS/s

% Assuming peak_index is known
peak_index = 549; % Example value (in kilosamples)

% Calculate distance in micrometers
% distance = ((peak_index * 1000 * air_speed) / sr) / 2;
distance = (peak_index * 1000 * air_speed) / (2 * sr); % Simplified
distance = distance / 1000000;
% Display the result
fprintf('Distance: %.8f meters\n', distance);