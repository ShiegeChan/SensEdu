function [g, th_high, th_low] = dec_thresholds(floor_est, press_est, min_gap, frac_high, frac_low)
%DEC_THRESHOLDS  Press/release thresholds from the floor/press estimates.
%
%   Inputs:
%     floor_est : rest-level estimate per channel (NaN = not ready).
%     press_est : learned typical press height above the floor (NaN = not learned).
%     min_gap   : minimum onset height above the floor; noise/dead-channel guard.
%     frac_high : onset   fraction of g (e.g., 0.40 -> onset at 40% of the press height).
%     frac_low  : release fraction of g (e.g., 0.20 -> release at 20%).
%
%   Outputs:
%     g         : effective press height used.
%     th_high   : onset threshold - envelope must exceed this to start a press.
%     th_low    : release threshold - envelope must drop to this to finish a press.
%
%   frac_high and frac_low are fractions of g (0..1). frac_high > frac_low
%   creates hysteresis: the release sits lower than the onset so the gate does
%   not flicker when the envelope sits near a single 
%
%   g is the effective press height - the larger of the learned press_est and
%   the cold-start fallback (min_gap / frac_high).
%
    g = max(press_est, min_gap / frac_high);
    th_high = floor_est + frac_high * g;
    th_low  = floor_est + frac_low  * g;
end
