function current_max = adjust_current_max(all_history_max, last_1min_max, last_1sec_max)
    current_max = 0.0.*all_history_max + 0.3.*last_1min_max + 0.7*last_1sec_max;
    for i = 1:size(all_history_max, 1)
        %limit = 0.25*all_history_max(i);
        limit = 150; % hard coded value here is less error prone
        if current_max(i) < limit
            current_max(i) = limit;
        end
    end
end