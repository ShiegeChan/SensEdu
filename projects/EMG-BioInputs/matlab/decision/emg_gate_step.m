function [gate, keys_down, done, onset, off] = emg_gate_step(v, k, gate, th_high, th_low, hangover)
%EMG_GATE_STEP  One causal step of the per-channel press/release gate.
%   Online state machine shared by the live and offline scripts. Each channel
%   drives one button: a contraction crossing th_high turns the gate ON;
%   dropping below th_low for "hangover" chunks turns it OFF. The hysteresis
%   (high onset / low release) stops chatter and the hangover bridges brief
%   envelope dips inside one sustained contraction. The button is pressed at
%   the contraction ONSET (low latency) and released after the hangover.
%
%   Inputs:
%     v        : 1 x ch envelope sample for this chunk (already smoothed).
%     k        : current chunk index (used only for onset/off bookkeeping).
%     gate     : persistent struct with 1 x ch fields .mode/.onset/.off/.gap
%                (.mode: 0 idle / 1 active / 2 pending-release).
%     th_high  : 1 x ch onset thresholds  (inf -> channel disabled).
%     th_low   : 1 x ch release thresholds.
%     hangover : release debounce, in chunks.
%
%   Outputs:
%     gate      : updated state struct.
%     keys_down : 1 x ch logical, true while the button is held (live use).
%     done      : 1 x ch logical, true on the chunk an activation finalizes.
%     onset/off : 1 x ch, the finalized activation's onset/off chunk
%                 (valid only where done is true; offline event use).
    ch_num = numel(v);
    done  = false(1, ch_num);
    onset = zeros(1, ch_num);
    off   = zeros(1, ch_num);
    for ch = 1:ch_num
        switch gate.mode(ch)
            case 0   % idle: wait for a clear onset
                if v(ch) > th_high(ch)
                    gate.mode(ch) = 1;
                    gate.onset(ch) = k;
                end
            case 1   % active: contraction in progress
                if v(ch) <= th_low(ch)
                    gate.mode(ch) = 2;          % maybe finished -> start hangover
                    gate.off(ch) = k;
                    gate.gap(ch) = 1;
                end
            case 2   % pending release: bridge brief dips (hangover)
                if v(ch) > th_low(ch)
                    gate.mode(ch) = 1;          % dip bridged, same contraction
                else
                    gate.gap(ch) = gate.gap(ch) + 1;
                    if gate.gap(ch) >= hangover
                        done(ch)  = true;
                        onset(ch) = gate.onset(ch);
                        off(ch)   = gate.off(ch);
                        gate.mode(ch) = 0;      % released
                    end
                end
        end
    end
    keys_down = gate.mode ~= 0;
end
