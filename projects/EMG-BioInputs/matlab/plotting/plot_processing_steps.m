function plot_processing_steps(emg_buffers, dc, bp, rect, env, fs, ch_num)
%PLOT_PROCESSING_STEPS  Snapshot of the DSP pipeline over the rolling buffer.
%   Shows every stage of the EMG processing chain applied to the current ~1 s
%   rolling buffer, one stage per row, all channels overlaid. Intended for
%   documentation / understanding the pipeline rather than live tuning.
%
%   Stages: raw -> DC removed -> band-pass -> rectified -> envelope. The later
%   stages are shorter than the raw buffer (the first TAPS samples and the
%   envelope group delay are dropped), so each stage is drawn on its own time
%   axis aligned to the newest sample (now = 0, the past is negative).
%
%   The downstream decision smoothing (the asymmetric "gate input") is NOT
%   shown here: it is a chunk-rate decision filter, visualised separately in
%   plot_debug_window as the blue trace.
%
%   Inputs:
%     emg_buffers : raw rolling buffer (samples x ch_num), ADC counts.
%     dc          : DC-removed buffer (before band-pass).
%     bp          : band-passed signal.
%     rect        : rectified band-pass.
%     env         : linear envelope.
%     fs          : sampling rate (Hz).
%     ch_num      : number of channels.

    % Each stage: data, title, y-axis label.
    stages = {emg_buffers, 'Raw rolling buffer',     'ADC counts'; ...
              dc,          'DC removed',             'Counts'; ...
              bp,          'Band-pass (30-450 Hz)',  'Counts'; ...
              rect,        'Rectified',              'Counts'; ...
              env,         'Envelope (10 Hz LP)',    'Counts'};
    n_stages = size(stages, 1);

    % Channel legend labels.
    ch_labels = cell(1, ch_num);
    for c = 1:ch_num
        ch_labels{c} = sprintf('CH%d', c);
    end

    for s = 1:n_stages
        data = stages{s, 1};

        % Time axis aligned to the newest sample (now = 0, past = negative).
        n = size(data, 1);
        t = ((0:n-1) - (n-1)) / fs;

        subplot(n_stages, 1, s);
        plot(t, data);
        xlim([t(1) t(end)]);
        title(stages{s, 2});
        ylabel(stages{s, 3});

        % Time label only on the bottom row to keep the stack compact.
        if s == n_stages
            xlabel('Time (s)');
        end

        % Channel legend only on the first row.
        if s == 1
            legend(ch_labels, 'Location', 'northeast');
        end
    end
    sgtitle('Processing steps over the rolling buffer');
end
