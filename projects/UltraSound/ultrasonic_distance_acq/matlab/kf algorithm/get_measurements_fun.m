function [y] = get_measurements_fun(x, microphones, sigma_r)
% GET_MEASUREMENTS_FUN
% Function to give measurements based on the current
% position of the object which in reality we don't know but in this case we
% are modeling the expected trajectory 

mic_num = size(microphones, 1);

% Sound speed and frequency of ultrasound waves
speed_of_sound = 343;

% Input signal is one pulse
load("sineLUT.mat", "sine_lut");
sine_lut = double(sine_lut);
fac_scale = 3.5/4095;
scaled_sig = sine_lut * fac_scale;
pulsed_sig = repmat(scaled_sig, 10, 1);
step_size = 1e-6;
t = 0:step_size:3e-2; % change this to increase the range
P_begin = zeros(length(t), 1);
P_begin(1:length(pulsed_sig)) = pulsed_sig;

% Temp Variables
TOF_mics = zeros(mic_num, 1);

% Speaker is the transmitter 
Tx = 0;
Ty = 0;
Tz = 0;

% Target is defined as a receiver
Rx = x(1);
Ry = x(2);
Rz = x(3);

% Sending the wave from the speaker to space
[~, P_sent, t_sent] = pressure_fun_v2(P_begin, t, Tx, Rx, Ty, Ry, Tz, Rz, false); % input signal is pulsed train, with source at (0, 0) and object (1, 1)

for i=1:mic_num
    % select the microphone
    mic = microphones(i, :);

    %---------------REFLECTING WAVES------------------------------%
    
    % Reflecting from the object 
    [P_shift, ~, ~] = pressure_fun_v2(P_sent, t_sent, Rx, mic(1), Ry, mic(2), Rz, mic(3), true); 
    
    %---------------CROSS CORRELATION-----------------------------%
    
    % Perform cross correlation between the two signals 
    [xcorr_mic, ~] = xcorr(P_shift, P_sent, 'normalized');

    % Finding the index of the peak 
    [~, peak_index] = max(xcorr_mic);

    % Time delay (td) from the peak index 
    td = (peak_index - length(t)) * step_size;

    % Caluclating the total distance traversed using TOF (time of flight
    % information. This is what we will regard as a measurement in real
    % situation
    sigma_r = 0.005; 
    TOF_mics(i, 1) = double(td * speed_of_sound + randn(length(td), 1) * sigma_r); % in meters
end

y = TOF_mics./2;

end

