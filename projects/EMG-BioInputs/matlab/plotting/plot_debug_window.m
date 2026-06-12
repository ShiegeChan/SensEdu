function plot_debug_window(t, env, env_dec, th_high, th_low, keys, ch_num, ch_button, win_s, floor_est, gap_est)
%PLOT_DEBUG_WINDOW  Retrospective debug view of the live decision (last win_s).
%   Per channel, draws the processed envelope (raw plus the smoothed value the
%   gate actually uses), the adaptive onset/release thresholds that were in
%   effect over time, and the resulting key-down decisions, so the gate
%   behaviour can be checked against the signal after the fact. A key DOWN is
%   shown three ways so it is unambiguous:
%     * a shaded green span over its whole duration,
%     * an onset (green triangle) and release (red triangle) marker placed on
%       the gate-input curve at the exact decision samples,
%     * a solid green "KEY" strip along the bottom of the axis.
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

        % Non-finite thresholds (before calibration / inactive channel) -> NaN
        % so they leave gaps instead of distorting the axis.
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

        % Key-down spans (start/stop sample indices of every press).
        kd     = keys(:, ch);
        edges  = diff([false; kd(:); false]);
        starts = find(edges == 1);
        stops  = find(edges == -1) - 1;

        % Shade each key-down span (drawn first = behind everything).
        for j = 1:numel(starts)
            xs = t(starts(j)); xe = t(stops(j));
            patch([xs xe xe xs], [yl(1) yl(1) yl(2) yl(2)], [0.2 0.7 0.2], ...
                'FaceAlpha', 0.18, 'EdgeColor', 'none', 'HandleVisibility', 'off');
        end

        h_env = plot(t, env(:, ch), 'Color', [0.7 0.7 0.7]);
        h_dec = plot(t, env_dec(:, ch), 'b');
        h_hi  = plot(t, th_h, '--', 'Color', [0.85 0.33 0.10]);
        h_lo  = plot(t, th_l, ':',  'Color', [0.85 0.33 0.10]);

        % Bottom "KEY" strip: solid green where the key is held. Anchored to a
        % thin band at the bottom of the axis so it reads regardless of scale.
        ks_y   = yl(1) + 0.03 * (yl(2) - yl(1));
        key_y  = nan(numel(kd), 1);
        key_y(kd) = ks_y;
        h_key = plot(t, key_y, 'Color', [0.15 0.6 0.15], 'LineWidth', 5);

        % Onset (green up-triangle) and release (red down-triangle) markers on
        % the gate-input curve, at the exact samples the gate switched.
        h_on = []; h_off = [];
        if ~isempty(starts)
            h_on = plot(t(starts), env_dec(starts, ch), '^', ...
                'MarkerEdgeColor', [0 0.5 0], 'MarkerFaceColor', [0.2 0.8 0.2], ...
                'MarkerSize', 8, 'LineStyle', 'none');
        end
        if ~isempty(stops)
            h_off = plot(t(stops), env_dec(stops, ch), 'v', ...
                'MarkerEdgeColor', [0.6 0 0], 'MarkerFaceColor', [0.9 0.3 0.3], ...
                'MarkerSize', 8, 'LineStyle', 'none');
        end
        hold off;

        ylim(yl);
        xlim([t(1) t(end)]);
        title(sprintf('Channel %d [%s] - %d press(es)', ch, ch_button{ch}, numel(starts)));
        xlabel('Time (s)'); ylabel('Envelope');
        if ch == 1
            % Build the legend only from handles that exist this frame.
            lh = [h_env h_dec h_hi h_lo h_key];
            ll = {'envelope', 'gate input', 'onset thr', 'release thr', 'key DOWN'};
            if ~isempty(h_on);  lh(end+1) = h_on;  ll{end+1} = 'press';  end
            if ~isempty(h_off); lh(end+1) = h_off; ll{end+1} = 'lift';   end
            legend(lh, ll, 'Location', 'northwest');
        end
    end
    sgtitle(sprintf('Last %g s   (shaded / bottom strip = key DOWN)', win_s));
end
