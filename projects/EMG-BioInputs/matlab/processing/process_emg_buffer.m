function [env, bp, rect, dc] = process_emg_buffer(emg_buffers, fir_coeffs, taps, fs, env_cutoff)
%PROCESS_EMG_BUFFER  Run the EMG DSP chain on one rolling buffer.
%   Shared by the live (EMG_BioInputs) and offline (EMG_Offline_Processor)
%   scripts so the processing is byte-for-byte identical in both.
%
%   Inputs:
%     emg_buffers : (samples x channels) rolling buffer (~1 s of data).
%     fir_coeffs  : band-pass FIR coefficients.
%     taps        : FIR taps (the first TAPS samples are dropped).
%     fs          : sampling rate (Hz).
%     env_cutoff  : envelope low-pass cutoff (Hz).
%
%   Outputs (each (samples-taps x channels)):
%     env  : linear envelope (the decision input).
%     bp   : band-passed signal.
%     rect : rectified band-pass.
%     dc   : DC-removed input buffer (before band-pass; full length).
%
%   Steps (all causal, real-time safe):
%     1. Remove the large ADC DC offset BEFORE band-passing. With 150 taps at
%        Fs = 5 kHz the FIR transition is ~Fs/TAPS wide, so a 30 Hz lower edge
%        cannot reject DC (sum(fir_coeffs) ~ 0.2). Left in, the ~25k offset
%        leaks ~5k counts into the output and buries the EMG.
%     2. Band-pass FIR.
%     3. Drop the first TAPS samples: removes the FIR start-up transient and
%        compensates the TAPS/2 linear-phase group delay in one step.
%     4. Rectify.
%     5. Linear envelope (low-pass of the rectified signal).
    dc = emg_buffers - mean(emg_buffers, 1);
    filtered = filter(fir_coeffs, 1, dc);
    bp = filtered((taps + 1):end, :);
    rect = abs(bp);
    env = envelop(rect, fs, env_cutoff);
end
