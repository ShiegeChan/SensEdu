function y = pctl(x, p)
%PCTL  Percentile linear interpolation.
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
    
    r = p / 100 * (n - 1) + 1;
    lo = floor(r);
    hi = ceil(r);
    if lo == hi
        y = x(lo);
    else
        y = x(lo) + (r - lo) * (x(hi) - x(lo));
    end
end
