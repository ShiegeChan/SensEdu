function [env, bp, rect, dc] = process_emg_buffer(emg_buffers, fir_coeffs, taps, fs, env_cutoff)
%PROCESS_EMG_BUFFER  Run the EMG DSP chain on one rolling buffer.
%
%   Inputs:
%     emg_buffers : (samples x ch_num) rolling buffer.
%     fir_coeffs  : band-pass FIR coefficients.
%     taps        : FIR taps.
%     fs          : sampling rate (Hz).
%     env_cutoff  : envelope low-pass cutoff (Hz).
%
%   Outputs (each (samples x ch_num)):
%     env  : envelope (future decision input).
%     bp   : band-passed signal.
%     rect : rectified band-pass.
%     dc   : DC-removed input buffer (before band-pass).
    dc = emg_buffers - mean(emg_buffers, 1);
    filtered = filter(fir_coeffs, 1, dc);
    bp = filtered((taps + 1):end, :);
    rect = abs(bp);
    env = envelop(rect, fs, env_cutoff);
end
