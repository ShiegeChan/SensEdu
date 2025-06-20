function plot_max(max_index, current_max, all_history_max, last_1min_max, last_1sec_max)
    for i = 1:size(current_max, 1)
        %subplot(1,size(current_max, 1),i);
        hold on;
        plot([1,max_index], [current_max(i), current_max(i)], 'color', '#29505d', 'linewidth', 2.5);
        plot([1,max_index], [all_history_max(i), all_history_max(i)], 'color', '#010f1c', 'linewidth', 2.5);
        plot([1,max_index], [last_1min_max(i), last_1min_max(i)], 'color', '#304529', 'linewidth', 2.5);
        plot([1,max_index], [last_1sec_max(i), last_1sec_max(i)], 'color', '#4a6741', 'linewidth', 2.5);
        hold off;
    end
end