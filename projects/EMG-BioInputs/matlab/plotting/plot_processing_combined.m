function plot_processing_combined(dc, bp, rect, env, fs, ch_num)
%PLOT_PROCESSING_COMBINED  All DSP stages overlaid, one subplot per channel.
%   Same as plot_processing_steps, but instead of one stage per row, this
%   overlays the processing stages on a single axes per channel, so their
%   relative shape and scale can be compared directly. The raw buffer is
%   omitted, since it is the same as DC removed, just shifted downwards.
%
%   The later stages are shorter than the raw buffer (the first TAPS samples 
%   and the envelope group delay are dropped), so each stage is drawn aligned 
%   to the newest sample.
%
%   Inputs:
%     dc     : DC-removed buffer (samples x ch_num).
%     bp     : band-passed signal.
%     rect   : rectified band-pass.
%     env    : envelope.
%     fs     : sampling rate (Hz).
%     ch_num : number of channels.
    stages = {dc,   'DC removed',            [0.70 0.70 0.70]; ...
              bp,   'Band-pass (30-450 Hz)', [0.00 0.45 0.74]; ...
              rect, 'Rectified',             [0.85 0.55 0.20]; ...
              env,  'Envelope (10 Hz LP)',   [0.85 0.10 0.10]};
    n_stages = size(stages, 1);

    % DC stage is the longest, so it sets the left edge.
    xl = [-(size(dc, 1) - 1) / fs, 0];

    for ch = 1:ch_num
        if ch_num > 1
            subplot(2, ch_num/2, ch);
        end
        cla;
        hold on;

        h = gobjects(1, n_stages);
        for s = 1:n_stages
            data = stages{s, 1}(:, ch);

            % Time axis aligned to the newest sample (now = 0, past negative).
            n = size(data, 1);
            t = ((0:n-1) - (n-1)) / fs;

            % Make the envelope stand out.
            lw = 1;
            if s == n_stages
                lw = 1.5;
            end
            h(s) = plot(t, data, 'Color', stages{s, 3}, 'LineWidth', lw);
        end
        hold off;

        xlim(xl);
        title(sprintf('Channel %d', ch));
        xlabel('Time [s]');
        ylabel('Counts');

        legend(h, stages(:, 2), 'Location', 'northwest');
    end
    sgtitle('Processing steps over the rolling buffer (combined)');
end
