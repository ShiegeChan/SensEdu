function plot_debug_window(t, env, env_dec, th_high, th_low, keys, ch_num, ch_button, win_s, floor_est, gap_est)
%PLOT_DEBUG_WINDOW  Retrospective debug view of the live decision (last win_s).
%   Per channel, draws the processed envelope (raw plus the smoothed value the
%   gate actually uses), the adaptive onset/release thresholds that were in
%   effect over time, and the resulting key-down decisions (shaded spans), so
%   the gate behaviour can be checked against the signal after the fact.
%
%   The y-range is anchored to the slow-moving floor/ceiling (floor_est,
%   gap_est) rather than the per-frame data range, so a given press looks the
%   same size from one window to the next and windows can be compared.
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        cla;
        hold on;

        % Inf thresholds (before calibration / inactive channel) -> NaN so they
        % leave gaps instead of distorting the axis.
        th_h = th_high(:, ch); th_h(~isfinite(th_h)) = NaN;
        th_l = th_low(:, ch);  th_l(~isfinite(th_l)) = NaN;

        if isfinite(floor_est(ch)) && isfinite(gap_est(ch)) && gap_est(ch) > 0
            yl = [floor_est(ch) - 0.3 * gap_est(ch), ...
                  floor_est(ch) + 2.2 * gap_est(ch)];
        else
            % Not calibrated yet: fall back to the data range.
            vals = [env(:, ch); env_dec(:, ch)];
            lo = min(vals); hi = max(vals);
            if ~(isfinite(lo) && isfinite(hi)) || hi <= lo
                lo = 0; hi = 1;
            end
            pad = 0.05 * (hi - lo);
            yl = [lo - pad, hi + pad];
        end

        % Shade the spans where the key was held down (drawn first = behind).
        kd = keys(:, ch);
        edges  = diff([false; kd(:); false]);
        starts = find(edges == 1);
        stops  = find(edges == -1) - 1;
        for j = 1:numel(starts)
            xs = t(starts(j)); xe = t(stops(j));
            patch([xs xe xe xs], [yl(1) yl(1) yl(2) yl(2)], [0.2 0.7 0.2], ...
                'FaceAlpha', 0.15, 'EdgeColor', 'none', 'HandleVisibility', 'off');
        end

        h_env = plot(t, env(:, ch), 'Color', [0.7 0.7 0.7]);
        h_dec = plot(t, env_dec(:, ch), 'b');
        h_hi  = plot(t, th_h, '--', 'Color', [0.85 0.33 0.10]);
        h_lo  = plot(t, th_l, ':',  'Color', [0.85 0.33 0.10]);
        hold off;

        ylim(yl);
        xlim([t(1) t(end)]);
        title(sprintf('Channel %d [%s]', ch, ch_button{ch}));
        xlabel('Time (s)'); ylabel('Envelope');
        if ch == 1
            legend([h_env h_dec h_hi h_lo], ...
                {'envelope', 'gate input', 'onset', 'release'}, ...
                'Location', 'northwest');
        end
    end
    sgtitle(sprintf('Last %g s   (shaded = key DOWN)', win_s));
end
