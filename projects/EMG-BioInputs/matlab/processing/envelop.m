function enveloped_data = envelop(data, fs, cutoff)
%ENVELOP  Causal linear envelope (rectified-signal low-pass) of EMG data.
%   Shared by the live (EMG_BioInputs) and offline (EMG_Offline_Processor)
%   scripts so the envelope is computed identically in both.
%
%   A 2nd-order Butterworth low-pass is applied causally (filter, not
%   filtfilt) and the average group delay below the cutoff is trimmed from
%   the front so the output lines up in time.
    [b, a] = butter(2, cutoff / (fs / 2), 'low');
    enveloped_data = filter(b, a, data);

    [gd, w] = grpdelay(b, a, 512, fs);
    avg_gd = mean(gd(w <= cutoff));
    d = max(0, round(avg_gd));

    enveloped_data = enveloped_data(d+1:end, :);
end
