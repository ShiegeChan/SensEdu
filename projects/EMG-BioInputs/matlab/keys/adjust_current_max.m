function current_max = adjust_current_max(current_max, all_history_max, last_1min_max, last_1sec_max)
    current_max = 0.2.*current_max + 0.0.*all_history_max + 0.3.*last_1min_max + 0.5*last_1sec_max;
    for i = 1:size(all_history_max, 1)
        %limit = 0.25*all_history_max(i);
        limit = 200; % hard coded value is less error prone due to random artifacts
        if current_max(i) < limit
            current_max(i) = limit;
        end
    end
end