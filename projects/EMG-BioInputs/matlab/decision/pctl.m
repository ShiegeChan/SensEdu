function y = pctl(x, p)
%PCTL  Linear-interpolated percentile (matches numpy.percentile default).
%   Shared by the live and offline decision code so the calibration does not
%   depend on the Statistics Toolbox prctile.
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
