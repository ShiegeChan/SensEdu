function plot_processing_steps(emg_buffers, dc, bp, rect, env, fs, ch_num)
%PLOT_PROCESSING_STEPS  Debug view of the DSP pipeline over the rolling buffer.
%   Shows every stage of the EMG processing chain applied to the current
%   rolling buffer, one stage per row. Intended for documentation and
%   tuning.
%
%   Stages: raw -> DC removed -> band-pass -> rectified -> envelope. 
%   
%   The later stages are shorter than the raw buffer (the first TAPS samples 
%   and the envelope group delay are dropped), so each stage is drawn aligned 
%   to the newest sample.
%
%   Inputs:
%     emg_buffers : raw rolling buffer (samples x ch_num).
%     dc          : DC-removed buffer.
%     bp          : band-passed signal.
%     rect        : rectified band-pass.
%     env         : envelope.
%     fs          : sampling rate (Hz).
%     ch_num      : number of channels.
    
    % Up to which channel plots are drawn.
    % Helpful to debug specific channel.
    MAX_DISP_CH = ch_num;

    disp_channels = 1:MAX_DISP_CH;
    stages = {emg_buffers(:, disp_channels), 'Raw rolling buffer',     'ADC Value'; ...
              dc(:, disp_channels),          'DC removed',             ''; ...
              bp(:, disp_channels),          'Band-pass (30-450 Hz)',  ''; ...
              rect(:, disp_channels),        'Rectified',              ''; ...
              env(:, disp_channels),         'Envelope (10 Hz LP)',    ''};
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

        % Time label only on the bottom row.
        if s == n_stages
            xlabel('Time [s]');
        end

        % Channel legend only on the first row.
        if s == 1
            legend(ch_labels, 'Location', 'northwest');
        end
    end
    sgtitle('Processing steps over the rolling buffer');
end
