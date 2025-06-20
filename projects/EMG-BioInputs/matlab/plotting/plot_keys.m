function plot_keys(keys_plotting_x, keys_plotting_y)
    for i = 3 % 1:size(keys_plotting_y, 1)
        %subplot(1,size(keys_plotting_y, 1),i);
        hold on;
        plot(keys_plotting_x, 200.*keys_plotting_y(i,:), 'color', '#29505d', 'linewidth', 2.5);
        hold off;
    end
end