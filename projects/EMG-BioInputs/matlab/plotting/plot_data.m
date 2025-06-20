function plot_data(processed_x, buffer, buffer_plotting_offsets, rectified_buffer, enveloped_buffer)
    for i = 3
        %subplot(1,size(buffer,1),i);
        %plot(buffer(i,:) - buffer_plotting_offsets(i));
        %hold on;
        plot(processed_x, rectified_buffer(i,:));
        hold on;
        plot(processed_x, enveloped_buffer(i,:), 'r', 'linewidth', 2.5);
        ylim([-600,800]);
        %legend(["Raw Data (centered)", "Filtered and Rectified", "Envelope"]);
        hold off;
    end
end