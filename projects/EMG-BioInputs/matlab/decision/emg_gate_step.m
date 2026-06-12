function [gate, keys_down, done, onset, off] = emg_gate_step(emg_level, k, gate, th_high, th_low, hangover)
%EMG_GATE_STEP  One step of the per-channel press/release gate.
%   Each channel drives one button: a contraction crossing th_high turns the
%   gate ON (key pressed). To turn it OFF, the envelope must drop
%   below th_low AND stay there for a full "hangover" window - only then is
%   the key released.
%
%   Inputs:
%     emg_level : smoothed envelope sample for this chunk.
%     k         : current chunk index (used only for bookkeeping).
%     gate      : state struct, passed in and returned updated:
%                  .mode  - 0 = idle (waiting for a contraction)
%                           1 = active (key held, contraction in progress)
%                           2 = pending release (below th_low, counting hangover)
%                  .onset - chunk index when the current activation started
%                  .off   - chunk index when the envelope first dropped below th_low
%                  .gap   - number of consecutive sub-threshold chunks counted so far
%     th_high   : onset thresholds.
%     th_low    : release thresholds.
%     hangover  : release debounce, in chunks.
%
%   Outputs:
%     gate      : updated state struct (pass back in on the next call).
%     keys_down : true while the key is pressed right now.
%     done      : true on the one chunk an activation ends.
%     onset/off : chunk indices of that activation's start and end.
%                 Used only by the offline processor; ignored by the live script.
    ch_num = numel(emg_level);
    done  = false(1, ch_num);
    onset = zeros(1, ch_num);
    off   = zeros(1, ch_num);
    for ch = 1:ch_num
        switch gate.mode(ch)

            % idle: wait for a clear onset
            case 0
                if emg_level(ch) > th_high(ch)
                    gate.mode(ch) = 1;
                    gate.onset(ch) = k;
                end
            
            % active: contraction in progress
            case 1
                if emg_level(ch) <= th_low(ch)
                    % maybe finished -> start hangover
                    gate.mode(ch) = 2;
                    gate.off(ch) = k;
                    gate.gap(ch) = 1;
                end

            % pending release: bridge brief dips (hangover)
            case 2
                if emg_level(ch) > th_low(ch)
                    % dip bridged, same contraction
                    gate.mode(ch) = 1;
                else
                    gate.gap(ch) = gate.gap(ch) + 1;
                    if gate.gap(ch) >= hangover
                        done(ch)  = true;
                        onset(ch) = gate.onset(ch);
                        off(ch)   = gate.off(ch);

                        % released
                        gate.mode(ch) = 0;
                    end
                end
        end
    end
    keys_down = gate.mode ~= 0;
end
