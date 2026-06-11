function [th_high, th_low, base, span] = calibrate_thresholds(env, p)
%CALIBRATE_THRESHOLDS  Per-channel onset/release thresholds from an envelope.
%   Shared by the live and offline scripts. Each channel is scaled to its OWN
%   signal so the detector is robust to amplitude changes (muscle fatigue,
%   electrode position):
%       base B = p.base_pctl-th percentile of the envelope (rest level)
%       span S = p.span_pctl-th percentile - B           (activation level)
%       onset   threshold = B + p.frac_high * S
%       release threshold = B + p.frac_low  * S
%   A channel whose span is below p.min_span (e.g. no electrode connected)
%   returns inf thresholds, so it can never trigger.
%
%   Inputs:
%     env : (samples x channels) envelope history to calibrate from.
%     p   : struct with fields base_pctl, span_pctl, frac_high, frac_low,
%           min_span.
%
%   Outputs (each 1 x channels):
%     th_high, th_low : onset / release thresholds (inf for inactive channels).
%     base, span      : the measured baseline and span (for reporting).
    ch_num = size(env, 2);
    base = zeros(1, ch_num);
    span = zeros(1, ch_num);
    th_high = inf(1, ch_num);
    th_low  = inf(1, ch_num);
    for ch = 1:ch_num
        B = pctl(env(:, ch), p.base_pctl);
        S = pctl(env(:, ch), p.span_pctl) - B;
        base(ch) = B;
        span(ch) = S;
        if S >= p.min_span
            th_high(ch) = B + p.frac_high * S;
            th_low(ch)  = B + p.frac_low  * S;
        end
    end
end

function y = pctl(x, p)
    % Linear-interpolated percentile (matches numpy.percentile default) so the
    % calibration does not depend on the Statistics Toolbox prctile.
    x = sort(x(:));
    n = numel(x);
    if n == 0
        y = NaN;
        return;
    end
    if n == 1
        y = x(1);
        return;
    end
    r = p / 100 * (n - 1) + 1;     % 1-based fractional rank
    lo = floor(r);
    hi = ceil(r);
    if lo == hi
        y = x(lo);
    else
        y = x(lo) + (r - lo) * (x(hi) - x(lo));
    end
end
