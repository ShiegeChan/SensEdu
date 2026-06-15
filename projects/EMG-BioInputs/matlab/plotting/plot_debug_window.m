function plot_debug_window(t, env, env_dec, th_high, th_low, keys, ch_num, ch_button, win_s, floor_est, gap_est)
%PLOT_DEBUG_WINDOW  Debug view of the last win_s seconds.
%   Per channel: raw envelope, smoothed gate input, adaptive onset/release
%   thresholds, and key-down decisions. A press is shown three ways:
%   shaded green span, onset (▲) / release (▼) markers, and a bottom strip.
%
%   The y-axis is anchored to floor_est / gap_est rather than the per-frame
%   data range, so the same press looks the same size every redraw.
%
%   Inputs:
%     t         : time axis.
%     env       : raw envelope history.
%     env_dec   : smoothed gate-input history.
%     th_high   : onset   threshold history (NaN before calibration).
%     th_low    : release threshold history (NaN before calibration).
%     keys      : key press history.
%     ch_num    : number of channels.
%     ch_button : cell array of button label strings {ch1, ch2, ...}.
%     win_s     : window duration in seconds.
%     floor_est : current rest floor per channel.
%     gap_est   : current effective press height per channel.
    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        cla;
        hold on;

        th_h = th_high(:, ch);
        th_h(~isfinite(th_h)) = NaN;

        th_l = th_low(:, ch);
        th_l(~isfinite(th_l)) = NaN;

        % Set ylim.
        if isfinite(floor_est(ch)) && isfinite(gap_est(ch)) && gap_est(ch) > 0
            yl = [floor_est(ch) - 0.3 * gap_est(ch), floor_est(ch) + 2.2 * gap_est(ch)];
        else
            % Not calibrated yet: fall back to the data range.
            vals = [env(:, ch); env_dec(:, ch)];
            lo = min(vals); 
            hi = max(vals);
            if ~(isfinite(lo) && isfinite(hi)) || hi <= lo
                lo = 0;
                hi = 1;
            end
            pad = 0.05 * (hi - lo);
            yl = [lo - pad, hi + pad];
        end

        % Key-down spans.
        key_down  = keys(:, ch);
        key_edges = diff([false; key_down(:); false]);
        starts    = find(key_edges == 1);
        stops     = find(key_edges == -1) - 1;

        % Shade each key-down span.
        for j = 1:numel(starts)
            xs = t(starts(j));
            xe = t(stops(j));
            patch([xs xe xe xs], [yl(1) yl(1) yl(2) yl(2)], [0.2 0.7 0.2], ...
                'FaceAlpha', 0.18, 'EdgeColor', 'none');
        end
        
        % Draw envelopes and thresholds.
        h_env = plot(t, env(:, ch), 'Color', [0.7 0.7 0.7]);
        h_dec = plot(t, env_dec(:, ch), 'b');
        h_hi  = plot(t, th_h, '--', 'Color', [0.85 0.33 0.10]);
        h_lo  = plot(t, th_l, ':',  'Color', [0.85 0.33 0.10]);

        % Bottom key strip.
        ks_y   = yl(1) + 0.03 * (yl(2) - yl(1));
        key_y  = nan(numel(key_down), 1);
        key_y(key_down) = ks_y;
        h_key = plot(t, key_y, 'Color', [0.15 0.6 0.15], 'LineWidth', 5);

        % Onset and release markers.
        h_on = [];
        h_off = [];
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
        
        % Plot settings.
        ylim(yl);
        xlim([t(1) t(end)]);
        title(sprintf('Channel %d [%s] - %d press(es)', ch, ch_button{ch}, numel(starts)));
        xlabel('Time (s)');
        ylabel('Envelope');
        if ch == 1
            % Use only existing handles this frame.
            lh = [h_env h_dec h_hi h_lo h_key];
            ll = {'Envelope', 'Gate Input', 'Onset Threshold', 'Release Threshold', 'Key Down'};
            if ~isempty(h_on)
                lh(end+1) = h_on;
                ll{end+1} = 'Press';
            end
            if ~isempty(h_off)
                lh(end+1) = h_off;
                ll{end+1} = 'Lift';
            end
            legend(lh, ll, 'Location', 'northwest');
        end
    end
    sgtitle(sprintf('Last %g s of EMG Data', win_s));
end
