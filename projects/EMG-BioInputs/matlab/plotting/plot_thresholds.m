function plot_thresholds(max_index, current_max, press_thresholds, release_thresholds)
    for i = 3
        %subplot(1,size(current_max, 1),i);
        hold on;
        plot([1,max_index], [current_max(i), current_max(i)], 'color', '#000000', 'linewidth', 2.5);
        plot([1,max_index], [press_thresholds(i), press_thresholds(i)], 'color', '#6B8E23', 'linewidth', 2.5);
        plot([1,max_index], [release_thresholds(i), release_thresholds(i)], 'color', '#FF8C00', 'linewidth', 2.5);
        hold off;
    end
end