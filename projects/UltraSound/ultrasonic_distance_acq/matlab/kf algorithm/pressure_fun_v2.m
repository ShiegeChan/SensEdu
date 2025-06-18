function [P_shift, P_end, t_end] = pressure_fun_v2(P_begin, t, Tx, Rx, Ty, Ry, Tz, Rz, uniform_scatter)
% pressure_fun_with_angle is a function to determine the pressure at a
% point (x0, y0) with a source in point (x, y) and initial pressure P_begin
% as a function of time t. If the uniform_scatter is true, then the TX is
% uniformly scattering the output signal from 0-360 °C, otherwise, it is
% using the TX scattering characteristic

alpha = 0.1561; % air attenuation coefficient
% alpha = 1; % attenuation coefficient
f = 32000; % Frequency in Hz (32 kHz)
c = 343; % Speed of sound in air in m/s
lambda = c / f; % Wavelength
phi = 0; % Phase shift
k = 2 * pi / lambda; % Wave number
omega = 2 * pi * f; % Angular frequency
speaker_gain = 1.2468; % gain from the amplifier before the speaker on the board
%sigma = 0.01;
%sigma = 0.1;
sigma = 1e-3;

R = sqrt((Tx - Rx)^2 + (Ty - Ry)^2+ (Tz - Rz)^2); % converting to mm
epsilon = 1; % for computational purposes when r = 0.

theta = round(atan2(Ry-Ty, Rx-Tx), 2); % calculate the angle theta

td = R/c; % calculate the time delay

if (uniform_scatter == false)
    load("LUT.mat", 'speaker_gain_LUT');
    TX_Gain = speaker_gain * 10^(speaker_gain_LUT(theta)/20); % speaker gain
else
    TX_Gain = 1; % otherwise it is a uniform scattering
end

P_end =  TX_Gain * P_begin .* (1/((R+epsilon))) .* exp(-alpha * R) + randn(length(P_begin), 1) * sigma;
% 25.11.2024 Taking the noise out of this function and putting it in the
% measurement function 
% P_end =  TX_Gain * P_begin .* (1/((R+epsilon))) .* exp(-alpha * R); 
t_end = t + td;

% Convert shift amount to number of samples
shiftSamples = round(t_end / (t(2) - t(1)));

% Perform the circular shift
P_shift = circshift(P_end, shiftSamples);
end