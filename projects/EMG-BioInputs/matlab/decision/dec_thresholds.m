function [g, th_high, th_low] = dec_thresholds(floor_est, press_est, min_gap, frac_high, frac_low)
%DEC_THRESHOLDS  Onset/release thresholds from the decoupled floor/press estimates.
%   Shared by the live (EMG_BioInputs) and offline (EMG_Offline_Processor)
%   decision code so both compute the gate thresholds identically.
%
%       g       = max(press_est, min_gap / frac_high)   % per channel
%       th_high = floor_est + frac_high * g             % onset
%       th_low  = floor_est + frac_low  * g             % release
%
%   max() ignores NaN, so an unlearned press_est falls back to the cold-start
%   gap, which puts the onset exactly min_gap above the floor. A NaN floor (a
%   channel whose rest level is not yet measured) makes the thresholds NaN, so
%   the gate can never fire on it.
    g = max(press_est, min_gap / frac_high);
    th_high = floor_est + frac_high * g;
    th_low  = floor_est + frac_low  * g;
end
