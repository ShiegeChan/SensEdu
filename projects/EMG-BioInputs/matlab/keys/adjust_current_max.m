function current_max = adjust_current_max(current_max, all_history_max, last_1min_max, last_1sec_max)
    current_max = 0.6.*last_1min_max + 0.4*last_1sec_max; %0.2.*current_max + 
    for i = 1:size(all_history_max, 1)
        limit = 200; % hard coded minimum value
        if current_max(i) < limit
            current_max(i) = limit;
        end
    end
end


